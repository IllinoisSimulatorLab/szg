//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsClient.h"
#include "arLogStream.h"

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*) {
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientDisconnectCallback(void* client) {
  // Frame-locking for the wildcat boards cannot be disabled
  // here, since this is not in the graphics thread but in the connection
  // thread!
  arGraphicsClient* c = (arGraphicsClient*) client;
  ar_log_remark() << "arGraphicsClient disconnected.\n";
  // We should *delete* the bundle path information. This
  // is really unique to each connection. This information
  // lets an application have its textures elsewhere than
  // on the texture path.
  c->setDataBundlePath("NULL", "NULL");
  c->reset();
  // Call skipConsumption from the arSyncDataClient proper, not here.
  return true;
}

// Draw a particular viewing frustum, not the whole arGraphicsWindow.
// Called from the arGraphicsClientRenderCallback object.
// (Use the camera for view frustum culling.)
void ar_graphicsClientDraw( arGraphicsClient* c, arGraphicsWindow& win, arViewport& view ) {
  glEnable(GL_DEPTH_TEST);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  glColor3f(1.0, 1.0, 1.0);
  if (c->_overrideColor[0] == -1) {
    // Compute the view transform, from info in the database's viewer node.
    c->_graphicsDatabase.activateLights();
    c->_graphicsDatabase.draw(win, view);
  } else {
    // colored background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glColor3fv(&c->_overrideColor[0]);
    glBegin(GL_QUADS);
    glVertex3f(1, 1, -0.5);
    glVertex3f(-1, 1, -0.5);
    glVertex3f(-1, -1, -0.5);
    glVertex3f(1, -1, -0.5);
    glEnd();
  }
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientConsumptionCallback(void* client, ARchar* buf) {
  if (!((arGraphicsClient*)client)->_graphicsDatabase.handleDataQueue(buf)) {
    ar_log_error() << "arGraphicsClient failed to consume buffer.\n";
    return false;
  }
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientActionCallback(void* client) {
  arGraphicsClient* c = (arGraphicsClient*) client;
  c->getWindowManager()->activateFramelock();
  c->updateHead();
  c->drawAllWindows();
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientNullCallback(void* client) {
  arGraphicsClient* c = (arGraphicsClient*) client;

  // Disable framelock so it's properly
  // re-enabled upon reconnection to the master. Do this here
  // so it's in the same thread as the graphics (critical),
  // and is called upon disconnect.
  c->getWindowManager()->deactivateFramelock();

  // Pure black image.
  c->setOverrideColor(arVector3(0, 0, 0));
  c->drawAllWindows();

  // Revert to normal drawing mode.
  c->setOverrideColor(arVector3(-1, -1, -1));

  // Throttle CPU when screen is blank.
  ar_usleep(40000);
  return ar_graphicsClientPostSyncCallback(client);
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientPostSyncCallback(void* client) {
  ((arGraphicsClient*) client)->_windowManager->swapAllWindowBuffers(true);
  return true;
}

//***********************************************************************
// arGraphicsWindow callback classes
//***********************************************************************
class arGraphicsClientWindowInitCallback : public arWindowInitCallback {
  public:
    arGraphicsClientWindowInitCallback() {}
    ~arGraphicsClientWindowInitCallback() {}
    void operator()( arGraphicsWindow& );
};

void arGraphicsClientWindowInitCallback::operator()( arGraphicsWindow& ) {
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

class arGraphicsClientRenderCallback : public arGUIRenderCallback {
  public:
    arGraphicsClientRenderCallback( arGraphicsClient& cli ) :
      _cliGraphics( &cli ) {}
    ~arGraphicsClientRenderCallback() {}
    void operator()(arGraphicsWindow&, arViewport&);
    void operator()(arGUIWindowInfo*) {}
    void operator()(arGUIWindowInfo*, arGraphicsWindow*);
  private:
    arGraphicsClient* _cliGraphics;
};

// The callback for the arGraphicsWindow (i.e. for an individual viewport).
void arGraphicsClientRenderCallback::operator()( arGraphicsWindow& w,
                                                 arViewport& v) {
  if (_cliGraphics)
    ar_graphicsClientDraw(_cliGraphics, w, v);
}

// Callback for the arGUIWindow.
void arGraphicsClientRenderCallback::operator()(arGUIWindowInfo* wI,
                                                arGraphicsWindow* w) {
  if (_cliGraphics && wI && w)
    _cliGraphics->render(*wI, *w);
}

void arGraphicsClient::render(arGUIWindowInfo& wI, arGraphicsWindow& w) {
  // Window's contents.
  w.draw();

  // "Main" window's additional graphical widgets.
  arGUIWindowManager* wm = getWindowManager();
  const bool fFirst = wm->isFirstWindow( wI.getWindowID() );
  if ( fFirst ) {
    // HACK for the simulator interface, when standalone in arDistSceneGraphFramework.
    if (_simulator && _showSimulator) {
      _simulator->drawWithComposition();
    }
    // Performance graphs.
    if (_drawFrameworkObjects) {
      for (list<arFrameworkObject*>::iterator i = _frameworkObjects.begin();
           i != _frameworkObjects.end(); ++i) {
        (*i)->drawWithComposition();
      }
    }
  }

  // Wait for drawing to complete to the frame buffer.
  // Force this by reading a few pixels (some vendors' glFlush/glFinish are flaky).
  char buffer[32];
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  glFinish(); // paranoid

  // Drawing thread handles screenshots (of main window).
  if ( fFirst && screenshotRequested() )
    takeScreenshot( wm->isStereo(wI.getWindowID()));
}

arGraphicsClient::arGraphicsClient() :
  _windowManager(NULL),
  _guiParser(NULL),
  _overrideColor(-1, -1, -1),
  _simulator(NULL),
  _showSimulator(true),
  _drawFrameworkObjects(false),
  _screenshotPath(""),
  _screenshotX(0),
  _screenshotY(0),
  _screenshotWidth(1),
  _screenshotHeight(1),
  _doScreenshot(false),
  _whichScreenshot(0) {

  _cliSync.setConnectionCallback(ar_graphicsClientConnectionCallback);
  _cliSync.setDisconnectCallback(ar_graphicsClientDisconnectCallback);
  _cliSync.setConsumptionCallback(ar_graphicsClientConsumptionCallback);
  _cliSync.setActionCallback(ar_graphicsClientActionCallback);
  _cliSync.setNullCallback(ar_graphicsClientNullCallback);
  _cliSync.setPostSyncCallback(ar_graphicsClientPostSyncCallback);
  _cliSync.setBondedObject(this);
}

// Get configuration parameters from the Syzygy database.  Setup the object.
bool arGraphicsClient::configure(arSZGClient* szgClient) {
  if (!szgClient)
    return false;

  // Configure the window manager, do the initial window parsing.
  if (!_guiParser)
    _guiParser = new arGUIXMLParser(szgClient);

  const string screenName( szgClient->getMode( "graphics" ) );
  _guiParser->setConfig( szgClient->getDisplayName(
    "SZG_DISPLAY" + screenName.substr( screenName.length() - 1, 1 )));
  (void)_guiParser->parse();

  setTexturePath(szgClient->getAttribute("SZG_RENDER", "texture_path"));
  string textPath(szgClient->getAttribute("SZG_RENDER", "text_path"));
  ar_pathAddSlash(textPath);
  loadAlphabet(textPath.c_str());
  return true;
}

bool arGraphicsClient::updateHead() {
  arHead* head = _graphicsDatabase.getHead();
  if (!head) {
    ar_log_error() << "arGraphicsClient: no head to update.\n";
    return false;
  }

  _defaultHead = *head;
  return true;
}

void arGraphicsClient::loadAlphabet(const char* thePath) {
  _graphicsDatabase.loadAlphabet(thePath);
}

void arGraphicsClient::setTexturePath(const string& thePath) {
  _graphicsDatabase.setTexturePath(thePath);
}

void arGraphicsClient::setDataBundlePath(const string& bundlePathName,
                                    const string& bundleSubDirectory) {
  _graphicsDatabase.setDataBundlePath(bundlePathName, bundleSubDirectory);
}

void arGraphicsClient::addDataBundlePathMap(const string& bundlePathName,
                                    const string& bundlePath) {
  _graphicsDatabase.addDataBundlePathMap(bundlePathName, bundlePath);
}

// Define on which networks this object will try to connect to a server,
// in descending order of preference.
void arGraphicsClient::setNetworks(string networks) {
  _cliSync.setNetworks(networks);
}

bool arGraphicsClient::start(arSZGClient& client, bool startSynchronization) {
  // For standalone mode in arDistSceneGraphFramework,
  // start only windowing, not synchronization.
  if (startSynchronization) {
    _cliSync.setServiceName("SZG_GEOMETRY");
    if (!_cliSync.init(client) || !_cliSync.start()) {
      return false;
    }
  }
  // todo: _label = client.getComputerName() or something like that, for ar_log_xxx.

  std::vector< arGUIXMLWindowConstruct* >* windowConstructs =
    _guiParser->getWindowingConstruct()->getWindowConstructs();

  // Populate callbacks for gui and graphics windows and head for any vr cameras
  std::vector< arGUIXMLWindowConstruct*>::iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); itr++ ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arGraphicsClientWindowInitCallback() );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arGraphicsClientRenderCallback(*this) );
    (*itr)->setGUIDrawCallback( new arGraphicsClientRenderCallback(*this) );

    std::vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    std::vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if ( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &_defaultHead );
      }
    }
  }

  // Start the window threads.
  return _windowManager->createWindows(_guiParser->getWindowingConstruct()) >= 0;
}

