//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGraphicsUtilities.h"
#include "arGraphicsWindow.h"
#include "arScreenObject.h"
#include <iostream>

arCamera* ar_graphicsWindowDefaultCameraFactory(){
  return new arScreenObject();
}

void arDefaultWindowInitCallback::operator()( arGraphicsWindow& ) {
  glEnable(GL_DEPTH_TEST);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void arDefaultRenderCallback::operator()( arGraphicsWindow&, arViewport& ) {
  glClearColor( 0,0,0,1 );
}

arGraphicsWindow::arGraphicsWindow() :
  _useOGLStereo(false),
  _defaultEyeSign(0.),
  _viewMode(VIEW_NORMAL),
  _defaultCamera(NULL),
  _initCallback(0),
  _drawCallback(0){
  
  // by default, the graphics window contains a single "normal" viewport
  // in its list (note that the default viewport corresponds to normal).
  arViewport temp;
  _viewportList.push_back(temp);

  _initCallback = new arDefaultWindowInitCallback;
  _drawCallback = new arDefaultRenderCallback;

  // by default, we'll be using screen objects for our automatically
  // created configurations (i.e. from configure(...)). It is still
  // possible to put an arGraphicsWindow together in an arbitrary fashion.
  _cameraFactory = ar_graphicsWindowDefaultCameraFactory;
}

//arGraphicsWindow::arGraphicsWindow( const arGraphicsWindow& x ) :
//  _useOGLStereo( x._useOGLStereo ),
//  _viewMode( x._viewMode ),
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
//  _viewMode = x._viewMode;
//  _initCallback = x._initCallback;
//  _drawCallback = x. _drawCallback;
//  _width = x._width;
//  _height = x._height;
//  return *this;
//}

arGraphicsWindow::~arGraphicsWindow() {
  // It is not a good idea to delete the callback objects here.
  // For instance, declaring the callbacks statically and then registering
  // them with the arGraphicsWindow will then generate a segfault on exit.
  // A good general policy is: if a pointer is passed-in, it is not owned...
  // do not delete it. It is the responsibility of the passer to delete.
  /*if (_initCallback != 0)
      delete _initCallback;
    if (_drawCallback != 0)
      delete _drawCallback;*/
}

bool arGraphicsWindow::configure(arSZGClient* client){
  if (!client){
    return false;
  }
  
  // Figure out the viewport configuration
  string screenName = client->getMode("graphics");
  string viewMode = client->getAttribute(screenName, "view_mode",
                    "|normal|anaglyph|walleyed|crosseyed|overunder|custom|");
  if (viewMode == "custom"){
    // Very important to clear the viewport list.
    clearViewportList();
    // In this case, we are defining an arbitrary sequence of viewports
    // Add the first viewport. This is the "master" viewport.
    // The arMasterSlaveFramework expects a "privileged camera". This is
    // it.
    _defaultCamera = _parseViewport(client,screenName);
    // Add the other viewports
    arSlashString viewportList(client->getAttribute(screenName,
		                                    "viewport_list"));
    for (int i=0; i<viewportList.size(); i++){
      _parseViewport(client, viewportList[i]);
    }
  }
  else{
    _defaultCamera = _cameraFactory();
    // configure using the default screen name
    if (!_defaultCamera->configure(client)){
      return false;
    }
    // we are using one of the pre-defined view modes
    setViewMode(viewMode);
    string defaultEye = client->getAttribute(screenName, "default_eye",
                                             "|none|left|right|");
    setDefaultEye( defaultEye );
  }
  return true;
}

void arGraphicsWindow::setDefaultEye( const std::string& eye ) {
  if (eye == "left")
    _defaultEyeSign = -1.;
  else if (eye == "right")
    _defaultEyeSign = 1.;
  else if ((eye == "none")||(eye == "middle")||(eye == "NULL"))
    _defaultEyeSign = 0.;
  else {
    std::cerr << "arGraphicsWindow warning: invalid eye sign \""
              << eye << "\", defaulting to none.\n";
    _defaultEyeSign = 0.;
  }
}

void arGraphicsWindow::setDefaultCamera(arCamera* camera){
  _defaultCamera = camera;
}

void arGraphicsWindow::setInitCallback( arWindowInitCallback* callback ) {
  if (_initCallback != 0)
    delete _initCallback;
  if (callback != 0) {
    _initCallback = callback;
  } else {
    _initCallback = new arDefaultWindowInitCallback;
  }
}

void arGraphicsWindow::setDrawCallback( arRenderCallback* callback ) {
  if (_drawCallback != 0)
    delete _drawCallback;
  if (callback != 0) {
    _drawCallback = callback;
  } else {
    _drawCallback = new arDefaultRenderCallback;
  }
}

bool arGraphicsWindow::setViewMode( const std::string& viewModeString ) {
  // NOTE: We use a "default camera" here. The only thing that changes
  // between views is the eye sign, at most.
  arViewport temp;
  if (viewModeString == "normal" || viewModeString == "NULL"){
    _viewMode = VIEW_NORMAL;
    // important to go ahead and clear the list
    _viewportList.clear();
	temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
  }
  else if (viewModeString == "anaglyph"){
    _viewMode = VIEW_ANAGLYPH;
    // important to go ahead and clear the list
    _viewportList.clear();
    temp.setColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
    temp.setViewport(0,0,1,1);
    temp.setEyeSign(-1);
    temp.clearDepthBuffer(false);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
    temp.setColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
    temp.setViewport(0,0,1,1);
    temp.setEyeSign(1);
    temp.clearDepthBuffer(true);
	temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
  }
  else if (viewModeString == "walleyed"){
    _viewMode = VIEW_WALLEYED;
    // important to go ahead and clear the list
    _viewportList.clear();
    temp.setViewport(0,0,0.5,1);
    temp.setEyeSign(-1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
    temp.setViewport(0.5,0,0.5,1);
    temp.setEyeSign(1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
  }
  else if (viewModeString == "crosseyed"){
    _viewMode = VIEW_CROSSEYED;
    // important to go ahead and clear the list
    _viewportList.clear();
    temp.setViewport(0,0,0.5,1);
    temp.setEyeSign(1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
    temp.setViewport(0.5,0,0.5,1);
    temp.setEyeSign(-1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
  }
  else if (viewModeString == "overunder"){
    _viewMode = VIEW_OVERUNDER;
    // important to go ahead and clear the list
    _viewportList.clear();
    temp.setViewport(0,0,1,0.5);
    temp.setEyeSign(-1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
    temp.setViewport(0,0.5,1,0.5);
    temp.setEyeSign(1);
    temp.setCamera(_defaultCamera);
    _viewportList.push_back(temp);
  }
  else {
    std::cerr << "arGraphicsWindow error: " << viewModeString
              << " is not a valid view mode.\n";
    return false;
  }

  // NOT SURE WHERE THIS CODE SHOULD GO OR HOW INTERLACED STEREO SHOULD BE
  // IMPLEMENTED
  //    case VIEW_INTERLACED:
  //      vp.update();
  //      vp.getViewport( left, bottom, width, height );
  //      glMatrixMode( GL_PROJECTION );
  //      glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
  //      glMatrixMode( GL_MODELVIEW );
  //      glLoadIdentity();
  //      glLineWidth(1.);
  //      glColor3f(0,0,0);
  //      glClearStencil(0);
  //      glClear(GL_STENCIL_BUFFER_BIT);
  //      glStencilFunc (GL_LESS 0x1, 0x1);
  //      glStencilOp (GL_REPLACE, GL_REPLACE, GL_REPLACE);
  //      glStencilOp (GL_REPLACE, GL_REPLACE, GL_REPLACE);
  //      glDisable(GL_STENCIL_TEST);

  return true;
}

void arGraphicsWindow::addViewport(const arViewport& v){
  _viewportList.push_back(v);
  // we have a custom view mode
  _viewMode = VIEW_CUSTOM;
}

void arGraphicsWindow::clearViewportList(){
  _viewportList.clear();
}

list<arViewport>* arGraphicsWindow::getViewportList(){
  return &_viewportList;
}

bool arGraphicsWindow::draw() {
  // NOTE: CURRENTLY THIS DOES NOT FULLY WORK AS EXPECTED!
  // SPECIFICALLY, THE EYESIGN PARAMETER PASSED TO _renderPass IS ONLY
  // HONORED IF WE ARE IN VIEW_NORMAL MODE. i.e. you cannot really mix
  // quad-buffered stereo and these other modes. Actually, for the most
  // part, that's not a loss... and isn't that different from what was
  // already happening! MUST FIX (so that we can mix quad-buffered and
  // anaglyph for a 2 person experience)
  if (_useOGLStereo) {
    glDrawBuffer(GL_BACK_LEFT);
    _renderPass(-1.);
    glDrawBuffer(GL_BACK_RIGHT);
    _renderPass(1.);
  } else {
    _renderPass( _defaultEyeSign );
  }
  return true;
}

void arGraphicsWindow::_renderPass( float eyeSign ) {
  // do whatever it takes to initialize the window. 
  // by default, the depth buffer and color buffer are clear and depth test
  // is enabled.
  (*_initCallback)( *this );

  // NOTE: in what follows, we will be changing the viewport. Its current
  // extent must be saved so that it can subsequently be restored.
  int params[4];
  glGetIntegerv(GL_VIEWPORT, params);

  list<arViewport>::iterator i;
  for (i=_viewportList.begin(); i != _viewportList.end(); i++){
    (*i).activate();
    // NOTE THE KLUDGE THAT MAKES VIEW_NORMAL WORK!
    if (_viewMode == VIEW_NORMAL){
      (*i).setEyeSign(eyeSign);
    }
    (*_drawCallback)(*this, *i);
    // restore the original viewport. if this doesn't occur, the viewports
    // will get progressively smaller and disappear in modes like walleyed.
    glViewport( (GLint)params[0], (GLint)params[1], 
                (GLsizei)params[2], (GLsizei)params[3] );
  }

  // It's a good idea to set this back to normal. What if the user-defined
  // window callback exists and knows nothing about the color buffer?
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}

arCamera* arGraphicsWindow::_parseViewport(arSZGClient* client,
                                           const string& screenName){
  arCamera* camera = _cameraFactory();
  if (!camera->configure(screenName, client)){
    return NULL;
  }
  arViewport* viewport = new arViewport();

  viewport->setCamera(camera);
  // get viewport dimensions (relative to screen dimensions)
  float dim[4];
  if (!client->getAttributeFloats(screenName, "viewport_dim", dim, 4)){
    // use defaults and do not fail.
    cout << "arGraphicsWindow remark: could not get viewport dimensions for "
	 << screenName << ". Using defaults.\n";
    dim[0] = 0; dim[1] = 0; dim[2] = 1; dim[3] = 1;
  }
  viewport->setViewport(dim[0], dim[1], dim[2], dim[3]);
  // get viewport eye sign
  float eyeSign;
  client->getAttributeFloats(screenName, "eye_sign", &eyeSign, 1);
  viewport->setEyeSign(eyeSign);
  // get viewport color mask
  arSlashString colorMask(client->getAttribute(screenName, "color_mask"));
  if (colorMask.size() != 4){
    cout << "arGraphicsWindow remark: could not get viewport color mask for "
	 << screenName << ". Using defaults.\n";
    viewport->setColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }
  else{
    for (int i=0; i<4; i++){
      GLboolean mask[4];
      if (colorMask[i] == "true"){
        mask[i] = GL_TRUE;
      }
      else if (colorMask[i] == "false"){
        mask[i] = GL_FALSE;
      }
      else{
        cout << "arGraphicsWindow remark: found illegal color mask value = "
	     << colorMask[i] << "\n for screen = " << screenName 
	     << ". Using default.\n";
	mask[i] = GL_TRUE;
      }
      viewport->setColorMask(mask[0], mask[1], mask[2], mask[3]);
    }
  }
  // get viewport depth buffer clear
  string depthClear = client->getAttribute(screenName, "depth_clear",
	                                   "|true|false|");
  if (depthClear == "true"){
    viewport->clearDepthBuffer(true);
  }
  else{
    viewport->clearDepthBuffer(false);
  }

  addViewport(*viewport);
  return camera;
}



