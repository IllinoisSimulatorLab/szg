/** *************************************************************************
          schprel.cpp  -  schpecial relativistic motion simulator
                             -------------------
    begin                : Tue Jun 4 2002
    copyright            : (C) 2002 by mflider
    email                : mflider@uiuc
 ** *************************************************************************/

#include "arPrecompiled.h"
#include "arMesh.h"
#include "arGraphicsAPI.h"
#include "arGlut.h"
#ifndef AR_USE_WIN_32
  #include <sys/types.h>
  #include <signal.h>
#endif

#include "arMasterSlaveFramework.h"
#include "arMath.h"
#include "arInterfaceObject.h"
#include "arDataUtilities.h"

#include "arSZGClient.h"
#include "SchprelScene.h" // custom schprel headers
#include "Self.h"
#include "SpaceJunk.h"

// A power of 2 for efficient binary search.
static const unsigned int HOW_FAR_BACK = 2048;

string      interfaceType;
bool        stereoMode;
int         windowSizeX, windowSizeY;
arMatrix4   _headMatrix;
arMatrix4   worldMatrix;
int         worldTransformID;
int         STATE, STATE_MATRIX, STATE_JOYSTICK, STATE_BUTTON, STATE_FRAME,
            STATE_VOLUME, STATE_VELOCITY, STATE_LIGHTSPEED ;
float       nearclip = 0.1;
float       farclip = 1000.;

#define MOVING 1
#define FROZEN 0
int	theMode = MOVING;
bool	movingBackwards = false;
bool	button0Pressed = false;

ARfloat      *selfPosition = NULL;  		// keeps track of our recent movement
ARfloat      *selfPositionTimes = NULL; 	// time from current index to 0th
ARfloat      selfRotationAngles[3] = {0}; 	// current rotation (for drawing)
ARfloat	     selfOffset[3] = {0};
ARfloat      velocity = .9, lightspeed = 2.;
SchprelScene *scene = NULL;
SchprelScene *self = NULL;
ARfloat      fps = 0.;
float        bouncyLightPosition = 0.;
float        bouncyLightDirection = 1.; // this should be 1 or -1

#include "SpecialRelativityMath.h"

inline arMatrix4 selfRotationMatrix(void) {
  return ar_rotationMatrix(arVector3(1,0,0), selfRotationAngles[0]) *
         ar_rotationMatrix(arVector3(0,1,0), selfRotationAngles[1]) *
         ar_rotationMatrix(arVector3(0,0,1), selfRotationAngles[2]);
}

/// Displays past positions as white-to-black gradient line
void drawSelfPositionLine(void) {
  float tempColor;
  arVector3 tempPoint;
  arMatrix4 selfRotationInv = selfRotationMatrix().inverse();
  glBegin(GL_LINE_STRIP);
  for (unsigned int aa=0; aa < HOW_FAR_BACK*3; aa+=3){
    tempColor = 1.-(float)aa/(float)HOW_FAR_BACK;
    glColor3f(tempColor, tempColor, tempColor);
    tempPoint = arVector3(selfPosition[aa], selfPosition[aa+1], selfPosition[aa+2]);
    //tempPoint = selfRotationInv*tempPoint;
    glVertex3f(tempPoint[0], tempPoint[1], tempPoint[2]);
  }
  glEnd();
}

/// Draws how fast the speed of light is (visually)
/// NOTE: This functionality requires at least one fps draw time.
/// \todo: Add a velocity bar, perhaps?
void drawBouncyLight(float fps) {
  float temp;

  // draws a bar at the top of the front screen showing how far a photon would
  // travel during one second (or all the way across if c too big)
  temp = (lightspeed > 3.)? 100 : lightspeed*100./3.;
  glLineWidth(3);
  glBegin(GL_LINES);
    glVertex2f(0,99);
    glVertex2f(temp,99);
  glEnd();
  glLineWidth(1);

  // also, draw a bouncy light going back and forth between two mirrors,
  // like in spaceship thought experiment
  if (lightspeed < 3.) {
    // only draw if speed of light low enough to make this useful
    bouncyLightPosition = bouncyLightPosition +
	    bouncyLightDirection*1./fps*lightspeed;
    if (bouncyLightPosition > 3.) { 
      bouncyLightPosition = 6. - bouncyLightPosition;
      bouncyLightDirection *= -1.;
    }
    if (bouncyLightPosition < 0.) {
      bouncyLightPosition = 0. - bouncyLightPosition;
      bouncyLightDirection *= -1.;
    }
    temp = bouncyLightPosition*100./3.;
    float offset = 0.5;
    glBegin(GL_POLYGON);
      glVertex2f(temp+offset,98.);
      glVertex2f(temp,98.+offset);
      glVertex2f(temp-offset,98.);
      glVertex2f(temp,98.-offset);
    glEnd();
    glPointSize(1);
  }
}

