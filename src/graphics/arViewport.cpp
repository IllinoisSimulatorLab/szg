//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arViewport.h"

arViewport::arViewport() :
  _left(0),
  _bottom(0),
  _width(1),
  _height(1),
  _camera(NULL),
  _eyeSign(0),
  _red(GL_TRUE),
  _green(GL_TRUE),
  _blue(GL_TRUE),
  _alpha(GL_TRUE),
  _clearDepthBuffer(false){
}

arViewport::arViewport( const arViewport& v ) :
  _left( v._left ),
  _bottom( v._bottom ),
  _width( v._width ),
  _height( v._height ),
  _camera( v._camera ),
  _eyeSign( v._eyeSign ),
  _red( v._red ),
  _green( v._green ),
  _blue( v._blue ),
  _alpha( v._alpha ),
  _clearDepthBuffer( v._clearDepthBuffer){
}

arViewport& arViewport::operator=( const arViewport& v ) {
  if (&v == this){
    return *this;
  }

  _left = v._left;
  _bottom = v._bottom;
  _width = v._width;
  _height = v._height;
  _camera = v._camera;
  _eyeSign = v._eyeSign;
  _red = v._red;
  _green = v._green;
  _blue = v._blue;
  _alpha = v._alpha;
  _clearDepthBuffer = v._clearDepthBuffer;

  return *this;
}

arViewport::~arViewport() {
  // DO NOT DELETE THE arScreenObject HERE! IT MIGHT NOT BE OWNED!
}

void arViewport::setViewport( float left, float bottom,
                              float width, float height ) {
  _left = left;
  _bottom = bottom;
  _width = width;
  _height = height;
}

void arViewport::setCamera(arCamera* camera){
  _camera = camera;
}

arCamera* arViewport::getCamera(){
  return _camera;
}

// IT SEEMS LIKE THIS SHOULD GO AWAY... BEING FOLDED BACK IN TO THE
// CAMERA....
void arViewport::setEyeSign(float eyeSign){
  _eyeSign = eyeSign;
}

float arViewport::getEyeSign(){
  return _eyeSign;
}

/// The viewport sets the color mask upon drawing (used for anaglyph stereo).
void arViewport::setColorMask(GLboolean red, GLboolean green, 
		              GLboolean blue, GLboolean alpha){
  _red = red;
  _blue = blue;
  _green = green;
  _alpha = alpha;
}

/// The viewport might clear the depth buffer upon drawing (used for
/// anaplyph stereo).
void arViewport::clearDepthBuffer(bool flag){
  _clearDepthBuffer = flag;
}

void arViewport::activate(){
  // get the window size and set the viewport based on that and the 
  // proportions stored herein.
  int params[4];
  glGetIntegerv( GL_VIEWPORT, params );
  int left, bottom, width, height;
  // there's a whole compilcated song and dance to ensure that, for instance,
  // viewports of (0,0,0.5,1) and (0.5,0,0.5,1) on a window of size 1024x768
  // give OpenGL viewports of (0,0,512,768) and (512,0,512,768) respectively
  left = int(params[2]*_left);
  bottom = int(params[3]*_bottom);
  width = int(params[2]*_width);
  height = int(params[3]*_height);

  glViewport( (GLint)left, (GLint)bottom, (GLsizei)width, (GLsizei)height );

  // set the follow mask
  glColorMask(_red, _green, _blue, _alpha);
  
  // clear the depth buffer if necessary
  if (_clearDepthBuffer){
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}

