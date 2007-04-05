//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGlut.h"
#include "arMasterSlaveFramework.h"
#include "arMath.h"

void drawWand(const arMatrix4& m, const float large = 1.0) {
  glPushMatrix();
    if (large != 1.0)
      glScalef(large, large, large); // larger for visibility
    glMultMatrixf(m.v);
    glBegin(GL_LINES);
      glColor3f(1,1,0);
        glVertex3f(0,0,1);
        glVertex3f(0,0,-1);
      glColor3f(0,1,1);
        glVertex3f(1,0,0);
        glVertex3f(-1,0,0);
      glColor3f(1,0,1);
        glVertex3f(0,1,0);
        glVertex3f(0,-1,0);
    glEnd();
    glPushMatrix();
      glTranslatef(0,0,-1);
      glColor3f(1,0,1);
      glutSolidSphere(0.07,10,10);
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

// copied from arInputSimulator::_drawHead()
// bug: glDisable(GL_DEPTH_TEST) destroys pupil-eyeball-head occlusion
//      but it's needed for drawing flat on the front wall.
// ?workaround: glScalef(1,1,.001)  ;;;;
void drawHead() {
  glPushMatrix();
    glScalef(1.5,1.5,1.5); // larger for visibility
    glColor3f(1,1,0);
    glutWireSphere(1,10,10);
    // two eyes
    glColor3f(0,1,1);
    glPushMatrix();
      glTranslatef(0.5,0,-0.8);
      glutSolidSphere(0.4,8,8);
      glTranslatef(0,0,-0.4);
      glColor3f(1,0,0);
      glutSolidSphere(0.15,5,5);
    glPopMatrix();
      glTranslatef(-0.5,0,-0.8);
      glColor3f(0,1,1);
      glutSolidSphere(0.4,8,8);
      glTranslatef(0,0,-0.4);
      glColor3f(1,0,0);
      glutSolidSphere(0.15,5,5);
  glPopMatrix();
}

static bool fComplained = false;
void callbackDraw(arMasterSlaveFramework& fw, arGraphicsWindow& gw, arViewport&){
  unsigned cm = fw.getNumberMatrices();
  if (!fComplained && cm < 2)
    ar_log_warning() << "cubevars: expect at least a head and wand matrix.\n";
  const unsigned cmMax = 10;
  if (!fComplained && cm > cmMax) {
    cm = cmMax;
    ar_log_warning() << "cubevars: too many matrices.\n";
  }
  unsigned i;
  arMatrix4 rgm[cmMax];
  for (i=0; i<cm; ++i)
    rgm[i] = fw.getMatrix(i);
  const arMatrix4& headMatrix = rgm[0];
  const arMatrix4& wandMatrix = rgm[1];

  const unsigned ca = fw.getNumberAxes();
  if (!fComplained && ca != 2)
    ar_log_warning() << "cubevars expected 2 axes.\n";
  const float caveJoystickX = fw.getAxis(0);
  const float caveJoystickY = fw.getAxis(1);

  unsigned cb = fw.getNumberButtons();
  // wall-dependent.  Camille doesn't know why, 2006-06-12.
  // ?workaround -- only master calls this, in preexchange.

  if (!fComplained && cb < 3)
    ar_log_warning() << "cubevars expected at least 3 buttons.\n";
  const unsigned cbMax = 250;
  if (!fComplained && cb > cbMax) {
    cb = cbMax;
    ar_log_warning() << "cubevars: too many buttons.\n";
  }
  int rgButton[cbMax+1];
  for (i=0; i < cb; ++i)
    rgButton[i] = fw.getButton(i);

  fComplained = true;

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
  glColor3f( 1, 1, 1 );

  glutPrintf(0,  2, -5, "Front");
  glutPrintf(-5, 5,  0, "Left",  -90);
  glutPrintf( 5, 5,  0, "Right",  90);
  glutPrintf(0,  5,  5, "Back",  180);
  glutPrintf(0,  10,  0, "Ceiling", 0, 90);
  glutPrintf(0,  0,  0, "Floor", 0, -90);

  const int iEye = 1 + int(gw.getCurrentEyeSign());
  glColor3f(.2,.8,0);

  glutEyeglasses( .7, 0.5+.5,  -5, iEye, 0, 0);
  glutEyeglasses( -5, 3.5+.5, -.7, iEye, -90, 0);
  glutEyeglasses(  5, 3.5+.5,  .7, iEye,  90, 0);
  glutEyeglasses(-.7, 3.5+.5,   5, iEye, 180, 0);
  glutEyeglasses( .7, 10,   -1-.5, iEye, 0, 90);
  glutEyeglasses( .7, 0,     1+.5, iEye, 0, -90);

  // Maya-style cross sections, pasted to front wall.
  glutPrintf(-4, 8.2, -5, "top" ); // xz
  glutPrintf(-1, 8.2, -5, "side"); // yz
  glutPrintf( 2, 8.2, -5, "back"); // xy

  glDisable(GL_DEPTH_TEST);

  // top view
  glPushMatrix();
    glTranslatef(-3, 7, -5);
    glScalef(1, 1, 0); // flatten onto front screen

    glColor3f(.1,.1,.4);
    glBegin(GL_POLYGON);
      glVertex3f( 1,  1, 0);
      glVertex3f( 1, -1, 0);
      glVertex3f(-1, -1, 0);
      glVertex3f(-1,  1, 0);
    glEnd();

    glPushMatrix();
      glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
      glRotatef(90, 1,0,0); // rotate back view to top view: 90 degrees about x axis
      glTranslatef(0, -5, 0); // correct y coord
      for (i=1; i<cm; ++i)
	drawWand(rgm[i], 1.5);
      glMultMatrixf(headMatrix.v);
      drawHead();
    glPopMatrix();
  glPopMatrix();

  // side view
  glPushMatrix();
    glTranslatef(0, 7, -5);
    glScalef(1, 1, 0); // flatten onto front screen
    glColor3f(.1,.1,.4);
    glBegin(GL_POLYGON);
      glVertex3f( 1,  1, 0);
      glVertex3f( 1, -1, 0);
      glVertex3f(-1, -1, 0);
      glVertex3f(-1,  1, 0);
    glEnd();
    glPushMatrix();
      glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
      glRotatef(-90, 0,1,0); // rotate back view to side view: 90 degrees about y axis
      glTranslatef(0, -5, 0); // correct y coord
      for (i=1; i<cm; ++i)
	drawWand(rgm[i], 1.5);
      glMultMatrixf(headMatrix.v);
      drawHead();
    glPopMatrix();
  glPopMatrix();

  // back view
  glPushMatrix();
    glTranslatef(3, 7, -5);
    glScalef(1, 1, 0); // flatten onto front screen
    glColor3f(.1,.1,.4);
    glBegin(GL_POLYGON);
      glVertex3f( 1,  1, 0);
      glVertex3f( 1, -1, 0);
      glVertex3f(-1, -1, 0);
      glVertex3f(-1,  1, 0);
    glEnd();
    glPushMatrix();
      glScalef(.2, .2, .2); // shrink 10 feet to 2 feet
      glTranslatef(0, -5, 0); // correct y coord
      for (i=1; i<cm; ++i)
	drawWand(rgm[i], 1.5);
      glMultMatrixf(headMatrix.v);
      drawHead();
    glPopMatrix();
  glPopMatrix();

  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glLineWidth(3);

  // Head display, to verify orientation.
  glPushMatrix();
    glMultMatrixf(headMatrix.v);
    glTranslatef(0,0,-2); // in front of your eyes
    glBegin(GL_LINES);
      glColor3f(1,1,0);
        glVertex3f(0,0,1);
        glVertex3f(0,0,-1);
      glColor3f(0,1,1);
        glVertex3f(1,0,0);
        glVertex3f(-1,0,0);
      glColor3f(1,0,1);
        glVertex3f(0,1,0);
        glVertex3f(0,-1,0);
    glEnd();
    glPushMatrix();
      glTranslatef(0,0,-1);
      glColor3f(0,1,1);
      glutSolidSphere(0.07,10,10);
    glPopMatrix();
  glPopMatrix();

  // Wand(s)
  for (i=1; i<cm; ++i)
    drawWand(rgm[i]);

  glLineWidth(1);

  const arMatrix4 m(ar_extractTranslationMatrix(wandMatrix));
  glMultMatrixf(m.v);

  glPushMatrix();
    glTranslatef(0.5 ,0.0, -1.5);

    // Box for each wand button
    // Released, small red.  Pushed, big green.
    for (i=0; i<cb; ++i) {
      glPushMatrix();
	const float x = .15 + 1.2 * (i / (cb - 1.0));
	glTranslatef(x, -0.2, 0);
	if (rgButton[i] == 0) {
	  glColor3f(1, .3, .2);
	  glutSolidCube(0.5 / cb);
	}
	else {
	  glColor3f(.3, 1, .6);
	  glutSolidCube(0.7 / cb);
	}
      glPopMatrix();
    }

    // Box for the wand's joystick, in a square.
    glLineWidth(3);
    glTranslatef(0.0 ,0.2, 0.0);
    glScalef(.2, .2, .2);
    glColor3f(1,1,.3);
    glBegin(GL_LINE_LOOP);
      glVertex3f( 1,  1, 0);
      glVertex3f( 1, -1, 0);
      glVertex3f(-1, -1, 0);
      glVertex3f(-1,  1, 0);
    glEnd();
    glBegin(GL_LINES);
      glVertex3f( 0,  1, 0);
      glVertex3f( 0, -1, 0);
      glVertex3f( 1,  0, 0);
      glVertex3f(-1,  0, 0);
    glEnd();
    glColor3f(0.3,.9,0);
    glPushMatrix();
      glTranslatef(caveJoystickX, caveJoystickY, 0);
      glutSolidCube(0.35);
    glPopMatrix();

  glPopMatrix();
}

int main(int argc, char** argv){
  arMasterSlaveFramework fw;
  fw.setDrawCallback(callbackDraw);
  fw.setClipPlanes(.2, 20.);
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
