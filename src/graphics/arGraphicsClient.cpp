//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsClient.h"
#include "arWildcatUtilities.h"

bool ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*){
  return true;
}

bool ar_graphicsClientDisconnectCallback(void* client){
  // Note that frame-locking for the wildcat boards cannot be disabled
  // here, since this is not in the graphics thread but in the connection
  // thread!
  arGraphicsClient* c = (arGraphicsClient*) client;
  // cout << "arGraphicsClient remark: disconnected from server.\n";
  // We should go ahead and *delete* the bundle path information. This
  // is really unique to each connection. This information is used to
  // let an application have its textures in a flexible location (i.e.
  // NOT on the texture path).
  c->setDataBundlePath("NULL","NULL");
  c->reset();
  // NOTE: DO NOT CALL skipConsumption from here! That is done in the
  // arSyncDataClient proper.
  return true;
}

void ar_graphicsClientShowRasterString(int x, int y, char* s){
  glRasterPos2f(x, y);
  char c;
  for (; (c=*s) != '\0'; ++s)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

void ar_graphicsClientDraw( arGraphicsClient* c ) {
  // if we are over-riding the default, do it here and then return
  if (c->_drawFunction){
    c->_drawFunction(&c->_graphicsDatabase);
    return;
  }

  glEnable(GL_DEPTH_TEST);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  
  glColor3f(1.0,1.0,1.0);
  if (c->_overrideColor[0] == -1){
    // this is where the view transform is set. We use the
    // viewer node in the database, if available, to provide
    // the additional info the screen object needs to calculate
    // the view transform
    c->_graphicsDatabase.activateLights();
    c->_graphicsDatabase.draw();
  }
  else{
    // we just want a colored background
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

  // performance display
  if (c->_showFramerate){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,3000,0,3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor3f(1., 1., 0.);
    char buf[100];
    sprintf(buf, "%4.0f fps (ideal %4.0f fps)",
            1000000.0 / c->_cliSync.getFrameTime(),
            1000000.0 / c->_cliSync.getActionTime());
    ar_graphicsClientShowRasterString(100, 200, buf);
    sprintf(buf, "Source/Sink %5.2f / %5.2f Mbps",
	    c->_cliSync.getServerSendSize()*8.0 / c->_cliSync.getFrameTime(),
            c->_cliSync.getRecvSize()*8.0 / c->_cliSync.getFrameTime());
    ar_graphicsClientShowRasterString(100, 100, buf);
  }
}

bool ar_graphicsClientConsumptionCallback(void* client,
					  ARchar* buf){
  if (!((arGraphicsClient*)client)->_graphicsDatabase.handleDataQueue(buf)) {
    cerr << "arGraphicsClient error: failed to consume buffer.\n";
    return false;
  }
  return true;
}

bool ar_graphicsClientActionCallback(void* client){
  arGraphicsClient* c = (arGraphicsClient*) client;
  
  // A HACK for the wildcat cards! glFinish does not work appropriately 
  // there... so framelock needs to be enabled.
  ar_activateWildcatFramelock();

  c->updateHead();
  c->_graphicsWindow.draw();

  // A HACK for drawing the simulator interface. This is used in
  // standalone mode on the arDistSceneGraphFramework.
  if (c->_simulator){
    c->_simulator->drawWithComposition();
  }
  if (c->_drawFrameworkObjects){
    for (list<arFrameworkObject*>::iterator i = c->_frameworkObjects.begin();
	 i != c->_frameworkObjects.end(); i++){
      (*i)->drawWithComposition();
    }
  }

  // it seems like glFlush/glFinish are a little bit unreliable... not
  // every vendor has done a good job of implementing these. 
  // Consequently, we do a small pixel read to force drawing to complete
  char buffer[32];
  glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,buffer);

  return true;
}

bool ar_graphicsClientNullCallback(void* client){
  arGraphicsClient* c = (arGraphicsClient*) client;
  
  // we need to disable framelock now, so that it can be appropriately 
  // re-enabled upon reconnection to the master. This occurs here
  // because it is called in the same thread as the graphics (critical)
  // and is called when a disconnect occurs
  ar_deactivateWildcatFramelock();

  if (c->_stereoMode){
    glDrawBuffer(GL_BACK_LEFT);
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_BACK_RIGHT);
  }
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  // let's not spin the CPU out-of-control
  ar_usleep(40000);
  return ar_graphicsClientPostSyncCallback(client);
}

