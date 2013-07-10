//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGlutRenderFuncs.h"
#include "arMasterSlaveFramework.h"

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

  glLineWidth(2.);
  glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0, -1, 0);
    glRotatef(rotX, 1,  0, 0);

    // nosepiece and temples
    glBegin(GL_LINES);
    glVertex3f(-xNose, 0, 0);
    glVertex3f( xNose, 0, 0);
    glVertex3f(-xEar, 0, 0);
    glVertex3f(-xEar, 0, dz);
    glVertex3f( xEar, 0, 0);
    glVertex3f( xEar, 0, dz);
    glEnd();

    // earpieces
    glPushMatrix();
      glTranslatef( xEar, -r, dz);
      glArc(0, 0, r);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(-xEar, -r, dz);
      glArc(0, 0, r);
    glPopMatrix();

    if (i < 2) {
      // left lens
      glPushMatrix();
        glTranslatef(-xPupil, 0, 0);
        glCircle(0, 0, r);
      glPopMatrix();
    }
    if (i > 0) {
      // right lens
      glPushMatrix();
        glTranslatef( xPupil, 0, 0);
        glCircle(0, 0, r);
      glPopMatrix();
    }
  glPopMatrix();
}

void glutPrintf(float x, float y, float z, const char* sz, float rotY=0., float rotX=0.) {
  glPushMatrix();
#ifdef using_bitmap_fonts
    glRasterPos3f(x, y, z);
    for (const char* c = sz; *c; ++c)
      ar_glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
#else
    // Thick huge font for dim projectors.
    glLineWidth(3.);
    glTranslatef(x, y, z);
    glScalef(.004, .004, .004);
    glRotatef(rotY, 0, -1, 0);
    glRotatef(rotX, 1,  0, 0);
    for (const char* c = sz; *c; ++c)
      ar_glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
#endif
  glPopMatrix();
}

// derived from arInputSimulator::_drawHead()
void drawHead() {
  glPushMatrix();
    glScalef(1.5, 1.5, 1.5); // larger for visibility
    glColor3f(.8, .8, 0);
    ar_glutSolidSphere(1, 16, 16);
    // two eyes
    glColor3f(0, 1, 1);
    glPushMatrix();
      glTranslatef(0.5, 0, -0.7);
      ar_glutSolidSphere(0.4, 8, 8);
      glTranslatef(0, 0, -0.34);
      glColor3f(1, 0, 0);
      ar_glutSolidSphere(0.15, 5, 5);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(-0.5, 0, -0.7);
      glColor3f(0, 1, 1);
      ar_glutSolidSphere(0.4, 8, 8);
      glTranslatef(0, 0, -0.34);
      glColor3f(1, 0, 0);
      ar_glutSolidSphere(0.15, 5, 5);
    glPopMatrix();
    // cap
    glColor3f( 1, 1, 0 );
    glPushMatrix();
      glTranslatef(-0., 1., -0.);
      glScalef( 2., .1, 2. );
      ar_glutSolidCube(0.4);
    glPopMatrix();
  glPopMatrix();
}

unsigned cButton = 0;
unsigned cAxis = 0;
unsigned cMatrix = 0;

