//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"

#include "arGlut.h"
#include "arGraphicsHeader.h"
#include "arInputEvent.h"
#include "arInputNode.h"
#include "arNetInputSource.h"

arInputNode* inputDevice = NULL;
arNetInputSource* netSource = NULL;
arSZGClient* szgClient = NULL;
arVector3 positions[100] = {0};
arMatrix4 basePosition;
int numberEventsLogged = 0;
bool filled = false;

void eventCallback(arInputEvent& inputEvent) {
  if (inputEvent.getType() != AR_EVENT_BUTTON)
    return;

  if (inputEvent.getIndex() == 0 && inputEvent.getButton() == 1) {
    positions[numberEventsLogged] = inputDevice->getMatrix(1)*arVector3(0, 0, 0);
    numberEventsLogged++;
    if (numberEventsLogged > 100) {
      numberEventsLogged = 0;
      filled = true;
    }
  }
  if (inputEvent.getIndex() == 1 && inputEvent.getButton() == 1) {
    basePosition = ar_translationMatrix(0, 5, 0)*!inputDevice->getMatrix(1);
    int i;
    for (i=0; i<16; i++) {
      cout << basePosition[i] << "/";
    }
    cout << "\n";
    FILE* outputFile = fopen("temp-config.txt", "w");
    if (!outputFile) {
      cout << "DeskCalibrator: cannot write file.\n";
    }
    else{
      for (i=0; i<16; i++)
        fprintf(outputFile, "%f/", basePosition[i]);
      fprintf(outputFile, "\n");
      fclose(outputFile);
    }
  }
  if (inputEvent.getIndex() == 2 && inputEvent.getButton() == 1) {
    filled = false;
    numberEventsLogged = 0;
  }
}

void keyboard(unsigned char key, int /*x*/, int /*y*/) {
  switch( key ) {
    case 27: /* escape key */
      exit(0);
    break;

    case 'a':
      positions[ numberEventsLogged ] = inputDevice->getMatrix( 1 ) * arVector3( 0, 0, 0 );
      numberEventsLogged++;
      if ( numberEventsLogged > 100 ) {
        numberEventsLogged = 0;
        filled = true;
      }
      cout << "DeskCalibrator: event logged.\n";
    break;

    case 's':
      {
      basePosition = ar_translationMatrix( 0, 2.375, 0 ) * !inputDevice->getMatrix( 0 );
      int i;
      for( i = 0; i < 16; i++ )
        cout << basePosition[ i ] << "/";
      cout << "\n";
        FILE* outputFile = fopen("temp-config.txt", "w");
      if (!outputFile) {
        cout << "DeskCalibrator: cannot write file.\n";
      }
      else{
        for (i=0; i<16; i++)
          fprintf(outputFile, "%f/", basePosition[i]);
        fprintf(outputFile, "\n");
      }
      fclose(outputFile);
      }
    break;

    case 'd':
      filled = false;
      numberEventsLogged = 0;
      cout << "events cleared" << endl;
    break;
 }
}

void display() {
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-20, 20, -20, 20, 0.1, 100);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 5, 20, 0, 5, 4, 0, 1, 0);

  const arMatrix4 sensorMatrix0 = basePosition * inputDevice->getMatrix(0);
  const arMatrix4 sensorMatrix1 = basePosition * inputDevice->getMatrix(1);

  glPushMatrix();

    glColor3f(1, 1, 1);
    glTranslatef(0, 5, 0);
    glutWireCube(10);

  glPopMatrix();

  glPushMatrix();

    glMultMatrixf(sensorMatrix0.v);

    glPushMatrix();

      // right eye
      glTranslatef(1, 0, -1.5);
      glColor3f(1, 0, 0);
      glutSolidSphere(0.5, 16, 16);

    glPopMatrix();
    glPushMatrix();

      // left eye
      glTranslatef(-1, 0, -1.5);
      glColor3f(0, 1, 0);
      glutSolidSphere(0.5, 16, 16);

    glPopMatrix();
    glPushMatrix();

      // head
      glLineWidth( 1.0 );
      glColor3f(1, 1, 1);
      glutWireSphere(2, 10, 10);

    glPopMatrix();
  glPopMatrix();

  glPushMatrix();

    glMultMatrixf(sensorMatrix1.v);

    glColor3f(1, 1, 1);
    glutSolidSphere(0.1, 10, 10);

    glLineWidth( 1.5 );

    glBegin(GL_LINES);

      glColor3f(1, 0, 0);
      glVertex3f(0, 0, 0);
      glVertex3f(1, 0, 0);

      glColor3f(0, 1, 0);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 1, 0);

      glColor3f(0, 0, 1);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 0, 1.5);

      glColor3f(1, 1, 1);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 0, -1.5);

    glEnd();

    glPushMatrix();

      glTranslatef(1, 0, 0);
      glColor3f(1, 0, 0);
      glutSolidSphere(0.1, 10, 10);

    glPopMatrix();
    glPushMatrix();

      glTranslatef(0, 1, 0);
      glColor3f(0, 1, 0);
      glutSolidSphere(0.1, 10, 10);

    glPopMatrix();
    glPushMatrix();

      glTranslatef(0, 0, 1.5);
      glColor3f(0, 0, 1);
      glutSolidSphere(0.1, 10, 10);

    glPopMatrix();
  glPopMatrix();

  int limit = filled ? 100 : numberEventsLogged;
  for (int i=0; i < limit; i++) {
    glPushMatrix();
    arVector3 tempVector = basePosition*positions[i];
    glTranslatef(tempVector[0], tempVector[1], tempVector[2]);
    glColor3f(1, 0, 0);
    glutWireSphere(0.02, 10, 10);
    glPopMatrix();
  }

  glutSwapBuffers();
}

int main(int argc, char** argv) {
  szgClient = new arSZGClient();
  // ON AT LEAST ONE WINDOWS TEST SYSTEM, THESE MUST BE NEW'ED
  // INSTEAD OF DECLARED AS GLOBALS. WHAT'S WRONG????
  inputDevice = new arInputNode();
  netSource = new arNetInputSource();
  const bool fInit = szgClient->init(argc, argv);
  if (!*szgClient)
    return szgClient->failStandalone(fInit);

  // Connect to the primary input source.
  if (!netSource->setSlot(0))
    return 1;

  inputDevice->addInputSource(netSource, false);
  inputDevice->setEventCallback(eventCallback);
  if (!inputDevice->init(*szgClient))
    return 1;

  if (!inputDevice->start())
    return 1;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Calibrator");
  glutSetCursor(GLUT_CURSOR_NONE);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutMainLoop();
  return 0;
}
