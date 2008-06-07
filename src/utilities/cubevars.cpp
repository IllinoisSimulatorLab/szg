//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGlut.h"
#include "arMasterSlaveFramework.h"

void drawWand(const arMatrix4& m, const float large = 1.0) {
  glPushMatrix();
    glScalef(large, large, large);
    glMultMatrixf(m.v);
    glBegin(GL_LINES);
      glColor3f(0,1,1);
        glVertex3f(.25,0,0);
        glVertex3f(-.25,0,0);
        glVertex3f(0,.5,0);
        glVertex3f(0,-.25,0);
    glEnd();

    // Forward
    glPushMatrix();
      glTranslatef(0,0,1);
      glScalef(1,1,-1);
      glColor3f(1,0,1);
      glutWireCone(0.12,2,8,2);
    glPopMatrix();

    // Up
    glPushMatrix();
      glTranslatef(0,.6,0);
      glColor3f(.8,.8,.8);
      glutWireSphere(0.07,5,5);
    glPopMatrix();

  glPopMatrix();
}

void glArc(const float y, const float z, const float r) {
  glBegin(GL_LINE_STRIP);
  const float cStep = 60;
  for (float a=0.; a<M_PI*.75; a+=2*M_PI/cStep)
    glVertex3f(0., y + r * cos(a), z + r * sin(a));
  glEnd();
}

void glCircle(const float x, const float y, const float r) {
  glBegin(GL_LINE_LOOP);
  const float cStep = 60;
  for (float a=0.; a<2*M_PI; a+=2*M_PI/cStep)
    glVertex3f(x + r * cos(a), y + r * sin(a), 0.);
  glEnd();
}

void glutEyeglasses(float x, float y, float z, const int i,
    const float rotY, const float rotX) {
  // i==0/1/2: left/both/right eye.

  const float dz = 1.25;
  const float r = 0.24;
  const float xNose = 0.04;
  const float xPupil = xNose + r;
  const float xEar = xNose + 2*r;

  glLineWidth(2);
  glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0, -1, 0);
    glRotatef(rotX, 1,  0, 0);

    // nosepiece and temples
    glBegin(GL_LINES);
    glVertex3f(-xNose,0,0);
    glVertex3f( xNose,0,0);
    glVertex3f(-xEar,0,0);
    glVertex3f(-xEar,0,dz);
    glVertex3f( xEar,0,0);
    glVertex3f( xEar,0,dz);
    glEnd();

    // earpieces
    glPushMatrix();
      glTranslatef( xEar,-r,dz);
      glArc(0,0,r);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(-xEar,-r,dz);
      glArc(0,0,r);
    glPopMatrix();

    if (i < 2) {
      // left lens
      glPushMatrix();
	glTranslatef(-xPupil,0,0);
	glCircle(0,0,r);
      glPopMatrix();
    }
    if (i > 0) {
      // right lens
      glPushMatrix();
	glTranslatef( xPupil,0,0);
	glCircle(0,0,r);
      glPopMatrix();
    }
  glPopMatrix();
}

void glutPrintf(float x, float y, float z, const char* sz, float rotY=0., float rotX=0.) {
  glPushMatrix();
#ifdef using_bitmap_fonts
    glRasterPos3f(x, y, z);
    for (const char* c = sz; *c; ++c)
      glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
#else
    // Thick huge font for dim projectors.
    glLineWidth(3);
    glTranslatef(x, y, z);
    glScalef(.004, .004, .004);
    glRotatef(rotY, 0, -1, 0);
    glRotatef(rotX, 1,  0, 0);
    for (const char* c = sz; *c; ++c)
      glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
#endif
  glPopMatrix();
}