/// Moves self and updates selfPosition circular queue
void updateSelfPosition (const arVector3 &transVec, const arMatrix4 &rotationMatrix) {
  arVector3 newTransVec(0,0,0);
  if (theMode == MOVING) {
    newTransVec = selfRotationMatrix().inverse()*transVec;
    for (int i=0; i<3; i++)
      selfOffset[i] += newTransVec[i];	// move Self
    newTransVec = transVec;
  }
  
  arVector3 tempVec;
  for (int bb=HOW_FAR_BACK-1; bb > 0; bb--) {
    tempVec = arVector3(selfPosition[bb*3-3],// - newTransVec[0],
	                selfPosition[bb*3-2],// - newTransVec[1],
	 	        selfPosition[bb*3-1]);// - newTransVec[2]);
    tempVec = rotationMatrix*tempVec;

    selfPosition[bb*3+0] = tempVec[0] - newTransVec[0];
    selfPosition[bb*3+1] = tempVec[1] - newTransVec[1];
    selfPosition[bb*3+2] = tempVec[2] - newTransVec[2];
  }

  //if (badMathCountdown && !freeze) badMathCountdown--;
}

/// adds time to each selfPosition (for accuracy)
void updateSelfPositionTimes(const float &lastFrameTime) {
float temp = 0.;
  for (int i=HOW_FAR_BACK-1; i>0; i--) {
    selfPositionTimes[i] = selfPositionTimes[i-1]+lastFrameTime;
    if (selfPositionTimes[i] > temp)
      temp = selfPositionTimes[i];
  }
}

// arServerFramework callbacks

bool init(arMasterSlaveFramework& fw, arSZGClient&){
  ar_navTranslate(arVector3(0., -5., 0.));
  fw.addTransferField("velocity", &velocity, AR_FLOAT, 1);
  fw.addTransferField("lightspeed", &lightspeed, AR_FLOAT, 1);
  // NOTE: pass in POINTERS, not POINTERS-to-POINTERS
  // fw.addTransferField("selfPosition",&selfPosition,...)
  // is wrong, since selfPosition is of type float*
  fw.addTransferField("selfPosition", selfPosition, AR_FLOAT, HOW_FAR_BACK*3);
  fw.addTransferField("selfPositionTimes", selfPositionTimes, 
                      AR_FLOAT, HOW_FAR_BACK);
  fw.addTransferField("selfRotationAngles", selfRotationAngles, AR_FLOAT, 3);
  fw.addTransferField("selfOffset", selfOffset, AR_FLOAT, 3);
  fw.addTransferField("theMode", &theMode, AR_INT, 1);
  return true;
}

void preExchange(arMasterSlaveFramework& fw){
  const ARfloat fpsRaw = 1000./fw.getLastFrameTime();
  static ARfloat fpsPrev = 40.;
  fps = fpsRaw * .05 + fpsPrev * .95;
  fpsPrev = fps;
  
  /// Handle Joystick controls ///
  arVector3 wandDirection(ar_extractRotationMatrix(fw.getMatrix(1)) *
		  	  arVector3(0,0,-1));
  double forwardSpeed = fw.getAxis(1);
  double barrelRoll = fw.getAxis(0);
  if (fabs(forwardSpeed) < 0.1)
    forwardSpeed = 0.;		// "dead zone"
  else if (forwardSpeed > 0.) {
    forwardSpeed = velocity;
    movingBackwards = false;
  }
  else {
    forwardSpeed = -velocity;
    movingBackwards = true;
  }
  if (fabs(barrelRoll) < 0.1)
    barrelRoll = 0.;		// "dead zone"

  /// hold button 1, use 0 and 2 to adjust velocity
  if (fw.getButton(1) < 0.5){
    if (fw.getButton(2) > 0.5)
      velocity = min(velocity*1.01, lightspeed-.001);
    if (fw.getButton(0) > 0.5)
      velocity /= 1.01;
  }
  /// use 0 and 2 (alone) to adjust speed/light
  else{
    if (fw.getButton(2) > 0.5)
      lightspeed *= 1.01;
    if (fw.getButton(0) > 0.5)
      lightspeed = max(lightspeed/1.01, velocity+.001);
  }
#if 0
  // start/stop moving //
  if (fw.getButton(0)) {
    if (!button0Pressed) {	// if button was up, now down
      theMode = 1 - theMode;
      button0Pressed = true;
    }
  }
  else
    button0Pressed = false;
#endif 
  /// End Joystick controls ///

  // make per unit time, not per frame
  barrelRoll /= fps;
  forwardSpeed /= fps;

#if 0  
  arVector3 rotVec(0,0,0);
  if (theMode == MOVING)
    rotVec = ar_extractEulerAngles(fw.getMatrix(1));

  rotVec /= fps*10.;	// make rotation per unit time, not per frame
  
  for (int i=0; i<3; i++) {
    if (rotVec[i]*rotVec[i] < .0025/fps)	// "dead zone"
      rotVec[i] = 0.;
    rotVec[i] *= -1.;//(1.-2.*(i%2));	// inverts "y" axis
    selfRotationAngles[i] += rotVec[i]; // adjusted navigation angles
  }
#endif

  /// \todo: fix barrelRoll, as it does not work.
  barrelRoll = 0.; // nullify barrelRoll. :(
  const arMatrix4 rotationMatrix = ar_rotationMatrix(wandDirection, barrelRoll);
  const arVector3 tempVec = ar_extractEulerAngles(rotationMatrix);
  selfRotationAngles[0] += tempVec.v[0];
  selfRotationAngles[1] += tempVec.v[1];
  selfRotationAngles[2] += tempVec.v[2];
  const arVector3 transVec = wandDirection * forwardSpeed;

  updateSelfPosition(transVec, rotationMatrix);
  updateSelfPositionTimes(1./fps);
}