vector<ARint> rgButton;
vector<ARfloat> rgAxis;
vector<ARint> rgfjoy32k;
vector<arMatrix4> rgMatrix; // [0] is head, rest are wands.
bool fWhitewalls = false;

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
  static float amplSaberPrev = 0.;

  const float xyz[5][3] = // left, front, right, default, saber
    { {-5, 4, 0}, {0, 4, -5}, {5, 4, 0}, {2.34, 0, 0}, {0, 5, -5} };
  const int iPingDefault = 3;
  const int iSaber = 4;
  if (iPing < 0 || iPing > 2) {
    iPing = iPingDefault;
  }

  static bool fInit = false;
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
  cButton = fw.getNumberButtons();
  cAxis = fw.getNumberAxes();
  cMatrix = fw.getNumberMatrices();

  bool fPing = false;
  bool fPong = false;
  unsigned i;
  int iPing = -1;
  int cOn = 0;
  rgButton.resize(cButton);
  for (i=0; i < cButton; ++i) {
    rgButton[i] = fw.getButton(i);
    const bool onButton = fw.getOnButton(i);
    fPing |= onButton;
    fPong |= fw.getOffButton(i);
    if (onButton) {
      iPing = int(i);
      ++cOn;
    }
  }
  // Mash several buttons *at once* to toggle white walls.
  if (cOn > 1) {
    fWhitewalls = !fWhitewalls;
  }
  assert(fw.setInternalTransferFieldSize("a", AR_INT, cButton));
  int unused;
  ARint* pIntDst = (ARint*)fw.getTransferField("a", AR_INT, unused);
  assert(pIntDst != NULL);
  std::copy(rgButton.begin(), rgButton.end(), pIntDst);

  rgAxis.resize(cAxis);
  rgfjoy32k.resize(cAxis);
  for (i=0; i<cAxis; ++i) {
    if (rgfjoy32k[i] |= fabs(rgAxis[i] = fw.getAxis(i)) > 16400.)
      rgAxis[i] /= 32768.;
    clamp(rgAxis[i], -M_PI, M_PI);
  }
  assert(fw.setInternalTransferFieldSize("b", AR_FLOAT, cAxis));
  assert(fw.setInternalTransferFieldSize("d", AR_INT, cAxis));
  ARfloat* pFloatDst = (ARfloat*)fw.getTransferField("b", AR_FLOAT, unused);
  assert(pFloatDst != NULL);
  std::copy(rgAxis.begin(), rgAxis.end(), pFloatDst);
  pIntDst = (ARint*)fw.getTransferField("d", AR_INT, unused);
  assert(pIntDst != NULL);
  std::copy(rgfjoy32k.begin(), rgfjoy32k.end(), pIntDst);

  rgMatrix.resize(cMatrix);
  if (cMatrix < 1)
    return;
  assert(fw.setInternalTransferFieldSize("c", AR_FLOAT, cMatrix*16));
  assert(sizeof(rgMatrix[0]) == 16 * sizeof(AR_FLOAT)); // so contiguity works for std::copy
  rgMatrix[0] = fw.getMidEyeMatrix();
  for (i=1; i<cMatrix; ++i) rgMatrix[i] = fw.getMatrix(i);
  arMatrix4* pMatrixDst = (arMatrix4*)fw.getTransferField("c", AR_FLOAT, unused);
  assert(pMatrixDst != NULL);
  std::copy(rgMatrix.begin(), rgMatrix.end(), pMatrixDst);

  if (cMatrix < 2)
    return;
  static arVector3 tipPosPrev(0, 5, -5);
  const  arVector3 tipPos(ar_ET(rgMatrix[1]) + (ar_ERM(rgMatrix[1]) * arVector3(0, 0, -wandConeLength)));
  float vSaber = (tipPos - tipPosPrev).magnitude() / 5.;
  clamp(vSaber, 0.0, 1.3);
  tipPosPrev = tipPos;
  doSounds(iPing, fPing, fPong, vSaber);
}

void callbackPostEx(arMasterSlaveFramework& fw) {
  if (fw.getMaster())
    return;
  int size;

  // update rgButton
  ARint* pIntSrc = (ARint*)fw.getTransferField("a", AR_INT, size);
  std::copy(pIntSrc, pIntSrc+size, rgButton.begin());
  cButton = size;

  // update rgAxis
  ARfloat* pFloatSrc = (ARfloat*)fw.getTransferField("b", AR_FLOAT, size);
  assert(pFloatSrc != NULL);
  rgAxis.resize(size);
  std::copy(pFloatSrc, pFloatSrc+size, rgAxis.begin());
  pIntSrc = (ARint*)fw.getTransferField("d", AR_INT, size);
  assert(pIntSrc != NULL);
  rgfjoy32k.resize(size);
  std::copy(pIntSrc, pIntSrc+size, rgfjoy32k.begin());
  cAxis = size;

  // update rgMatrix
  arMatrix4* pMatrixSrc = (arMatrix4*)fw.getTransferField("c", AR_FLOAT, size);
  assert(size % 16 == 0);
  size /= 16;
  rgMatrix.resize(size);
  std::copy(pMatrixSrc, pMatrixSrc+size, rgMatrix.begin());
  cMatrix = size;
}

