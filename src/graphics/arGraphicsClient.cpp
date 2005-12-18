//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsClient.h"

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*){
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientDisconnectCallback(void* client){
  // Frame-locking for the wildcat boards cannot be disabled
  // here, since this is not in the graphics thread but in the connection
  // thread!
  arGraphicsClient* c = (arGraphicsClient*) client;
  // cout << "arGraphicsClient remark: disconnected from server.\n";
  // We should *delete* the bundle path information. This
  // is really unique to each connection. This information
  // lets an application have its textures elsewhere than 
  // on the texture path.
  c->setDataBundlePath("NULL","NULL");
  c->reset();
  // Do not call skipConsumption here! That is done in the
  // arSyncDataClient proper.
  return true;
}

// Draw a particular viewing frustum, not the whole arGraphicsWindow.
// Called from the arGraphicsClientRenderCallback object.
// (Use the camera for view frustum culling.)
void ar_graphicsClientDraw( arGraphicsClient* c, arCamera* camera) {

  glEnable(GL_DEPTH_TEST);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  glColor3f(1.0,1.0,1.0);
  if (c->_overrideColor[0] == -1){
    // Set the view transform. Use the
    // viewer node in the database, if available, to provide
    // the additional info the screen object needs to calculate
    // the view transform
    c->_graphicsDatabase.activateLights();
    arMatrix4 projectionCullMatrix(camera->getProjectionMatrix());
    c->_graphicsDatabase.draw(&projectionCullMatrix);
  }
  else{
    // colored background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1,1,-1,1,0,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glColor3fv(&c->_overrideColor[0]);
    glBegin(GL_QUADS);
    glVertex3f(1,1,-0.5);
    glVertex3f(-1,1,-0.5);
    glVertex3f(-1,-1,-0.5);
    glVertex3f(1,-1,-0.5);
    glEnd();
  }
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientConsumptionCallback(void* client, ARchar* buf){
  if (!((arGraphicsClient*)client)->_graphicsDatabase.handleDataQueue(buf)) {
    cerr << "arGraphicsClient error: failed to consume buffer.\n";
    return false;
  }
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientActionCallback(void* client){
  arGraphicsClient* c = (arGraphicsClient*) client;

  // Hack for the wildcat graphics cards.
  c->getWindowManager()->activateFramelock();

  c->updateHead();

  // Draw all windows (simultaneously if threading is turned on),
  // blocking until all complete.
  c->_windowManager->drawAllWindows(true);
  return true;
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientNullCallback(void* client){
  arGraphicsClient* c = (arGraphicsClient*) client;

  // Disable framelock now, so that it can be appropriately
  // re-enabled upon reconnection to the master. This occurs here
  // because it is called in the same thread as the graphics (critical)
  // and is called when a disconnect occurs.
  c->getWindowManager()->deactivateFramelock();

  // Have everything drawn black.
  c->setOverrideColor(arVector3(0,0,0));

  // Draw all windows (simultaneously if threading is turned on),
  // blocking until all complete.
  c->_windowManager->drawAllWindows(true);

  // Return to normal drawing mode.
  c->setOverrideColor(arVector3(-1,-1,-1));

  // Throttle CPU when screen is blank.
  ar_usleep(40000);
  return ar_graphicsClientPostSyncCallback(client);
}

// Callback registered with the arSyncDataClient.
bool ar_graphicsClientPostSyncCallback(void* client){
  arGraphicsClient* c = (arGraphicsClient*) client;
  c->_windowManager->swapAllWindowBuffers(true);
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
      _client( &cli ) {}
    ~arGraphicsClientRenderCallback() {}
    void operator()(arGraphicsWindow&, arViewport&);
    void operator()(arGUIWindowInfo*) {}
    void operator()(arGUIWindowInfo* windowInfo,
                    arGraphicsWindow* graphicsWindow);
  private:
    arGraphicsClient* _client;
};

// The callback for the arGraphicsWindow (i.e. for an individual viewport).
void arGraphicsClientRenderCallback::operator()( arGraphicsWindow&,
                                                 arViewport& v) {
  if (_client)
    ar_graphicsClientDraw(_client, v.getCamera());
}

// The callback for the arGUIWindow.
void arGraphicsClientRenderCallback::operator()(arGUIWindowInfo* windowInfo,
                                                arGraphicsWindow* graphicsWindow){
  if (!_client){
    return;
  }
  // Draws all the stuff in the window.
  graphicsWindow->draw();

  // Some additional graphical widgets might need to be drawn, but only
  // do this for the "main" window.
  if( _client->getWindowManager()->isFirstWindow( windowInfo->getWindowID() ) ) {
    // A HACK for drawing the simulator interface. This is used in
    // standalone mode on the arDistSceneGraphFramework.
    if (_client->_simulator && _client->_showSimulator){
      _client->_simulator->drawWithComposition();
    }
    // Used for the performance display.
    if (_client->_drawFrameworkObjects){
      for (list<arFrameworkObject*>::iterator i
             = _client->_frameworkObjects.begin();
	   i != _client->_frameworkObjects.end(); i++){
        (*i)->drawWithComposition();
      }
    }
  }

  // All draw commands for this window have now been issued. Make sure that
  // these commands have written to the frame buffer before proceeding.

  // It seems like glFlush/glFinish are a little bit unreliable... not
  // every vendor has done a good job of implementing these.
  // Consequently, we do a small pixel read to force drawing to complete
  char buffer[32];
  glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,buffer);
  // It *also* seems like it's helpful to throw a glFinish() in for good
  // measure....
  glFinish();

  // Deal with a potential screenshot here in the drawing thread.
  // (NOTE: only screenshot the main window)
  if( _client->getWindowManager()->isFirstWindow( windowInfo->getWindowID() ) ) {
    bool stereo
      = _client->getWindowManager()->isStereo(windowInfo->getWindowID());
    if (_client->screenshotRequested()){
      _client->takeScreenshot(stereo);
    }
  }
}

arGraphicsClient::arGraphicsClient() :
  _windowManager(NULL),
  _guiParser(NULL),
  _overrideColor(-1,-1,-1),
  _simulator(NULL),
  _showSimulator(true),
  _drawFrameworkObjects(false),
  _screenshotPath(""),
  _screenshotX(0),
  _screenshotY(0),
  _screenshotWidth(1),
  _screenshotHeight(1),
  _doScreenshot(false),
  _whichScreenshot(0){

  _cliSync.setConnectionCallback(ar_graphicsClientConnectionCallback);
  _cliSync.setDisconnectCallback(ar_graphicsClientDisconnectCallback);
  _cliSync.setConsumptionCallback(ar_graphicsClientConsumptionCallback);
  _cliSync.setActionCallback(ar_graphicsClientActionCallback);
  _cliSync.setNullCallback(ar_graphicsClientNullCallback);
  _cliSync.setPostSyncCallback(ar_graphicsClientPostSyncCallback);
  _cliSync.setBondedObject(this);
}

arGraphicsClient::~arGraphicsClient(){
}

/// Get the configuration parameters from the Syzygy database and set-up the
/// object.
bool arGraphicsClient::configure(arSZGClient* client){
  if (!client){
    return false;
  }

  // Configure the window manager, do the initial window parsing.
  if (!_guiParser){
    _guiParser = new arGUIXMLParser(client);
  }
  const string screenName( client->getMode( "graphics" ) );
  string whichDisplay
    = "SZG_DISPLAY" + screenName.substr( screenName.length() - 1, 1 );
  std::string displayName  = client->getAttribute( whichDisplay, "name" );
  std::cout << "Using display: " << whichDisplay << " : "
            << displayName << std::endl;
  _guiParser->setConfig( client->getGlobalAttribute(displayName) );

  if (_guiParser->parse() < 0){
    cout << "szgrender remark: failed to parse the XML configuration.\n";
  }

  setTexturePath(client->getAttribute("SZG_RENDER", "texture_path"));
  // where to look for text textures
  string textPath = client->getAttribute("SZG_RENDER","text_path");
  ar_pathAddSlash(textPath);
  loadAlphabet(textPath.c_str());
  return true;
}

bool arGraphicsClient::updateHead() {
  arHead* head = _graphicsDatabase.getHead();
  if (!head) {
    cerr << "arGraphicsClient error: failed to update head.\n";
    return false;
  }
  _defaultHead = *head;
  return true;
}

void arGraphicsClient::loadAlphabet(const char* thePath){
  _graphicsDatabase.loadAlphabet(thePath);
}

void arGraphicsClient::setTexturePath(const string& thePath){
  _graphicsDatabase.setTexturePath(thePath);
}

void arGraphicsClient::setDataBundlePath(const string& bundlePathName,
                                    const string& bundleSubDirectory){
  _graphicsDatabase.setDataBundlePath(bundlePathName, bundleSubDirectory);
}

void arGraphicsClient::addDataBundlePathMap(const string& bundlePathName,
                                    const string& bundlePath){
  _graphicsDatabase.addDataBundlePathMap(bundlePathName, bundlePath);
}

/// Sets the networks on which this object will try to connect to a server, in
/// descending order of preference
void arGraphicsClient::setNetworks(string networks){
  _cliSync.setNetworks(networks);
}

bool arGraphicsClient::start(arSZGClient& client, bool startSynchronization){
  // In the case of standalone mode in the arDistSceneGraphFramework, we
  // do not want the synchronization to begin, only the windowing.
  if (startSynchronization){
    _cliSync.setServiceName("SZG_GEOMETRY");
    if (!(_cliSync.init(client) && _cliSync.start())){
      return false;
    }
  }

  std::vector< arGUIXMLWindowConstruct* >* windowConstructs =
    _guiParser->getWindowingConstruct()->getWindowConstructs();

  // populate the callbacks for both the gui and graphics windows and the head
  // for any vr cameras
  std::vector< arGUIXMLWindowConstruct*>::iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); itr++ ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arGraphicsClientWindowInitCallback() );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arGraphicsClientRenderCallback(*this) );
    (*itr)->setGUIDrawCallback( new arGraphicsClientRenderCallback(*this) );

    std::vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    std::vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &_defaultHead );
       }
    }
  }

  // Start the various window threads going.
  if( _windowManager->createWindows( _guiParser->getWindowingConstruct() ) < 0 ) {
    // An error occured.
    return false;
  }

  // Everything is OK.
  return true;
}

