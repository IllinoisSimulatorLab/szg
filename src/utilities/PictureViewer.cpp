//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include <stdio.h>
#include <stdlib.h>
#include "arMasterSlaveFramework.h"
#include "arGlutRenderFuncs.h"
#include "arLargeImage.h"
#include "arTexture.h"

const float FLIP_SECONDS = 2;
const float TEST_RED = .25;
const float STRIP_HEIGHT = 10;

class PictureApp: public arMasterSlaveFramework {
  public:
    PictureApp();
    virtual bool onStart( arSZGClient& SZGClient );
//    virtual void onWindowStartGL( arGUIWindowInfo* );
    virtual void onPreExchange( void );
    virtual void onPostExchange( void );
//    virtual void onWindowInit( void );
    virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
//    virtual void onDisconnectDraw( void );
//    virtual void onPlay( void );
    virtual void onWindowEvent( arGUIWindowInfo* );
//    virtual void onCleanup( void );
//    virtual void onOverlay( void );
//    virtual void onKey( unsigned char key, int x, int y );
//    virtual void onKey( arGUIKeyInfo* );
//    virtual void onMouse( arGUIMouseInfo* );

    void drawSyncTest();
    void showCenteredImage();

    int doSyncTest;
    string pictureFilename;

  private:
    arTexture _texture;
    arLargeImage _largeImage;
    int _screenWidth;
    int _screenHeight;
    int _flipped;
    arTimer _timer;
};

PictureApp::PictureApp() :
  arMasterSlaveFramework(),
  doSyncTest(0),
  pictureFilename("NULL"),
  _screenWidth(-1),
  _screenHeight(-1),
  _flipped(0) {
}

// onStart callback method (called in arMasterSlaveFramework::start()
//
// Note: DO NOT do OpenGL initialization here; this is now called
// __before__ window creation. Do it in the onWindowStartGL()
//
bool PictureApp::onStart( arSZGClient& szgClient ) {
  const string dataPath = szgClient.getDataPath();
  if (!_texture.readPPM( pictureFilename, dataPath )) {
    ar_log_error() << "PictureViewer failed to read picture file '"
                 << pictureFilename << "' from path '" << dataPath << "'.\n";
    return false;
  }
  // NOTE: we _do_ need to keep the arTexture.
  _largeImage.setImage(_texture);

  // Register shared memory.
  //  framework.addTransferField( char* name, void* address, arDataType type, int numElements ); e.g.
  addTransferField("doSyncTest", &doSyncTest, AR_INT, 1);
  addTransferField("_flipped", &_flipped, AR_INT, 1);
  return true;
}

// Called before data is transferred from master to slaves.
// Only called on the master.
// Processes user input or random variables.
void PictureApp::onPreExchange() {
  if (_timer.done()) {
    _flipped = !_flipped;
    _timer.start( FLIP_SECONDS*1e6 );
  }
}

// Called after transfer of data from master to slaves.
// Synchronizes slaves with master based on transferred data.
void PictureApp::onPostExchange() {
  // Do stuff after slaves got data and are again in sync with the master.
  if (!getMaster()) {
    // Nothing to do.
  }
}

// Catch reshape events,
// dealing with the fallout from the GLUT->arGUI conversion.
// Behavior implemented here is the default.
void PictureApp::onWindowEvent( arGUIWindowInfo* winInfo ) {
  // The values are defined in src/graphics/arGUIDefines.h.
  // arGUIWindowInfo is in arGUIInfo.h
  // The window manager is in arGUIWindowManager.h
  if (winInfo->getState() == AR_WINDOW_RESIZE) {
    const int windowID = winInfo->getWindowID();
#ifdef UNUSED
    const int x = winInfo->getPosX();
    const int y = winInfo->getPosY();
#endif
    _screenWidth = winInfo->getSizeX();
    _screenHeight = winInfo->getSizeY();
    getWindowManager()->setWindowViewport( windowID, 0, 0, _screenWidth, _screenHeight );
  }
}

void PictureApp::onDraw( arGraphicsWindow& /*win*/, arViewport& /*vp*/ ) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, _screenWidth, 0, _screenHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (doSyncTest) {
    drawSyncTest();
  }
  showCenteredImage();
}

void PictureApp::showCenteredImage() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5, 0.5, -0.5, 0.5, 0, 10);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,0,2, 0,0,0, 0,1,0);
  _largeImage.draw();
}

void PictureApp::drawSyncTest() {
  glColorMask( GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE );
  glColor3f(_flipped ? TEST_RED : 0, 0, 0);
  glBegin( GL_QUADS );
    glVertex2f( 0, 0 );
    glVertex2f( _screenWidth, 0 );
    glVertex2f( _screenWidth, STRIP_HEIGHT );
    glVertex2f( 0, STRIP_HEIGHT );
  glEnd();
  glColor3f(_flipped ? 0 : TEST_RED, 0, 0);
  glBegin( GL_QUADS );
    glVertex2f( 0, STRIP_HEIGHT );
    glVertex2f( _screenWidth, STRIP_HEIGHT );
    glVertex2f( _screenWidth, 2*STRIP_HEIGHT );
    glVertex2f( 0, 2*STRIP_HEIGHT );
  glEnd();
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}

int main(int argc, char** argv) {
  PictureApp framework;
  if (!framework.init(argc, argv)) {
    return 1;
  }

  if (argc < 2 || argc > 3) {
    ar_log_critical() << "usage: PictureViewer filename [synctest]\n";
    return 1;
  }

  framework.pictureFilename = string( argv[1] );
  framework.doSyncTest = argc==3;
  // Never returns unless something goes wrong
  return framework.start() ? 0 : 1;
}