void drawWand(const arMatrix4& m, const float large = 1.0) {
  glPushMatrix();
    glScalef(large, large, large);
    glMultMatrixf(m.v);
    glBegin(GL_LINES);
      glColor3f(0, 1, 1);
        glVertex3f(.25, 0, 0);
        glVertex3f(-.25, 0, 0);
        glVertex3f(0, .5, 0);
        glVertex3f(0, -.25, 0);
    glEnd();

    // Forward
    glPushMatrix();
      glTranslatef(0, 0, 1);
      glScalef(1, 1, -1);
      glColor3f(1, 0, 1);
      ar_glutWireCone(0.12, 2, 8, 2);
    glPopMatrix();

    // Up
    glPushMatrix();
      glTranslatef(0, .6, 0);
      if (fWhitewalls)
	glColor3f(.0, .0, .0);
      else
	glColor3f(.8, .8, .8);
      ar_glutWireSphere(0.07, 5, 5);
    glPopMatrix();

  glPopMatrix();
}

void bluesquare() {
  glColor3f(.1, .1, .4);
  glBegin(GL_POLYGON);
    glVertex3f( 1,  1, -.02);
    glVertex3f( 1, -1, -.02);
    glVertex3f(-1, -1, -.02);
    glVertex3f(-1,  1, -.02);
  glEnd();
  glScalef(1, 1, .001); // flatten onto front screen
}

void headwands() {
  glTranslatef(0, -5, 0); // correct y coord
  for (unsigned i=1; i<cMatrix; ++i)
    drawWand(rgMatrix[i], wandConeLength);
  if (cMatrix > 0) {
    glMultMatrixf(rgMatrix[0].v);
    drawHead();
  }
}

inline void drawSliderCube() {
  ar_glutSolidCube(0.37);
  glColor3f(0, 0, 0);
  ar_glutWireCube(0.42);
}

