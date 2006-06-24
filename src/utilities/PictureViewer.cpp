//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arTexture.h"
#include "arLargeImage.h"
#include "arDataUtilities.h"

#include <math.h>

arTexture bitmap;
arLargeImage largeImage;
int screenWidth = -1;
int screenHeight = -1;

bool doSyncTest = false;
bool flipped = false;
arTimer timer;
const float FLIP_SECONDS = 2;
const float TEST_RED = .25;
const float STRIP_HEIGHT = 10;

void drawSyncTest() {
  glColorMask( GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE );
  glColor3f(flipped ? TEST_RED : 0, 0, 0);
  glBegin( GL_QUADS );
  glVertex2f( 0, 0 );
  glVertex2f( screenWidth, 0 );
  glVertex2f( screenWidth, STRIP_HEIGHT );
  glVertex2f( 0, STRIP_HEIGHT );
  glEnd();
  glColor3f(flipped ? 0 : TEST_RED, 0, 0);
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

void showCenteredImage() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,0.5,-0.5,0.5,0,10);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,0,2, 0,0,0, 0,1,0);
  largeImage.draw();
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
  if (doSyncTest) {
    drawSyncTest();
  }
  showCenteredImage();
  glutSwapBuffers();
}

void reshape(int width, int height) {
  screenWidth = width;
  screenHeight = height;
  glViewport(0,0,width,height);
}

int main(int argc, char** argv){
  // This exe is too different from other syzygy exes.

  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  ar_log().setStream(szgClient.initResponse());
  if ((argc < 2)||(argc > 3)){
    ar_log_error() << "usage: PictureViewer filename [synctest]\n";
    if (!szgClient.sendInitResponse(false))
      cerr << "PictureViewer error: maybe szgserver died.\n";
    return 1;
  }
  doSyncTest = (argc == 3);

  // Lock the screen.
  const string screenLock = szgClient.getComputerName() + string("/")
                      + szgClient.getMode("graphics");
  int graphicsID = -1;
  if (!szgClient.getLock(screenLock, graphicsID)){
    cout << "PictureViewer error: failed to get screen resource held "
	 << "by component "
         << graphicsID << ".\n(Kill that component to proceed.)\n";
    if (!szgClient.sendInitResponse(false))
      cerr << "PictureViewer error: maybe szgserver died.\n";
    return 1;
  }

  const string dataPath = szgClient.getDataPath();
  if (!bitmap.readPPM(argv[1], dataPath)){
    ar_log_error() << "PictureViewer failed to read picture file '"
                 << argv[1] << "' from path '" << dataPath << "'.\n";
    if (!szgClient.sendInitResponse(false))
      cerr << "PictureViewer error: maybe szgserver died.\n";
    return 1;
  }
  // Kluge: undo our PPM reader's horizontal flip.
  // todo: move that flip command into the PPM reader.
  if (!bitmap.flipHorizontal()) {
    ar_log_error() << "PictureViewer failed to flip picture '" << argv[1] << "'.\n";
    if (!szgClient.sendInitResponse(false))
      cerr << "PictureViewer error: maybe szgserver died.\n";
    return 1;
  }

  // Draw the "large image" instead of the texture.
  largeImage.setImage(bitmap);
  ar_log_remark() << "PictureViewer loaded picture.\n";
  if (!szgClient.sendInitResponse(true))
    cerr << "PictureViewer error: maybe szgserver died.\n";
  ar_log().setStream(szgClient.startResponse());

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

  ar_log_remark() << "Picture window visible.\n";
  if (!szgClient.sendStartResponse(true))
    cerr << "PictureViewer error: maybe szgserver died.\n";
  ar_log().setStream(cout);
  arThread dummy(ar_messageTask, &szgClient);
  timer.start( FLIP_SECONDS * 1e6 );
  glutMainLoop(); 
  return 0;
}