// derived from arInputSimulator::_drawHead()
void drawHead() {
  glPushMatrix();
    glScalef(1.5,1.5,1.5); // larger for visibility
    glColor3f(.8,.8,0);
    glutSolidSphere(1,16,16);
    // two eyes
    glColor3f(0,1,1);
    glPushMatrix();
      glTranslatef(0.5,0,-0.7);
      glutSolidSphere(0.4,8,8);
      glTranslatef(0,0,-0.34);
      glColor3f(1,0,0);
      glutSolidSphere(0.15,5,5);
    glPopMatrix();
      glTranslatef(-0.5,0,-0.7);
      glColor3f(0,1,1);
      glutSolidSphere(0.4,8,8);
      glTranslatef(0,0,-0.34);
      glColor3f(1,0,0);
      glutSolidSphere(0.15,5,5);
  glPopMatrix();
}

const unsigned cmMax = 8;	// IS-900 reports this many tracked sensors.
const unsigned caMax = 20;
const unsigned cbMax = 30;

unsigned cSig[3] = {0};
unsigned& cb = cSig[0];
unsigned& ca = cSig[1];
unsigned& cm = cSig[2];

ARint rgButton   [cbMax+1] = {0};
ARint rgOnbutton [cbMax+1] = {0}; // technically unused, while rendered only as sound
ARint rgOffbutton[cbMax+1] = {0}; // technically unused, while rendered only as sound
ARfloat rgAxis[caMax] = {0};
bool rgfjoy32k[caMax] = {0};
bool fWhitewalls = false;
arMatrix4 rgm[cmMax]; // rgm[0] is head, rest are wands.

inline void clamp(ARfloat& a, const ARfloat aMin, const ARfloat aMax) {
  if (a > aMax)
    a = aMax;
  if (a < aMin)
    a = aMin;
}

void doSounds(int iPing, bool fPing, bool fPong, float amplSaber) {
  static int idPing = -1;
  static int idPong = -1;
  static int idSaber = -1;
  static bool fInit = false;
  static float amplSaberPrev = 0.;

  const float xyz[5][3] = // left, front, right, default, saber
    { {-5,4,0}, {0,4,-5}, {5,4,0}, {2.34,0,0}, {0,5,-5} };
  const int iPingDefault = 3;
  const int iSaber = 4;
  if (iPing < 0 || iPing > 2) {
    iPing = iPingDefault;
  }
  if (!fInit) {
    fInit = true;
    idPing = dsLoop("ping", "root", "q33beep.wav", 0, 0.0, xyz[iPingDefault]);
    idPong = dsLoop("pong", "root", "q33collision.wav", 0, 0.0, xyz[iPingDefault]);

    // Use .wav not .mp3, to avoid a click when the loop wraps around.
    idSaber = dsLoop("lightsaber", "root", "parade.wav", 1, 0.0, xyz[iSaber]);
  }

  static bool fResetPing = true;
  fPing &= idPing >= 0;
  if (fPing) {
    dsLoop(idPing, "q33beep.wav", -1, .3, xyz[iPing]);
    fResetPing = false;
  }
  else if (!fResetPing) {
    dsLoop(idPing, "q33beep.wav", 0, 0, xyz[iPing]);
    fResetPing = true;
  }

  static bool fResetPong = true;
  fPong &= idPong >= 0;
  if (fPong) {
    dsLoop(idPong, "q33collision.wav", -1, .2, xyz[iPingDefault]);
    fResetPong = false;
  }
  else if (!fResetPong) {
    dsLoop(idPong, "q33collision.wav", 0, 0, xyz[iPingDefault]);
    fResetPong = true;
  }

  // Slow decay.
  amplSaberPrev *= .95;
  if (amplSaber < amplSaberPrev)
    amplSaber = amplSaberPrev;
  amplSaberPrev = amplSaber;

  dsLoop(idSaber, "parade.wav", 1, amplSaber, xyz[iSaber]);

  // nyi: set idSaber's position
}

const float wandConeLength = 1.5;