void callbackDraw(arMasterSlaveFramework&, arGraphicsWindow& gw, arViewport&) {

  if (fWhitewalls)
    glClearColor(1, 1, 1, 0);
  else
    glClearColor(0, 0, 0, 0);

  // Wireframe around edges of standard 10-foot cube.
  glDisable(GL_LIGHTING);
  glPushMatrix();
    glTranslatef(0, 5, 0);
    glColor3f(.9, .9, .9);
    ar_glutWireCube( 9.8);
    if (fWhitewalls)
      glColor3f(.0, .0, .0);
    else
      glColor3f(1, 1, 1);
    ar_glutWireCube(10.0);
    glColor3f(.9, .9, .9);
    ar_glutWireCube(10.2);
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
  glColor3f(.2, .8, 0);
  glLineWidth(3.);

  glutEyeglasses( .7, 0.5+.5,  -5, iEye, 0, 0);
  glutEyeglasses( -5, 3.5+.5, -.7, iEye, -90, 0);
  glutEyeglasses(  5, 3.5+.5,  .7, iEye,  90, 0);
  glutEyeglasses(-.7, 3.5+.5,   5, iEye, 180, 0);
  glutEyeglasses( .7, 10,   -1-.5, iEye, 0, 90);
  glutEyeglasses( .7, 0,     1+.5, iEye, 0, -90);

  if (cMatrix == 0 && cButton == 0) {
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
        glRotatef(90, 1, 0, 0); // rotate back view to top view: 90 degrees about x axis
        headwands();
      glPopMatrix();
    glPopMatrix();

    // side view
    glPushMatrix();
      glTranslatef(0, 7, -5);
      bluesquare();
      glPushMatrix();
        glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
        glRotatef(-90, 0, 1, 0); // rotate back view to side view: 90 degrees about y axis
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

  if (cMatrix > 0) {
    // Reticle to show head orientation.
    glLineWidth(2.);
    glPushMatrix();
      glMultMatrixf(rgMatrix[0].v);
      glTranslatef(0, 0, -3); // in front of your eyes
      glColor3f(.6, .3, .9);
      glScalef(4., 1., 9.); // ratios of the Monolith
      ar_glutWireCube(.3);
    glPopMatrix();

    // Head, projected onto all walls except front.
    // x.1 draws it behind other stuff, since it's opaque.
    arVector3 xyz(ar_ET(rgMatrix[0]));
    assert(!rgMatrix.empty());
    const arMatrix4 mRot(ar_ERM(rgMatrix[0]));

    // floor
    glPushMatrix();
      glTranslatef(xyz[0], -.1, xyz[2]); // project
      glScalef(1, 0.01, 1); // squash
      glScalef(.4, .4, .4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // ceiling
    glPushMatrix();
      glTranslatef(xyz[0], 10.1, xyz[2]); // project
      glScalef(1, 0.01, 1); // squash
      glScalef(.4, .4, .4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // left
    glPushMatrix();
      glTranslatef(-5.1, xyz[1], xyz[2]); // project
      glScalef(0.01, 1, 1); // squash
      glScalef(.4, .4, .4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // right
    glPushMatrix();
      glTranslatef(5.1, xyz[1], xyz[2]); // project
      glScalef(0.01, 1, 1); // squash
      glScalef(.4, .4, .4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();

    // back
    glPushMatrix();
      glTranslatef(xyz[0], xyz[1], 5.1); // project
      glScalef(1, 1, 0.01); // squash
      glScalef(.4, .4, .4);
      glMultMatrixf(mRot.v);
      drawHead();
    glPopMatrix();
  }

  glLineWidth(3.);
  unsigned i;
  for (i=1; i<cMatrix; ++i)
    drawWand(rgMatrix[i]);

  glLineWidth(1.);

  // Axes and buttons near first wand.
  if (cMatrix > 1)
    glMultMatrixf(ar_ETM(rgMatrix[1]).v);

  glPushMatrix();
    glTranslatef(0.7, -.2, -1.5);

    glPushMatrix();
      glTranslatef(-.7, -.2, 0);
      // Box for each wand button, released or depressed.
      const float step = (1.0 / (cButton-1));
      for (i=0; i<cButton; ++i) {
        glTranslatef(step, 0, 0);
        if (rgButton[i] == 0) {
          glColor3f(1, .3, .2);
          ar_glutSolidCube(0.5 / cButton);
          glColor3f(0, 0, 0);
          ar_glutWireCube(0.52 / cButton);
        }
        else {
          glColor3f(.3, 1, .6);
          ar_glutSolidCube(0.7 / cButton);
          glColor3f(0, 0, 0);
          ar_glutWireCube(0.72 / cButton);
        }
        // 5 buttons per line
        const int buttonsPerLine = 5;
        if ((i+1) % buttonsPerLine == 0) {
          // x = carriage return, y = line feed
          glTranslatef(-buttonsPerLine*step, -step, 0);
        }
      }
    glPopMatrix();

    if (cAxis >= 2) {
      // Box for the wand's joystick, in a square.
      const float joy=0.28;
      glLineWidth(3.);
      glTranslatef(0.0 , joy, 0.0);
      glScalef(joy, joy, joy);
      glColor3f(.8, .4, .3);
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
      glColor3f(.3, .9, 0);
      glPushMatrix();
        glTranslatef(rgAxis[0], rgAxis[1], 0);
        drawSliderCube();
      glPopMatrix();
    }

    // Slider for each axis.
    glTranslatef(1.5, 0, 0);
    glLineWidth(2.);
    for (i=0; i<cAxis; ++i) {
      if (rgfjoy32k[i]) {
        glColor3f(1, .7, 0);
        glutPrintf(0, 1.1, 0, "32K" );
      } else {
        glColor3f(.8, .4, .3);
      }
      glTranslatef(.5, 0, 0);
      glLineWidth(2.); // paranoid
      glBegin(GL_LINES);
        glVertex3f( 0, -1, 0);
        glVertex3f( 0,  1, 0);
      glEnd();
      glPushMatrix();
        glColor3f(.3, .9, 0);
        glTranslatef(0, rgAxis[i], 0);
        drawSliderCube();
      glPopMatrix();
    }

  glPopMatrix();
}

bool callbackStart(arMasterSlaveFramework& fw, arSZGClient&) {
  assert(fw.addInternalTransferField("a", AR_INT, 1));
  assert(fw.addInternalTransferField("b", AR_FLOAT, 1));
  assert(fw.addInternalTransferField("c", AR_FLOAT, 1));
  assert(fw.addInternalTransferField("d", AR_INT, 1));
  assert(fw.addTransferField("e", &fWhitewalls, AR_INT, 1));

  dsSpeak("_", "root", "Welcome to V R test.\n");
  return true;
}

int main(int argc, char** argv) {
  arMasterSlaveFramework fw;
  fw.setStartCallback(callbackStart);
  fw.setDrawCallback(callbackDraw);
  fw.setPreExchangeCallback(callbackPreEx);
  fw.setPostExchangeCallback(callbackPostEx);
  fw.setClipPlanes(.15, 20.);
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
