//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arTexture.h"
#include "arDataUtilities.h"
#include <math.h>

arTexture bitmap;
int screenWidth, screenHeight;

bool doSyncTest = false;
bool flipped = false;
arTimer timer;
const float FLIP_SECONDS = 2;
const float TEST_RED = .25;
const float STRIP_HEIGHT = 10;

void drawSyncTest() {
  glColorMask( GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE );
  if (flipped)
    glColor3f(TEST_RED,0,0);
  else
    glColor3f(0,0,0);
  glBegin( GL_QUADS );
  glVertex2f( 0, 0 );
  glVertex2f( screenWidth, 0 );
  glVertex2f( screenWidth, STRIP_HEIGHT );
  glVertex2f( 0, STRIP_HEIGHT );
  glEnd();
  if (flipped)
    glColor3f(0,0,0);
  else
    glColor3f(TEST_RED,0,0);
  glBegin( GL_QUADS );
  glVertex2f( 0, STRIP_HEIGHT );
  glVertex2f( screenWidth, STRIP_HEIGHT );
  glVertex2f( screenWidth, 2*STRIP_HEIGHT );
  glVertex2f( 0, 2*STRIP_HEIGHT );
  glEnd();
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );  
}

void idle(void){
  ar_usleep(100000); // CPU throttle
  glutPostRedisplay();
}

void keyboard(unsigned char key, int, int){
  switch (key) { 
    case 27:
      exit(0);
  }
}

void showCenteredImage( arTexture& theImage ) {
  const int imageWidth = theImage.getWidth();
  const int imageHeight = theImage.getHeight();
  int xOffset = (imageWidth < screenWidth) ?
    (int)floor( 0.5*(screenWidth-imageWidth) ) : 0;
  int yOffset = (imageHeight < screenHeight) ?
    (int)floor( 0.5*(screenHeight-imageHeight) ) : 0;
  glRasterPos3i( xOffset, yOffset, 0 );
  glDrawPixels( imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                theImage.getPixels());
}

void draw() {
  if (timer.done()) {
    flipped = !flipped;
    timer.start( FLIP_SECONDS*1e6 );
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,screenWidth,0,screenHeight,-1,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  showCenteredImage( bitmap );
  if (doSyncTest) {
    drawSyncTest();
  }
  glutSwapBuffers();
}

void reshape(int width, int height) {
  screenWidth = width;
  screenHeight = height;
  glViewport(0,0,width,height);
}

int main(int argc, char** argv){
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // it would be a good idea to bring this into the fold of *normal*
  // syzygy programs... right now, it is a bit of a special case.
  if ((argc < 2)||(argc > 3)){
    cerr << "usage: PictureViewer filename [synctest]\n";
    return 1;
  }
  if (argc == 3) {
    doSyncTest = true;
  }
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  // we expect to be able to get a lock on the computer's screen
  const string screenLock(szgClient.getComputerName() + string("/")
                      + szgClient.getMode("graphics"));
  int graphicsID;
  if (!szgClient.getLock(screenLock, graphicsID)){
    cout << "PictureViewer error: failed to get screen resource held "
	 << "by component "
         << graphicsID << ".\n(Kill that component to proceed.)\n";
    szgClient.sendInitResponse(false);
    return 1;
  }
  
  arThread dummy(ar_messageTask, &szgClient);

  // we want to pack the init response stream
  stringstream& initResponse = szgClient.initResponse();
  const string dataPath = szgClient.getAttribute("SZG_DATA","path");
  if (!bitmap.readPPM(argv[1], dataPath)){
    initResponse << argv[0] << " error: failed to read picture file " 
                 << argv[1] << " from path " << dataPath << ".\n";
    szgClient.sendInitResponse(false);
    return 1;
  }
  if (!bitmap.flipHorizontal()) {
    initResponse << "PictureViewer error: failed to flip picture " << argv[1]
                 << endl;
    szgClient.sendInitResponse(false);
    return 1;
  }
  // we have succeeded in the init
  initResponse << "The picture has been loaded.\n";
  szgClient.sendInitResponse(true); 

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(800,600);
  glutCreateWindow("Picture Viewer");
  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  glutFullScreen();

  // we have succeeded in starting
  szgClient.startResponse() << "The picture window has appeared.\n";
  szgClient.sendStartResponse(true);
  timer.start( FLIP_SECONDS*1e6 );
  glutMainLoop(); 
  return 0;
}