void arGraphicsClient::setOverrideColor(arVector3 overrideColor){
  _overrideColor = overrideColor;
}

void arGraphicsClient::requestScreenshot(const string& path,
                                         int x, int y, int width, int height){
  _screenshotPath = path;
  _screenshotX = x;
  _screenshotY = y;
  _screenshotWidth = width;
  _screenshotHeight = height;
  _doScreenshot = true;
}

bool arGraphicsClient::screenshotRequested(){
  return _doScreenshot;
}

void arGraphicsClient::takeScreenshot(bool stereo){
  char numberBuffer[5];
  sprintf(numberBuffer,"%i",_whichScreenshot);
  string screenshotName = string("screenshot")+numberBuffer+string(".jpg");
  char* buffer1 = new char[_screenshotWidth*_screenshotHeight*3];
  if (!stereo){
    glReadBuffer(GL_FRONT);
  }
  else{
    glReadBuffer(GL_FRONT_LEFT);
  }
  glReadPixels(_screenshotX, _screenshotY,
               _screenshotWidth, _screenshotHeight,
               GL_RGB,GL_UNSIGNED_BYTE, buffer1);
  arTexture* texture = new arTexture;
  texture->setPixels(buffer1,_screenshotWidth,_screenshotHeight);
  if (!texture->writeJPEG(screenshotName.c_str(),_screenshotPath)){
    cerr << "szgrender remark: failed to write screenshot.\n";
  }
  delete texture;
  delete buffer1;
  // don't forget to increment the screenshot number
  _whichScreenshot++;
  _doScreenshot = false;
}