void callbackPreEx(arMasterSlaveFramework& fw) {
  cb = fw.getNumberButtons();
  ca = fw.getNumberAxes();
  cm = fw.getNumberMatrices();

  static bool fComplained = false;
  if (!fComplained) {
    fComplained = true;
    if (cb > cbMax) {
      ar_log_error() << cb << " is too many buttons. Truncating to " << cbMax << ".\n";
      cb = cbMax;
    }
    if (ca > caMax) {
      ar_log_error() << ca << " is too many axes. Truncating to " << caMax << ".\n";
      ca = caMax;
    }
    if (cm > cmMax) {
      ar_log_error() << cm << " is too many matrices. Truncating to " << cmMax << ".\n";
      cm = cmMax;
    }
  }

  bool fPing = false;
  bool fPong = false;
  unsigned i;
  int iPing = -1;
  int cOn = 0;
  for (i=0; i < cb; ++i) {
    rgButton[i]    = fw.getButton(i);
    rgOnbutton[i]  = fw.getOnButton(i);
    rgOffbutton[i] = fw.getOffButton(i);
    if (rgOnbutton[i]) {
      iPing = int(i);
      ++cOn;
    }
    fPing |= rgOnbutton[i];
    fPong |= rgOffbutton[i];
  }
  // Mash several buttons *at once* to toggle white walls.
  if (cOn > 1) {
    fWhitewalls = !fWhitewalls;
  }

  for (i=0; i<ca; ++i) {
    if (rgfjoy32k[i] |= fabs(rgAxis[i] = fw.getAxis(i)) > 16400.)
      rgAxis[i] /= 32768.;
    clamp(rgAxis[i], -M_PI, M_PI);
  }
  rgm[0] = fw.getMidEyeMatrix();
  for (i=1; i<cm; ++i)
    rgm[i] = fw.getMatrix(i);

  static arVector3 tipPosPrev(0,5,-5);
  const arVector3 tipPos(ar_ET(rgm[1]) + (ar_ERM(rgm[1]) * arVector3(0,0,-wandConeLength)));
  float vSaber = (tipPos - tipPosPrev).magnitude() / 5.;
  clamp(vSaber, 0., .8);
  tipPosPrev = tipPos;
  doSounds(iPing, fPing, fPong, vSaber>.8 ? .8 : vSaber);
}

void bluesquare() {
  glColor3f(.1,.1,.4);
  glBegin(GL_POLYGON);
    glVertex3f( 1,  1, -.02);
    glVertex3f( 1, -1, -.02);
    glVertex3f(-1, -1, -.02);
    glVertex3f(-1,  1, -.02);
  glEnd();
  glScalef(1,1,.001); // flatten onto front screen
}

void headwands() {
  glTranslatef(0, -5, 0); // correct y coord
  for (unsigned i=1; i<cm; ++i)
    drawWand(rgm[i], wandConeLength);
  if (cm > 0) {
    glMultMatrixf(rgm[0].v);
    drawHead();
  }
}

inline void drawSliderCube() {
  glutSolidCube(0.37);
  glColor3f(0,0,0);
  glutWireCube(0.42);
}

