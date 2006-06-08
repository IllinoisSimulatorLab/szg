//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#include "arCubeEnvironment.h"
#include "arMath.h"
#ifndef AR_USE_WIN_32
#include <sys/types.h>
#include <signal.h>
#endif

int billboardID = -1;

bool callbackStart(arMasterSlaveFramework&, arSZGClient&){
  arCubeEnvironment bigCube;
  bigCube.setHeight(20);
  bigCube.setRadius(14.14);
  bigCube.setOrigin(0,5,0);
  bigCube.setNumberWalls(6);

  bigCube.setWallTexture(0,"Nature1.ppm");
  bigCube.setWallTexture(1,"Nature2.ppm");
  bigCube.setWallTexture(2,"Nature3.ppm");
  bigCube.setWallTexture(3,"Nature1.ppm");
  bigCube.setWallTexture(4,"Nature2.ppm");
  bigCube.setWallTexture(5,"Nature3.ppm");
  bigCube.setWallTexture(6,"Satin.ppm");
  bigCube.setWallTexture(7,"Leopard.ppm");
  bigCube.attachMesh("room","root");

  // Attach a text display.
  dgTransform("billboard transform","root",
              ar_translationMatrix(0,0,-4)*ar_rotationMatrix('y',3.14159));
  billboardID = dgBillboard("billboard","billboard transform",1, "cubevars");
  return true;
}

void drawWand(const arMatrix4& m) {
  // Wand display
  glPushMatrix();
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

void callbackDraw(arMasterSlaveFramework& fw){
  int i;

  arMatrix4 rgm[20]; // buffer overflow
  const unsigned cm = fw.getNumberMatrices();
  if (cm < 2)
    cerr << "warning: expect at least a head and wand matrix.\n";
  if (cm > 20)
    cerr << "warning: too many matrices.\n";
  for (i=0; i<cm; ++i)
    rgm[i] = fw.getMatrix(i);
  const arMatrix4& headMatrix = rgm[0];
  const arMatrix4& wandMatrix = rgm[1];

  const unsigned ca = fw.getNumberAxes();
  const float caveJoystickX = fw.getAxis(0);
  const float caveJoystickY = fw.getAxis(1);

  const unsigned cb = fw.getNumberButtons();
  if (cb < 3)
    cerr << "warning: expect at least 3 buttons.\n";
  if (cb > 250)
    cerr << "warning: too many buttons.\n";
  int rgButton[257];
  for (i=0; i < cb; ++i)
    rgButton[i] = fw.getButton(i);


  // Wireframe cubes -- these should not translate, as head translates.
  glDisable(GL_LIGHTING);
  glColor3f(1,1,1);
  glPushMatrix();
    glTranslatef(0,5,0);
    glutWireCube(9.5);
    glutWireCube(10.0);
    glutWireCube(10.5);
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

  // Box for the wand's joystick, in a square.
  glPushMatrix();
    glTranslatef(0.5 ,-0.6, 0);
    glScalef(.3, .3, .3);
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

  // Box for each wand button
  for (i=0; i<cb; ++i) {
    glPushMatrix();
      const float x = 0.1 + 0.8 * (i / (cb - 1.0));
      glTranslatef(x, -0.2, 0);
      if (rgButton[i] == 0)
	glColor3f(1,0,0);
      else
	glColor3f(0,1,0);
      glutSolidCube(0.5 / cb);
    glPopMatrix();
  }

  glEnable(GL_LIGHTING);
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(callbackStart);
  framework.setDrawCallback(callbackDraw);
  framework.setClipPlanes( .1, 300. );
  return framework.start() ? 0 : 1;
}