void postExchange(arMasterSlaveFramework& fw) {
  if (!fw.getMaster()) {
    const ARfloat fpsRaw = 1000./fw.getLastFrameTime();
    static ARfloat fpsPrev = 40.;
    fps = fpsRaw * .05 + fpsPrev * .95;
    fpsPrev = fps;
  }
  if (!self || !scene)
    return;

  s_updateValues uv;
  uv.velocity = velocity;//movingBackwards?-velocity:velocity;
  uv.lightspeed = lightspeed;
  uv.gamma = gamm(velocity,lightspeed);
  uv.selfPosition = selfPosition;
  uv.selfPositionTimes = selfPositionTimes;
  uv.howFarBack = HOW_FAR_BACK;
  uv.selfRotation = arMatrix4();	// always pointing forward
  uv.selfOffset = arVector3(0,0,0);	// no offset yet
  
  self->updateAll( uv);

  uv.selfRotation = selfRotationMatrix();
  uv.selfOffset = arVector3(selfOffset);

  scene->updateAll(uv);
}

void showRasterString(int x, int y, char* s){
  glRasterPos2f(x, y);
  char c;
  for (; (c=*s) != '\0'; ++s)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

void draw(arMasterSlaveFramework& fw) {
  if (!fw.getConnected()) {
    glClearColor(0,0,0,0);
    return;
  }

  if (!self || !scene)
    return;

  scene->initTheGL();
  fw.loadNavMatrix();
 
  glPushMatrix(); 
  glTranslatef(0,0,-5);

  self->drawAll();
  scene->drawAll();
  drawSelfPositionLine(); 

  glPopMatrix();

  // draw info screen/ front wall
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0,100,0,100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glColor3f(1,1,1);
  drawBouncyLight(fps);
  glColor3f(.5,1,1);
  char fpsString[64];
  sprintf(fpsString, "FPS: %4.0f, vel: %.3fm/s, c: %.3fm/s, gamma: %.4f, (%s)",
	  fps, velocity, lightspeed, gamm(velocity, lightspeed),
	  theMode==MOVING ? "Moving" : "Not Moving");
  showRasterString(1,1,fpsString);

#ifdef DEBUGGING_ONLY
  sprintf(fpsString, "selfOffset: %f, %f, %f",
          selfOffset[0], selfOffset[1], selfOffset[2]);
  showRasterString(1,6,fpsString);
  sprintf(fpsString, "selfRotation: %f, %f, %f",
          selfRotationAngles[0], selfRotationAngles[1], selfRotationAngles[2]);
  showRasterString(1,11,fpsString);
  
  glColor3f(0,1,0);
  glBegin(GL_POINTS);
  for (unsigned int i=0; i<HOW_FAR_BACK; i++)
    glVertex2f(99.-10.*selfPositionTimes[i], 100.*(float)i/(float)HOW_FAR_BACK);
  glEnd();
#endif
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setPostExchangeCallback(postExchange);
  framework.setDrawCallback(draw);
  framework.setClipPlanes( nearclip, farclip );
  framework.setEyeSpacing(6./(12.*2.54));

  // now, initialize the application
  unsigned int i;
  selfPosition = new ARfloat[HOW_FAR_BACK*3];
  for (i=0; i<HOW_FAR_BACK*3; i++)
    selfPosition[i] = 0.;
  selfPositionTimes = new ARfloat[HOW_FAR_BACK];
  for (i=0; i<HOW_FAR_BACK; i++)
    selfPositionTimes[i] = 0.;
  selfOffset[0] = selfOffset[1] = selfOffset[2] = 0;

  worldTransformID = dgTransform("world","root",worldMatrix);
  scene = new SpaceJunk();
  self = new Self();
  return framework.start() ? 0 : 1;
}