void callbackDraw(arMasterSlaveFramework&, arGraphicsWindow& gw, arViewport&) {

  if (fWhitewalls)
    glClearColor(1,1,1,0);
  else
    glClearColor(0,0,0,0);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  // Wireframe around edges of standard 10-foot cube.
  glDisable(GL_LIGHTING);
  glPushMatrix();
    glTranslatef(0,5,0);
    glColor3f(.9, .9, .9);
    glutWireCube( 9.8);
    glColor3f(1, 1, 1);
    glutWireCube(10.0);
    glColor3f(.9, .9, .9);
    glutWireCube(10.2);
  glPopMatrix();

  // Labels on walls.
  glColor3f( .6, .6, .9 );

  glutPrintf(0,  2, -5, "Front");
  glutPrintf(-5, 5,  0, "Left",  -90);
  glutPrintf( 5, 5,  0, "Right",  90);
  glutPrintf(0,  5,  5, "Back",  180);
  glutPrintf(0,  10,  0, "Ceiling", 0, 90);
  glutPrintf(0,  0,  0, "Floor", 0, -90);

  const int iEye = 1 + int(gw.getCurrentEyeSign());
  glColor3f(.2,.8,0);
  glLineWidth(3);

  glutEyeglasses( .7, 0.5+.5,  -5, iEye, 0, 0);
  glutEyeglasses( -5, 3.5+.5, -.7, iEye, -90, 0);
  glutEyeglasses(  5, 3.5+.5,  .7, iEye,  90, 0);
  glutEyeglasses(-.7, 3.5+.5,   5, iEye, 180, 0);
  glutEyeglasses( .7, 10,   -1-.5, iEye, 0, 90);
  glutEyeglasses( .7, 0,     1+.5, iEye, 0, -90);

  if (cm == 0 && ca == 0 && cb == 0) {
    // Uninitialized.
    return;
  }

  // Maya-style cross sections, pasted to front wall.

  glColor3f( .4, .4, .8 );

    glutPrintf(-3.95, 7.45, -5.01, "Floor" ); // xz
    glutPrintf(-0.95, 7.45, -5.01, "Left"); // yz
    glutPrintf( 2.05, 7.45, -5.01, "Front"); // xy

    // top view
    glPushMatrix();
      glTranslatef(-3, 7, -5);
      bluesquare();
      glPushMatrix();
	glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
	glRotatef(90, 1,0,0); // rotate back view to top view: 90 degrees about x axis
	headwands();
      glPopMatrix();
    glPopMatrix();

    // side view
    glPushMatrix();
      glTranslatef(0, 7, -5);
      bluesquare();
      glPushMatrix();
	glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
	glRotatef(-90, 0,1,0); // rotate back view to side view: 90 degrees about y axis
	headwands();
      glPopMatrix();
    glPopMatrix();

    // back view
    glPushMatrix();
      glTranslatef(3, 7, -5);
      bluesquare();
      glPushMatrix();
	glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
	headwands();
      glPopMatrix();
    glPopMatrix();

  glDisable(GL_LIGHTING);

  if (cm > 0) {
    // Reticle to show head orientation.
    glLineWidth(2);
    glPushMatrix();
      glMultMatrixf(rgm[0].v);
      glTranslatef(0,0,-3); // in front of your eyes
      glColor3f(.6,.3,.9);
      glScalef(4.,1.,9.); // ratios of the Monolith
      glutWireCube(.3);
    glPopMatrix();

    // Head, projected onto all walls except front.
    // x.1 draws it behind other stuff, since it's opaque.
    arVector3 xyz(ar_ET(rgm[0]));
    const arMatrix4 mRot(ar_ERM(rgm[0]));

    // floor
    glPushMatrix();
      glTranslatef(xyz[0], -.1, xyz[2]); // project
      glScalef(1, 0.01, 1); // squash
      glScalef(.4,.4,.4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // ceiling
    glPushMatrix();
      glTranslatef(xyz[0], 10.1, xyz[2]); // project
      glScalef(1, 0.01, 1); // squash
      glScalef(.4,.4,.4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // left
    glPushMatrix();
      glTranslatef(-5.1, xyz[1], xyz[2]); // project
      glScalef(0.01, 1, 1); // squash
      glScalef(.4,.4,.4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // right
    glPushMatrix();
      glTranslatef(5.1, xyz[1], xyz[2]); // project
      glScalef(0.01, 1, 1); // squash
      glScalef(.4,.4,.4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // back
    glPushMatrix();
      glTranslatef(xyz[0], xyz[1], 5.1); // project
      glScalef(1, 1, 0.01); // squash
      glScalef(.4,.4,.4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();
  }

  glLineWidth(3);
  unsigned i;
  for (i=1; i<cm; ++i)
    drawWand(rgm[i]);

  glLineWidth(1);

  // Axes and buttons near first wand.
  if (cm > 1)
    glMultMatrixf(ar_ETM(rgm[1]).v);

  glPushMatrix();
    glTranslatef(0.7, -.2, -1.5);

    glPushMatrix();
      glTranslatef(-.7, -.2, 0);
      // Box for each wand button, released or depressed.
      const float step = (1.0 / (cb-1));
      for (i=0; i<cb; ++i) {
	glTranslatef(step, 0, 0);
	if (rgButton[i] == 0) {
	  glColor3f(1, .3, .2);
	  glutSolidCube(0.5 / cb);
	  glColor3f(0,0,0);
	  glutWireCube(0.52 / cb);
	}
	else {
	  glColor3f(.3, 1, .6);
	  glutSolidCube(0.7 / cb);
	  glColor3f(0,0,0);
	  glutWireCube(0.72 / cb);
	}
	// 5 buttons per line
	const int buttonsPerLine = 5;
	if ((i+1) % buttonsPerLine == 0) {
	  // x = carriage return, y = line feed
	  glTranslatef(-buttonsPerLine*step, -step, 0);
	}
      }
    glPopMatrix();

    if (ca >= 2) {
      // Box for the wand's joystick, in a square.
      const float joy=0.28;
      glLineWidth(3);
      glTranslatef(0.0 ,joy, 0.0);
      glScalef(joy, joy, joy);
      glColor3f(.8,.4,.3);
      glBegin(GL_LINE_LOOP);
	glVertex3f( 1,  1, 0);
	glVertex3f( 1, -1, 0);
	glVertex3f(-1, -1, 0);
	glVertex3f(-1,  1, 0);
      glEnd();
      glBegin(GL_LINES);
	glVertex3f( 0, -1, 0);
	glVertex3f( 0,  1, 0);
	glVertex3f(-1,  0, 0);
	glVertex3f( 1,  0, 0);
      glEnd();
      glColor3f(.3,.9,0);
      glPushMatrix();
	glTranslatef(rgAxis[0], rgAxis[1], 0);
	drawSliderCube();
      glPopMatrix();
    }

    // Slider for each axis.
    glTranslatef(1.5, 0, 0);
    glLineWidth(2);
    for (i=0; i<ca; ++i) {
      if (rgfjoy32k[i]) {
	glColor3f(1,.7,0);
	glutPrintf(0, 1.1, 0, "32K" );
      } else {
	glColor3f(.8,.4,.3);
      }
      glTranslatef(.5, 0, 0);
      glLineWidth(2); // paranoid
      glBegin(GL_LINES);
	glVertex3f( 0, -1, 0);
	glVertex3f( 0,  1, 0);
      glEnd();
      glPushMatrix();
	glColor3f(.3,.9,0);
	glTranslatef(0, rgAxis[i], 0);
	drawSliderCube();
      glPopMatrix();
    }

  glPopMatrix();
}

bool callbackStart(arMasterSlaveFramework& fw, arSZGClient&) {
  fw.addTransferField("a", cSig, AR_INT, 3);
  fw.addTransferField("b", rgButton, AR_INT, sizeof(rgButton)/sizeof(ARint));
//fw.addTransferField("c", rgOnbutton, AR_INT, sizeof(rgOnbutton)/sizeof(ARint));
//fw.addTransferField("d", rgOffbutton, AR_INT, sizeof(rgOffbutton)/sizeof(ARint));
  fw.addTransferField("e", rgAxis, AR_FLOAT, sizeof(rgAxis)/sizeof(ARfloat));
  fw.addTransferField("f", rgm, AR_FLOAT, sizeof(rgm)/sizeof(ARfloat));
  fw.addTransferField("g", rgfjoy32k, AR_INT, sizeof(rgfjoy32k)/sizeof(ARint));
  fw.addTransferField("h", &fWhitewalls, AR_INT, 1);

  return true;
}

int main(int argc, char** argv){
  arMasterSlaveFramework fw;
  fw.setStartCallback(callbackStart);
  fw.setDrawCallback(callbackDraw);
  fw.setPreExchangeCallback(callbackPreEx);
  fw.setClipPlanes(.15, 20.);
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
