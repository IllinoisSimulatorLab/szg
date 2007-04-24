//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGraphicsUtilities.h"
#include "arGraphicsWindow.h"
#include "arPerspectiveCamera.h"

void ar_defaultWindowInitCallback() {
  glEnable(GL_DEPTH_TEST);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void arDefaultWindowInitCallback::operator()( arGraphicsWindow& ) {
  ar_defaultWindowInitCallback();
}

void arDefaultRenderCallback::operator()( arGraphicsWindow&, arViewport& ) {
  glClearColor( 0,0,0,1 );
}

arGraphicsWindow::arGraphicsWindow( arCamera* cam ) :
  _useOGLStereo(false),
  _useColorFilter(false),
  _defaultCamera(0),
  _currentEyeSign(0.),
  _posX(-1),
  _posY(-1),
  _sizeX(-1),
  _sizeY(-1) {

  // By default, the graphics window contains a single "normal" viewport
  // in its list (the default viewport corresponds to normal).
  _addViewportNoLock( arViewport() );

  _setCameraNoLock( cam );

  _initCallback = new arDefaultWindowInitCallback;
  _drawCallback = new arDefaultRenderCallback;

  // by default, we'll be using screen objects for our automatically
  // created configurations (i.e. from configure(...)). It is still
  // possible to put an arGraphicsWindow together in an arbitrary fashion.
}

//arGraphicsWindow::arGraphicsWindow( const arGraphicsWindow& x ) :
//  _useOGLStereo( x._useOGLStereo ),
//  _initCallback( x._initCallback ),
//  _drawCallback( x._drawCallback ),
//  _width( x._width ),
//  _height( x._height ) {
//}
//
//arGraphicsWindow& arGraphicsWindow::operator=( const arGraphicsWindow& x ) {
//  if (&x == this)
//    return *this;
//  _useOGLStereo = x._useOGLStereo;
//  _initCallback = x._initCallback;
//  _drawCallback = x. _drawCallback;
//  _width = x._width;
//  _height = x._height;
//  return *this;
//}

arGraphicsWindow::~arGraphicsWindow() {
  clearViewportList();
  if (_defaultCamera) {
    delete _defaultCamera;
  }
}

bool arGraphicsWindow::configure( arSZGClient& client ){
  float colorFilterParams[7];

  string screenName = client.getMode("graphics");
  _useColorFilter = client.getAttributeFloats( screenName, "color_filter", colorFilterParams, 7 );
  if (_useColorFilter) {
    _contrastFilterParameters = arVector4( colorFilterParams );
    _colorScaleFactors = arVector3( colorFilterParams+4 );
    cout << "arGraphicsWindow remark: saturation/contrast filter parameters = " << _contrastFilterParameters
         << "\n                         color scale values                    = " << _colorScaleFactors << endl;
  } else {
    cout << "arGraphicsWindow remark: no color filter set.\n";
  }

  // NOTE: We must assume that configure(...) can be called in a different
  // thread from draw(...). Hence the below lock is needed.
  lockViewports();

  // Figure out the viewport configuration
  const string viewMode(client.getAttribute(screenName, "viewmode",
                    "|normal|anaglyph|walleyed|crosseyed|overunder|custom|"));

  _useOGLStereo = client.getAttribute( screenName, "stereo", "|false|true|" ) == "true";
cerr << "Window configuration: viewMode = " << viewMode << ", stereo = " << _useOGLStereo << endl;
  _defaultScreen.configure( screenName, client );

  if (viewMode != "custom") {
    // One of the pre-defined view modes.
    _setViewModeNoLock(viewMode);
  }
  else {
    _clearViewportListNoLock();
    // Define an arbitrary sequence of viewports.
    // Add the first viewport, the master.
    if (!_configureCustomViewport( screenName, client, true )) {
      unlockViewports();
      return false;
    }

    // Add the other viewports.
    arSlashString viewportNameList( client.getAttribute(screenName, "viewport_list") );
    if (viewportNameList != "NULL") {
      for (int i=0; i<viewportNameList.size(); ++i) {
        if (!_configureCustomViewport( viewportNameList[i], client )) {
          unlockViewports();
          return false;
        }
      }
    }
  }
  unlockViewports();
  return true;
}

arCamera* arGraphicsWindow::setCamera( arCamera* cam ) {
  lockViewports();
  arCamera* camout = _setCameraNoLock( cam );
  unlockViewports();
  return camout;
}

// Make a local copy of the passed-in pointer.
// Some places in szg, a TEMPORARY pointer passes in a camera!
arCamera* arGraphicsWindow::_setCameraNoLock( arCamera* cam ) {
  if (cam) {
    if (_defaultCamera) {
      delete _defaultCamera;
      _defaultCamera = 0;
    }
    _defaultCamera = cam->clone();
  }
  std::vector<arViewport>::iterator iter;
  for (iter = _viewportVector.begin(); iter != _viewportVector.end(); ++iter) {
    iter->setCamera( _defaultCamera );
  }
  return _defaultCamera;
}

void arGraphicsWindow::setInitCallback( arWindowInitCallback* callback ) {
  if (_initCallback != 0) {
    delete _initCallback;
  }

  if (callback != 0) {
    _initCallback = callback;
  } else {
    _initCallback = new arDefaultWindowInitCallback;
  }
}

void arGraphicsWindow::setDrawCallback( arRenderCallback* callback ) {
  if (_drawCallback != 0) {
    delete _drawCallback;
  }

  if (callback != 0) {
    _drawCallback = callback;
  } else {
    _drawCallback = new arDefaultRenderCallback;
  }
}

bool arGraphicsWindow::setViewMode( const std::string& viewModeString ) {
  lockViewports();
  const bool stat = _setViewModeNoLock( viewModeString );
  unlockViewports();
  return stat;
}

bool arGraphicsWindow::_setViewModeNoLock( const std::string& viewModeString ) {
  // Use a "default camera". The only thing that might change
  // between views is the eye sign.
  if (viewModeString == "normal" || viewModeString == "NULL"){
    _clearViewportListNoLock();
    if (!_useOGLStereo) {
      _addViewportNoLock(
        arViewport( 0,0,1,1, _defaultScreen, _defaultCamera, 0.,
                    GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                    GL_BACK_LEFT, true )
        );
    } else {
      _addViewportNoLock(
        arViewport( 0,0,1,1, _defaultScreen, _defaultCamera, -1.,
                    GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                    GL_BACK_LEFT, true )
        );
      _addViewportNoLock(
        arViewport( 0,0,1,1, _defaultScreen, _defaultCamera, 1.,
                    GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                    GL_BACK_RIGHT, true )
        );
    }
  }
  else if (viewModeString == "anaglyph"){
    // Only uses GL_BACK_LEFT buffer.
    _clearViewportListNoLock();
    _addViewportNoLock(
      arViewport( 0,0,1,1, _defaultScreen, _defaultCamera, -1.,
                  GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE,
                  GL_BACK_LEFT, false )
      );
    _addViewportNoLock(
      arViewport( 0,0,1,1, _defaultScreen, _defaultCamera, 1.,
                  GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
  }
  else if (viewModeString == "walleyed"){
    _clearViewportListNoLock();
    _addViewportNoLock(
      arViewport( 0,0,.5,1, _defaultScreen, _defaultCamera, -1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
    _addViewportNoLock(
      arViewport( .5,0,.5,1, _defaultScreen, _defaultCamera, 1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
  }
  else if (viewModeString == "crosseyed"){
    _clearViewportListNoLock();
    _addViewportNoLock(
      arViewport( 0,0,.5,1, _defaultScreen, _defaultCamera, 1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
    _addViewportNoLock(
      arViewport( .5,0,.5,1, _defaultScreen, _defaultCamera, -1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
  }
  else if (viewModeString == "overunder"){
    _clearViewportListNoLock();
    _addViewportNoLock(
      arViewport( 0,0,1,.5, _defaultScreen, _defaultCamera, -1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
    _addViewportNoLock(
      arViewport( 0,.5,1,.5, _defaultScreen, _defaultCamera, 1.,
                  GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                  GL_BACK_LEFT, true )
      );
  }
  else {
    std::cerr << "arGraphicsWindow error: " << viewModeString
              << " is not a valid view mode.\n";
    return false;
  }

  return true;
}

void arGraphicsWindow::addViewport(const arViewport& v){
  lockViewports();
  _addViewportNoLock(v);
  unlockViewports();
}

void arGraphicsWindow::_addViewportNoLock(const arViewport& v){
  _viewportVector.push_back(v);
}

void arGraphicsWindow::clearViewportList(){
  lockViewports();
  _clearViewportListNoLock();
  unlockViewports();
}

void arGraphicsWindow::_clearViewportListNoLock(){
  _viewportVector.clear();
}

std::vector<arViewport>* arGraphicsWindow::getViewports(){
  return &_viewportVector;
}

arCamera* arGraphicsWindow::setViewportCamera( unsigned int vpindex, arCamera* cam ) {
  if (vpindex >= _viewportVector.size()) {
    cerr << "arGraphicsWindow warning: setViewportCamera(" << vpindex << ") out of range.\n"
         << "   (max = " << _viewportVector.size()-1 << ").\n";
    return NULL;
  }
  return _viewportVector[vpindex].setCamera( cam );
}

arCamera* arGraphicsWindow::setStereoViewportsCamera( unsigned int startVPIndex, arCamera* cam ) {
  if (startVPIndex >= _viewportVector.size()-1) {
    cerr << "arGraphicsWindow warning: setViewportCamera(" << startVPIndex << ") out of range.\n"
         << "   (max = " << _viewportVector.size()-2 << ").\n";
    return NULL;
  }
  arCamera* cam1 = _viewportVector[startVPIndex].setCamera( cam );
  const arCamera* cam2 = _viewportVector[startVPIndex+1].setCamera( cam );
  if (!cam1 || !cam2) {
    cerr << "arGraphicsWindow error: setStereoViewportsCamera() failed.\n";
    return NULL;
  }
  return cam1;
}

arCamera* arGraphicsWindow::getViewportCamera( unsigned int vpindex ) {
  if (vpindex >= _viewportVector.size()) {
    cerr << "arGraphicsWindow warning: getViewportCamera(" << vpindex << ") out of range.\n"
         << "   (max = " << _viewportVector.size()-1 << ").\n";
    return NULL;
  }
  return _viewportVector[vpindex].getCamera();
}

arViewport* arGraphicsWindow::getViewport( unsigned int vpindex ) {
  if (vpindex >= _viewportVector.size()) {
    cerr << "arGraphicsWindow warning: getViewport(" << vpindex << ") out of range.\n"
         << "   (max = " << _viewportVector.size()-1 << ").\n";
    return NULL;
  }
  return &_viewportVector[vpindex];
}

// Not const because _renderPass can't be.
bool arGraphicsWindow::draw() {
  _renderPass( GL_BACK_LEFT );
  if (_useOGLStereo)
    _renderPass( GL_BACK_RIGHT );
  return true;
}

void arGraphicsWindow::setPixelDimensions( int posX, int posY, int sizeX, int sizeY ) {
  _posX = posX;
  _posY = posY;
  _sizeX = sizeX;
  _sizeY = sizeY;
}

void arGraphicsWindow::getPixelDimensions( int& posX, int& posY, int& sizeX, int& sizeY ) const {
  posX = _posX; posY = _posY; sizeX = _sizeX; sizeY = _sizeY;
}

void arGraphicsWindow::_renderPass( GLenum oglDrawBuffer ) {
  // NOTE: We must assume that configure(...) can be called in a different
  // thread from draw(...). Hence the below lock is needed.
  lockViewports();

  // do whatever it takes to initialize the window.
  // by default, the depth buffer and color buffer are clear and depth test
  // is enabled.
  glDrawBuffer( oglDrawBuffer );
//  GLint drawBuffer;
//  glGetIntegerv( GL_DRAW_BUFFER, &drawBuffer );
//  cerr << "_renderPass start: set draw buffer = " << oglDrawBuffer << ", drawBuffer = " << drawBuffer << endl;
  (*_initCallback)( *this );

  // NOTE: in what follows, we will be changing the viewport. Its current
  // extent must be saved so that it can subsequently be restored.
  int params[4];
  glGetIntegerv(GL_VIEWPORT, (GLint*)params);

  std::vector<arViewport>::iterator i;
  for (i=_viewportVector.begin(); i != _viewportVector.end(); ++i){
    if (i->getDrawBuffer() != oglDrawBuffer)
      continue;
    i->activate();
    _currentEyeSign = i->getEyeSign();
    (*_drawCallback)(*this, *i);
    if (_useColorFilter)
      _applyColorFilter();
    // Restore the original viewport, lest the viewports shrink
    // and disappear in modes like walleyed.
    glViewport( (GLint)params[0], (GLint)params[1],
                (GLsizei)params[2], (GLsizei)params[3] );
  }

//  glGetIntegerv( GL_DRAW_BUFFER, (GLint*)&drawBuffer );
//  cerr << "_renderPass end: drawBuffer = " << drawBuffer << endl;
  _currentEyeSign = 0.;

  // It's a good idea to set this back to normal. What if the user-defined
  // window callback exists and knows nothing about the color buffer?
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

  unlockViewports();
}

void arGraphicsWindow::_applyColorFilter() {
  glPushAttrib( GL_ALL_ATTRIB_BITS );
  glDisable( GL_TEXTURE_1D );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING );
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluOrtho2D( -1., 1., -1., 1. );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glColor4fv( _contrastFilterParameters.v );
  glBegin( GL_QUADS );
  glVertex2f( -1., -1. );
  glVertex2f( 1., -1. );
  glVertex2f( 1., 1. );
  glVertex2f( -1., 1. );
  glEnd();
  glBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
  glColor3fv( (.5*_colorScaleFactors).v );
  glBegin( GL_QUADS );
  glVertex2f( -1., -1. );
  glVertex2f( 1., -1. );
  glVertex2f( 1., 1. );
  glVertex2f( -1., 1. );
  glEnd();
  glPopAttrib();
}

bool arGraphicsWindow::_configureCustomViewport( const string& screenName,
                                                 arSZGClient& client,
                                                 bool masterViewport ) {
  arGraphicsScreen screen;
  if (masterViewport) {
    screen = _defaultScreen;
  } else {
    if (!screen.configure( screenName, client )) {
      cerr << "arGraphicsWindow error: failed to configure screen " << screenName << endl;
      return false;
    }
  }

  // get viewport dimensions (relative to screen dimensions)
  float dim[4];
  if (!client.getAttributeFloats(  screenName, "viewport_dim", dim, 4 )) {
    if (!masterViewport) {
      cerr << "arGraphicsWindow error: " << screenName << "/viewport_dim undefined when view_mode is custom.\n";
      return false;
    }
    // use defaults and do not fail.
    cout << "arGraphicsWindow remark: Failed to get " << screenName
         << "/viewport_dim (master viewport); using (0,0,1,1)=full window.\n";
    dim[0] = 0; dim[1] = 0; dim[2] = 1; dim[3] = 1;
  }
  // get viewport eye sign
  float eyeSign = 0.;
  if (!client.getAttributeFloats(screenName, "eye_sign", &eyeSign, 1)) {
    cout << "arGraphicsWindow remark: Failed to get " << screenName
         << "/eye_sign; using 0.\n";
    eyeSign = 0.;
  }

  // get viewport color mask
  arSlashString colorMask(client.getAttribute(screenName, "color_mask"));
  GLboolean mask[4];
  unsigned int i = 0;
  if (colorMask == "NULL") {
    cout << "arGraphicsWindow remark: could not get viewport color mask for "
	 << screenName << ". Using (true/true/true/true).\n";
    for (i=0; i<4; ++i)
      mask[i] = GL_TRUE;
  } else {
    if (colorMask.size() == 3) {
      cout << "arGraphicsWindow remark: " << screenName << "/color_mask alpha defaulting to true.\n";
      colorMask /= "true";
    }
    if (colorMask.size() != 4) {
      cerr << "arGraphicsWindow error: " << screenName
           << "/color_mask must contain 0, 3, or 4 elements.\n";
      return false;
  }
    for (i=0; i<4; ++i) {
      if (colorMask[i] == "true"){
        mask[i] = GL_TRUE;
      }
      else if (colorMask[i] == "false"){
        mask[i] = GL_FALSE;
      }
      else {
        cerr << "arGraphicsWindow error: " << screenName << "/color_mask contains illegal value '"
             << colorMask[i] << "'\n";
        return false;
      }
    }
  }

  // get viewport depth buffer clear
  const string depthClear = client.getAttribute(screenName, "depth_clear",
	                                   "|true|false|");

  // get associated OpenGL draw buffer
  const string bufName = client.getAttribute( screenName, "draw_buffer_mode",
	                                   "|left|right|normal|mono|");
  if (bufName == "normal") {
    cout << "arGraphicsWindow remark: " << screenName << "/draw_buffer_mode == normal,"
         << " ignoring eye_sign.\n";
    // NOTE: This is all called from within a viewport lock. So, we must
    // use the no-lock version of the call.
    if (_useOGLStereo) {
      _addViewportNoLock( arViewport( dim[0], dim[1], dim[2], dim[3], screen, _defaultCamera, -1.,
                                mask[0], mask[1], mask[2], mask[3],
                                GL_BACK_LEFT, (depthClear!="false") ) );
      _addViewportNoLock( arViewport( dim[0], dim[1], dim[2], dim[3], screen, _defaultCamera, 1.,
                                mask[0], mask[1], mask[2], mask[3],
                                GL_BACK_RIGHT, (depthClear!="false") ) );
    } else {
      _addViewportNoLock( arViewport( dim[0], dim[1], dim[2], dim[3], screen, _defaultCamera, 0.,
                                mask[0], mask[1], mask[2], mask[3],
                                GL_BACK_LEFT, (depthClear!="false") ) );
  }
  } else {
    GLenum oglDrawBuf = bufName == "right" ? GL_BACK_RIGHT : GL_BACK_LEFT;

    _addViewportNoLock( arViewport( dim[0], dim[1], dim[2], dim[3], screen, _defaultCamera, eyeSign,
                              mask[0], mask[1], mask[2], mask[3],
                              oglDrawBuf, (depthClear!="false") ) );
  }
  return true;
}



