//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arObjectUtilities.h"
#include <iostream>
#include "arGraphicsServer.h"
#include "arSZGClient.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arMesh.h"

arMatrix4 mouseWorldMatrix = arMatrix4();
arMatrix4 lightTransformMatrix = arMatrix4();
arVector3 lightDirection = arVector3();
int mouseTransformID, lightTransformID, light0ID;
enum { PAN=0, ROTATE, ZOOM, SLIDER, NONE };
int mouseManipState = ROTATE, oldState=ROTATE;
bool isPlaying = false, isModelSpinning = false, isLightSpinning = false;
float bgColor = 0.;
float _ratio = 1.;

float	frameRate = 0.;
arGraphicsDatabase* theDatabase = NULL;
int lastX=0, lastY=0;		// prev. mouse position
int _width=0, _height=0;	// window dimensions
int sliderCenterX=-50, sliderCenterY=10; // slider position in pixels

arObject *theObject = NULL;

/// Drawing functions ///
void showRasterString(float x, float y, char* s){
  glRasterPos2f(x, y);
  char c;
  for (; (c=*s) != '\0'; ++s)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

void drawHUD() {
  float width = _width, height = _height;
  char displayString[128];
  glColor3f(.9,.9,.9);
  glBegin(GL_QUADS);	// draw box to hold text
    glVertex2f(  0, 100);
    glVertex2f(  0, 100.*(1.-20./height));
    glVertex2f(100, 100.*(1.-20./height));
    glVertex2f(100, 100);
  glEnd();
  float startPos = 0, endPos = 0;
  switch(oldState) {
    case PAN:	 startPos = (    0.)*100./width;
		 endPos   = (15.*9.)*100./width;
      break;
    case ROTATE: startPos = (16.*9.)*100./width;
		 endPos   = (28.*9.)*100./width;
      break;
    case ZOOM:	 startPos = (29.*9.)*100./width;
		 endPos   = (40.*9.)*100./width;
      break;
  };
  glColor3f(.75,.75,.80);
  glBegin(GL_QUADS);	// draw highlighted box
    glVertex2f(startPos, 100);
    glVertex2f(startPos, 100.*(1.-20./height));
    glVertex2f(endPos,   100.*(1.-20./height));
    glVertex2f(endPos,   100);
  glEnd();
  // menu text
  glColor3f(0,0,0);
  sprintf(displayString, " [1] Translate | [2] Rotate | [3] Scale");
  showRasterString(0, 100.*(1.-15./height), displayString);
  
  // animation slider
  if (theObject->supportsAnimation() && theObject->numberOfFrames() > 0) {
    glColor3f(.9,.9,.9);
    glBegin(GL_QUADS);	// draw box for slider et. al.
      glVertex2f(100, 0);
      glVertex2f(100, 100.*(20./height));
      glVertex2f(  0, 100.*(20./height));
      glVertex2f(  0, 0);
    glEnd();
    glColor3f(1.0,1.0,1.0);
    glBegin(GL_QUADS);	// draw box for current frame number
      glVertex2f(100.-2.*100./width, 100.*02./height);
      glVertex2f(100.-2.*100./width, 100.*18./height);
      glVertex2f(100.-(4.+4.*9.)*100./width, 100.*20./height);
      glVertex2f(100.-(4.+4.*9.)*100./width, 100.*02./height);
    glEnd();
    
    float numFrames = theObject->numberOfFrames();
    float sliderStart = 2.*9.*100./width;
    float sliderEnd   = 100.-(4.+6.*9.)*100./width;
    float sliderWidth = sliderEnd - sliderStart;
    float sliderMaxY = 100.*15./height, sliderMinY = 100.*5./height;
    float sliderSpacing = sliderWidth;
    
    glColor3f(.25,.25,.25);
    glBegin(GL_LINES);	// draw slider bar
      glVertex2f(sliderStart, (sliderMinY+sliderMaxY)/2.);
      glVertex2f(sliderEnd,   (sliderMinY+sliderMaxY)/2.);
    /// \todo Only do this once per window resize isntead of every frame
    float stepSize = 1.;
    // make each tick mark at least 10 px across onscreen, and geometric series
    for (;;) {
      sliderSpacing = (sliderWidth*width/100.)/stepSize;
      if (sliderSpacing <= 10.)// && sliderSpacing <= 20.)
	break;
      else if ((int)stepSize % 2 == 0)
	stepSize *= 2.5;
      else
	stepSize *= 2.;
    }
    // draw tick marks
    for (float j=0.; j<=sliderWidth; j+=sliderSpacing) {
      glVertex2f(sliderStart+j, sliderMaxY);
      glVertex2f(sliderStart+j, sliderMinY);
    }
    glVertex2f(sliderEnd, sliderMaxY);
    glVertex2f(sliderEnd, sliderMinY);

    /* */
    glEnd();
    
    float sliderCenter = (sliderStart +
		    (float)theObject->currentFrame()/numFrames*sliderWidth);
    sliderCenterX = (int)(width*sliderCenter/100.); // for the mouse func
    
    glColor3f(.5,.5,.5);
    glBegin(GL_QUADS);	// draw slider position
      glVertex2f(sliderCenter-5.*100./width, sliderMaxY+4.*100./height);
      glVertex2f(sliderCenter-5.*100./width, sliderMinY-4.*100./height);
      glVertex2f(sliderCenter+5.*100./width, sliderMinY-4.*100./height);
      glVertex2f(sliderCenter+5.*100./width, sliderMaxY+4.*100./height);
    glEnd();
    
    glColor3f(0,0,0);
    sprintf(displayString, "%4i", theObject->currentFrame());
    showRasterString(100.-(2.+4.*9.)*100./width,
		     100.*02./height, displayString);
  }
}

void draw(){
  if (isPlaying) // loop animation
    if (!theObject->nextFrame())
      theObject->setFrame(0);

  if (isModelSpinning) {
    mouseWorldMatrix =	ar_rotationMatrix(arVector3(0.,1.,0.), .001)
    			* ar_rotationMatrix(arVector3(1.,0.,0.), .002)
	    		* mouseWorldMatrix;
    dgTransform(mouseTransformID, mouseWorldMatrix);
  }
  if (isLightSpinning) {
    lightTransformMatrix = ar_rotationMatrix(arVector3(1.,0.,0.), .05)
    			* ar_rotationMatrix(arVector3(0.,1.,0.), .07)
    			* ar_rotationMatrix(arVector3(0.,0.,1.), .03)
	    		* lightTransformMatrix;
    arVector3 tmp(lightTransformMatrix*lightDirection);
    dgLight(light0ID, 0, arVector4(tmp[0],tmp[1],tmp[2],0), arVector3(1.0,1.0,0.95));
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // set up the viewing transformation
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.03*_ratio,0.03*_ratio,-0.03,0.03,0.1,100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  gluLookAt(0.0,0.0,-5.0, 0.0,0.0,0.0, 0.0,1.0,0.0);
  theDatabase->activateLights();
  theDatabase->draw();
  // draw info screen
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0,100,0,100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glColor3f(1,1,1);
  drawHUD();

  glutSwapBuffers();
}

void idle(){
  draw();
}

void keyboard(unsigned char key, int /*x*/, int /*y*/){
  switch (key) {	// basic/global functions
    case 27: exit(0);
    case 'f': glutFullScreen();
      break;
    case 'F': glutReshapeWindow(500,500);
      break;
    case '1': mouseManipState = oldState = PAN;
      break;
    case '2': mouseManipState = oldState = ROTATE;
      break;
    case '3': mouseManipState = oldState = ZOOM;
      break;
    case '`': bgColor = (bgColor==1 ? 0 : bgColor+.25);
	      glClearColor(bgColor,bgColor,bgColor,1.);
      break;
    case '9': isLightSpinning = !isLightSpinning;
	      break;
    case '0': isModelSpinning = !isModelSpinning;
	      break;
    case 'd': theDatabase->printStructure();
      break;
    case 'p': theDatabase->printStructure(9);
      break;
    case 'o': theDatabase->printStructure();
      break;
  }
  if (theObject->supportsAnimation())	// animation functions
    switch (key) {
      case 'j': theObject->prevFrame();
	break;
      case 'k': isPlaying ^= 1;
	break;
      case 'l': theObject->nextFrame();
	break;
    }
}


void resizeFunction(int w, int h) {
  _width = w;
  _height = h;
  _ratio = (float)w/(float)h;
  glViewport(0, 0, w, h);
  //draw();
}

void mouseFunction(int button, int state, int x, int y){
  lastX = x;
  lastY = y;
 
  if (button != GLUT_LEFT_BUTTON ||
      !(theObject->supportsAnimation()) ||
      theObject->numberOfFrames() < 1 )
    return;
  // mouse down on the slider bar
  if (y > _height-20 && state == GLUT_DOWN) {
    oldState = mouseManipState;
    if (x <= _width-57 && x >= 20)
      mouseManipState = SLIDER;
    else
      mouseManipState = NONE;
  }
  
  if (mouseManipState == SLIDER) { // we're dragging
    if (x < 20)
      theObject->setFrame(0);
    else if (x > _width-57)
      theObject->setFrame(theObject->numberOfFrames()-1);
    else
      theObject->setFrame((int)((float)(x-20)/(float)(_width-77) *
			  (float)theObject->numberOfFrames()) );
  }
  if (state == GLUT_UP)
    mouseManipState = oldState;	// done dragging bar
}

void motionFunction(int x, int y){
  static bool initialized = false;
  if (!initialized){
    lastX = x;
    lastY = y;
    initialized = true;
  }

  int deltaX = x - lastX;
  int deltaY = y - lastY;
  arVector3 rotationAxis;
  switch (mouseManipState){
    case SLIDER:
      // the SGI compiler does not like the following:
      //return mouseFunction(0,-1,x,y);
      mouseFunction(0,-1,x,y);
      return;
      break;
    case PAN:
      mouseWorldMatrix = ar_translationMatrix(deltaX*-0.01,deltaY*-0.01,0)
			 * mouseWorldMatrix;
      break;
    case ROTATE:
      rotationAxis = arVector3(0,0,1)*arVector3(deltaX,deltaY,0);
      if (++rotationAxis > 0){
        float mag = ++rotationAxis;
        rotationAxis = rotationAxis/mag;
        mouseWorldMatrix= ar_extractTranslationMatrix(mouseWorldMatrix)
 		          //* ar_rotationMatrix(rotationAxis, mag/300.0)
 		          * ar_rotationMatrix(arVector3(-1,0,0), (float)deltaY /300.0)
 		          * ar_rotationMatrix(arVector3(0,1,0), (float)deltaX /300.0)
		          * ar_extractRotationMatrix(mouseWorldMatrix)
			  * ar_extractScaleMatrix(mouseWorldMatrix);
      }
      break;
    case ZOOM:
      mouseWorldMatrix = mouseWorldMatrix * ar_scaleMatrix(1. + 0.01*deltaX);
      break;
  }
  dgTransform(mouseTransformID, mouseWorldMatrix);
  lastX = x;
  lastY = y;
}

int main(int argc, char** argv){
  if (argc<2){
    cerr << "usage: " << argv[0] << " file.{obj|3ds|htr} [mesh.obj]\n"; 
    return 1;
  }
  
  theObject = arReadObjectFromFile(argv[1], "");
  if (!theObject) {
    cerr << "Invalid File: " << argv[1] << endl;
    return 1;
  }
  arObject *theMesh = NULL;
  if (argc == 3)
    theMesh = arReadObjectFromFile(argv[2], "");

  arSZGClient SZGClient;
  SZGClient.init(argc, argv);
  if (!SZGClient)
    return 1;

  theDatabase = new arGraphicsDatabase;
  char texPath[256];
#ifndef AR_USE_WIN_32
  getcwd(texPath, 256);
#else
  texPath[0] = '\0';
#endif
  theDatabase->setTexturePath(string(texPath));

  _width = 640;  // sensible defaults
  _height = 480;
  float sizeBuffer[2];
  // should use the screen name passed to the arSZGClient
  string screenName = SZGClient.getMode("graphics");
  if (SZGClient.getAttributeFloats(screenName, "size", sizeBuffer, 2)) {
    _width = (int) sizeBuffer[0];
    _height = (int) sizeBuffer[1];
  }

  arMatrix4 worldMatrix;
  dgSetGraphicsDatabase(theDatabase);
  dgTransform("world", "root", worldMatrix);
  int worldTransformID = theDatabase->getNodeID("world");
  dgTransform("mouse", "world", mouseWorldMatrix);
  mouseTransformID = theDatabase->getNodeID("mouse");
  
  dgTransform("lightTransform", "world", lightTransformMatrix );
  lightTransformID = theDatabase->getNodeID("lightTransform");
  
  lightDirection = arVector3(1, 2, -3);	// dir of primary light
  lightDirection /= ++lightDirection;
  // use as dir, not pos
  arVector4 lightDir(lightDirection[0],lightDirection[1],lightDirection[2],0);
  light0ID = dgLight("light0", "lightTransform", 0, lightDir, arVector3(1.0,1.0,0.95));
  theObject->normalizeModelSize(); // fits into unit sphere

  if (theObject->type() == "HTR") {
    if (argc == 3)
      attachOBJToHTRToNodeInDatabase((arOBJ*)theMesh,(arHTR*)theObject,"mouse");
    else
      ((arHTR*)theObject)->attachMesh("object", "mouse", true);
  }
  else
    theObject->attachMesh("object", "mouse");

  /* Useful for debugging */
  //theDatabase->prettyDump();

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH|GLUT_RGB);
  glutInitWindowPosition(0,0);
  if (_height <= 0 || _height <= 0){
    glutInitWindowSize(640, 480);
  }
  else{
    glutInitWindowSize(_width, _height); 
  }
  glutCreateWindow("Syzygy Object Viewer");
  glEnable(GL_DEPTH_TEST);
  glutDisplayFunc(draw);
  glutReshapeFunc(resizeFunction);
  glutKeyboardFunc(keyboard);
  glutMotionFunc(motionFunction);
  glutMouseFunc(mouseFunction);
  glutIdleFunc(idle);
  glutMainLoop();
  return 0;
}