void arGraphicsClient::setOverrideColor(arVector3 overrideColor) {
  _overrideColor = overrideColor;
}

void arGraphicsClient::requestScreenshot(const string& path,
                                         int x, int y, int width, int height) {
  _screenshotPath = path;
  _screenshotX = x;
  _screenshotY = y;
  _screenshotWidth = width;
  _screenshotHeight = height;
  _doScreenshot = true;
}

bool arGraphicsClient::screenshotRequested() {
  return _doScreenshot;
}

// copypaste with arMasterSlaveFramework::_handleScreenshot
void arGraphicsClient::takeScreenshot(bool stereo) {
  const string screenshotName = "screenshot" + ar_intToString(_whichScreenshot++) + ".jpg";
  char* buf = new char[_screenshotWidth*_screenshotHeight*3];
  glReadBuffer(stereo ? GL_FRONT_LEFT : GL_FRONT);
  glReadPixels(_screenshotX, _screenshotY, _screenshotWidth, _screenshotHeight,
               GL_RGB, GL_UNSIGNED_BYTE, buf);
  arTexture texture;
  texture.setPixels(buf, _screenshotWidth, _screenshotHeight);
  delete buf;
  if (!texture.writeJPEG(screenshotName.c_str(), _screenshotPath)) {
    ar_log_remark() << "arGraphicsClient failed to write screenshot.\n";
  }
  _doScreenshot = false;
}
