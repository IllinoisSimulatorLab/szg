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

void glutPrintf(float x, float y, float z, const char* sz, float rotY=0., float rotX=0.) {
  glPushMatrix();
#ifdef using_bitmap_fonts
    glRasterPos3f(x, y, z);
    for (const char* c = sz; *c; ++c)
      glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
#else
    // Thick huge font, easier to read on dim projector screens.
    glLineWidth(3);
    glTranslatef(x, y, z);
    glScalef(.0042, .0042, .0042);
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
      glutSolidSphere(0.1,8,8);
    glPopMatrix();
    glTranslatef(-0.5,0,-0.8);
    glColor3f(0,1,1);
    glutSolidSphere(0.4,8,8);
    glTranslatef(0,0,-0.4);
    glColor3f(1,0,0);
    glutSolidSphere(0.1,8,8);
  glPopMatrix();
}

void callbackDraw(arMasterSlaveFramework& fw, arGraphicsWindow& gw, arViewport&){
  unsigned i;

  arMatrix4 rgm[20]; // buffer overflow
  const unsigned cm = fw.getNumberMatrices();
  if (cm < 2)
    cerr << "cubevars warning: expect at least a head and wand matrix.\n";
  if (cm > 20)
    cerr << "cubevars warning: too many matrices.\n";
  for (i=0; i<cm; ++i)
    rgm[i] = fw.getMatrix(i);
  const arMatrix4& headMatrix = rgm[0];
  const arMatrix4& wandMatrix = rgm[1];

  const unsigned ca = fw.getNumberAxes();
  if (ca != 2)
    cerr << "cubevars warning: expect 2 axes.\n";
  const float caveJoystickX = fw.getAxis(0);
  const float caveJoystickY = fw.getAxis(1);

  const unsigned cb = fw.getNumberButtons();
  // wall-dependent.  Camille doesn't know why, 2006-06-12.
  // ?workaround -- only master calls this, in preexchange.

  if (cb < 3)
    cerr << "cubevars warning: expect at least 3 buttons.\n";
  if (cb > 250)
    cerr << "cubevars warning: too many buttons.\n";
  int rgButton[257];
  for (i=0; i < cb; ++i)
    rgButton[i] = fw.getButton(i);

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
  const int iEye = 1 + int(gw.getCurrentEyeSign());
  const char* rgszEye[3] = { "Left eye", "Both eyes", "Right eye" };
  const char* szEye = rgszEye[iEye];
  glColor3f( 1, 1, 1 );
    glutPrintf(0,  2, -5, "Front wall");
    glutPrintf(-5, 5,  0, "Left wall",  -90);
    glutPrintf( 5, 5,  0, "Right wall",  90);
    glutPrintf(0,  5,  5, "Back wall",  180);
    glutPrintf(0,  10,  0, "Top wall", 0, 90);
    glutPrintf(0,  0,  0, "Bottom wall", 0, -90);

    glutPrintf(0,  0.5 + iEye*.5, -5, szEye);
    glutPrintf(-5, 3.5 + iEye*.5,  0, szEye, -90);
    glutPrintf( 5, 3.5 + iEye*.5,  0, szEye,  90);
    glutPrintf(0,  3.5 + iEye*.5,  5, szEye, 180);
    glutPrintf(0,  10,   -1 - iEye*.5, szEye, 0, 90);
    glutPrintf(0,  0,     1 + iEye*.5, szEye, 0, -90);

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
  fw.setClipPlanes( .1, 200. );
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