bool ar_graphicsClientPostSyncCallback(void*){
  glutSwapBuffers();
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
  private:
};  
void arGraphicsClientWindowInitCallback::operator()( arGraphicsWindow& ) {
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

class arGraphicsClientRenderCallback : public arRenderCallback {
  public:
    arGraphicsClientRenderCallback( arGraphicsClient& cli ) :
      _client( &cli ) {}
    ~arGraphicsClientRenderCallback() {}
    void operator()( arGraphicsWindow&, arViewport&);
  private:
    arGraphicsClient* _client;
};
void arGraphicsClientRenderCallback::operator()( arGraphicsWindow&, 
                                                 arViewport& v) {
  if (!_client)
    return;
  ar_graphicsClientDraw( _client );
}

arGraphicsClient::arGraphicsClient() :
  _drawFunction(NULL),
  _showFramerate(false),
  _stereoMode(false),
  _overrideColor(-1,-1,-1),
  _simulator(NULL),
  _drawFrameworkObjects(false) {

  _cliSync.setConnectionCallback(ar_graphicsClientConnectionCallback);
  _cliSync.setDisconnectCallback(ar_graphicsClientDisconnectCallback);
  _cliSync.setConsumptionCallback(ar_graphicsClientConsumptionCallback);
  _cliSync.setActionCallback(ar_graphicsClientActionCallback);
  _cliSync.setNullCallback(ar_graphicsClientNullCallback);
  _cliSync.setPostSyncCallback(ar_graphicsClientPostSyncCallback);
  _cliSync.setBondedObject(this);
  
  _graphicsWindow.setInitCallback( new arGraphicsClientWindowInitCallback );
  _graphicsWindow.setDrawCallback( new arGraphicsClientRenderCallback(*this) );

  _defaultCamera.setHead( &_defaultHead );
}

arGraphicsClient::~arGraphicsClient(){
}

/// Get the configuration parameters from the Syzygy database and set-up the
/// object.
bool arGraphicsClient::configure(arSZGClient* client){
  if (!client){
    return false;
  }

  _graphicsWindow.setCamera( &_defaultCamera );
  _graphicsWindow.configure(*client);

  setTexturePath(client->getAttribute("SZG_RENDER", "texture_path"));
  // where to look for text textures
  string textPath = client->getAttribute("SZG_RENDER","text_path");
  ar_pathAddSlash(textPath);
  loadAlphabet(textPath.c_str());
  return true;
}

void arGraphicsClient::init(){
  // this needs to occur after the graphics have been initted

  // need to try to find the Wildcat card's frame-locking functions here
  ar_findWildcatFramelock();
}

//void arGraphicsClient::monoEyeOffset( const string& eye ) {
//  _graphicsWindow.setDefaultEye( eye );
//}

//void arGraphicsClient::addViewport(int /*originX*/, int /*originY*/, 
//                                   int sizeX, int sizeY){
//   HACK: this doesn't even nearly do what it's supposed to do!
//   Really, there should be a linked list of viewports, to which this
//   one is added. This should, ideally be a very general function
//  _viewportXOffset = sizeX;
//  _viewportYOffset = sizeY;
//}

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

void arGraphicsClient::setStereoMode(bool f){
  _graphicsWindow.useOGLStereo(f);
  _stereoMode = f;
}

//void arGraphicsClient::setAnaglyphMode(bool f) {
//  _anaglyphMode = f;
//}

void arGraphicsClient::setViewMode( const std::string& viewMode ) {
  _graphicsWindow.setViewMode( viewMode );
}

void arGraphicsClient::setFixedHeadMode(bool f) {
  _fixedHeadMode = f;
}

void arGraphicsClient::showFramerate(bool f){
  _showFramerate = f;
}

void arGraphicsClient::setDrawFunction(void (*drawFunction)
				       (arGraphicsDatabase*)){
  _drawFunction = drawFunction;
}

/// Sets the networks on which this object will try to connect to a server, in 
/// descending order of preference
void arGraphicsClient::setNetworks(string networks){
  _cliSync.setNetworks(networks);
}

bool arGraphicsClient::start(arSZGClient& client){
  _cliSync.setServiceName("SZG_GEOMETRY");
  return _cliSync.init(client) && _cliSync.start();
}

void arGraphicsClient::setOverrideColor(arVector3 overrideColor){
  _overrideColor = overrideColor;
}

arCamera* arGraphicsClient::setWindowCamera(int cameraID){
  if (cameraID < 0) {
    return _graphicsWindow.setCamera( &_defaultCamera );
  }
  arCamera* cam = (arCamera*)_graphicsDatabase.getCamera( (unsigned int)cameraID );
  if (!cam) {
    cerr << "arGraphicsClient error: failed to set camera.\n";
    return 0;
  }
  return _graphicsWindow.setCamera( cam );
}

arCamera* arGraphicsClient::setViewportCamera(unsigned int vpid, int cameraID) {
  if (cameraID < 0) {
    return _graphicsWindow.setViewportCamera( vpid, &_defaultCamera );
  }
  arCamera* cam = (arCamera*)_graphicsDatabase.getCamera( (unsigned int)cameraID );
  if (!cam) {
    cerr << "arGraphicsClient error: failed to set camera.\n";
    return 0;
  }
  return _graphicsWindow.setViewportCamera( vpid, cam );
}

arCamera* arGraphicsClient::setStereoViewportsCamera(unsigned int vpid, int cameraID) {
  if (cameraID < 0) {
    return _graphicsWindow.setStereoViewportsCamera( vpid, &_defaultCamera );
  }
  arCamera* cam = (arCamera*)_graphicsDatabase.getCamera( (unsigned int)cameraID );
  if (!cam) {
    cerr << "arGraphicsClient error: failed to set camera.\n";
    return 0;
  }
  return _graphicsWindow.setStereoViewportsCamera( vpid, cam );
}

arCamera* arGraphicsClient::setWindowLocalCamera( const float* const frust, 
                                                  const float* const look ) {
  // The Irix compiler does not like constructs like: foo(&my_constructor(...)), thus
  // use a temporary variable. The setCamera* functions deep copy the camera pointer they are 
  // passed, so this is OK.
  arPerspectiveCamera temp( frust, look );
  return _graphicsWindow.setCamera( &temp);
}

arCamera* arGraphicsClient::setViewportLocalCamera( unsigned int vpid, 
                                                    const float* const frust, 
													const float* const look ) {
  // The Irix compiler does not like constructs like: foo(&my_constructor(...)), thus
  // use a temporary variable. The setCamera* functions deep copy the camera pointer they are 
  // passed, so this is OK.
  arPerspectiveCamera temp( frust, look );
  return _graphicsWindow.setViewportCamera( vpid, &temp);
}

arCamera* arGraphicsClient::setStereoViewportsLocalCamera( unsigned int vpid, 
                                                           const float* const frust, 
                                                           const float* const look ) {
  // The Irix compiler does not like constructs like: foo(&my_constructor(...)), thus
  // use a temporary variable. The setCamera* functions deep copy the camera pointer they are 
  // passed, so this is OK.
  arPerspectiveCamera temp( frust, look );													
  return _graphicsWindow.setStereoViewportsCamera( vpid, &temp );
}
//
//void arGraphicsClient::setLocalCamera(float* frustum, float* lookat){
//  _graphicsDatabase.setLocalCamera(frustum,lookat);
//}
