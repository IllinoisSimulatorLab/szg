//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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

bool setUpWorld(arMasterSlaveFramework&, arSZGClient&){
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

void drawCallback(arMasterSlaveFramework& fw){
  const arMatrix4 headMatrix = fw.getMatrix(0);
  const arMatrix4 wandMatrix = fw.getMatrix(1);
  const float caveJoystickX = fw.getAxis(0);
  const float caveJoystickY = fw.getAxis(1);
  const int caveButton[3] =
    { fw.getButton(0), fw.getButton(1), fw.getButton(2) };

  // draw wireframe cubes -- these should not translate, as head translates!
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

  // draw the head display, to ensure orientation is correct!
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

  // draw the wand display
  glPushMatrix();
    glMultMatrixf(wandMatrix.v);
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

  glLineWidth(1);

  const arMatrix4 m(ar_extractTranslationMatrix(wandMatrix));
  glMultMatrixf(m.v);
  // one cube for the wand's joystick, in a square
  glColor3f(0.3,1,0);
  glPushMatrix();
    glTranslatef(0.5 ,-0.6, 0);
    glScalef(.3, .3, .3);
    glBegin(GL_LINE_LOOP);
      glVertex3f( 1,  1, 0);
      glVertex3f( 1, -1, 0);
      glVertex3f(-1, -1, 0);
      glVertex3f(-1,  1, 0);
    glEnd();
    glPushMatrix();
      glTranslatef(caveJoystickX, caveJoystickY, 0);
      glutSolidCube(0.35);
    glPopMatrix();
  glPopMatrix();
  // three cubes for the three wand buttons
  glPushMatrix();
    glTranslatef(0.1,-0.2,0);
    if (caveButton[0] == 0)
      glColor3f(1,0,0);
    else
      glColor3f(0,1,0);
    glutSolidCube(0.1);
  glPopMatrix();
  glPushMatrix();
    glTranslatef(0.5,-0.2,0);
    if (caveButton[1] == 0)
      glColor3f(1,0,0);
    else
      glColor3f(0,1,0);
    glutSolidCube(0.1);
  glPopMatrix();
  glPushMatrix();
    glTranslatef(0.9,-0.2,0);
    if (caveButton[2] == 0)
      glColor3f(1,0,0);
    else
      glColor3f(0,1,0);
    glutSolidCube(0.1);
  glPopMatrix();
  glEnable(GL_LIGHTING);

  // and finally, draw the framework itself.
  fw.draw();
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setInitCallback(setUpWorld);
  framework.setDrawCallback(drawCallback);
  framework.setClipPlanes( .1, 300. );
  return framework.start() ? 0 : 1;
}
