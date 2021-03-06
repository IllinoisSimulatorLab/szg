//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arViewport.h"

arViewport::arViewport() :
  _left(0),
  _bottom(0),
  _width(1),
  _height(1),
  _screen(),
  _camera(NULL),
  _eyeSign(0),
  _red(GL_TRUE),
  _green(GL_TRUE),
  _blue(GL_TRUE),
  _alpha(GL_TRUE),
  _oglDrawBuffer(GL_BACK_LEFT),
  _clearDepthBuffer(false),
  _clearColorBuffer(false) {
}

arViewport::arViewport( float left, float bottom, float width, float height,
                        const arGraphicsScreen& screen,
                        arCamera* cam,
                        float eyeSign,
                        GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha,
                        GLenum oglDrawBuf,
                        bool clearZBuf,
                        bool clearColBuf ) :
  _left(left),
  _bottom(bottom),
  _width(width),
  _height(height),
  _screen(screen),
  _camera(0),
  _eyeSign(eyeSign),
  _red(red),
  _green(green),
  _blue(blue),
  _alpha(alpha),
  _oglDrawBuffer(oglDrawBuf),
  _clearDepthBuffer(clearZBuf),
  _clearColorBuffer(clearColBuf) {
  if (cam) {
    _camera = cam->clone();
  }
}

arViewport::arViewport( const float* lbwh,
                        const arGraphicsScreen& screen,
                        arCamera* cam,
                        float eyeSign,
                        const GLboolean* rgba,
                        GLenum oglDrawBuf,
                        bool clearZBuf,
                        bool clearColBuf ) :
  _left(lbwh[0]),
  _bottom(lbwh[1]),
  _width(lbwh[2]),
  _height(lbwh[3]),
  _screen(screen),
  _camera(0),
  _eyeSign(eyeSign),
  _red(rgba[0]),
  _green(rgba[1]),
  _blue(rgba[2]),
  _alpha(rgba[3]),
  _oglDrawBuffer(oglDrawBuf),
  _clearDepthBuffer(clearZBuf),
  _clearColorBuffer(clearColBuf) {
  if (cam) {
    _camera = cam->clone();
  }
}

arViewport::arViewport( const arViewport& v ) :
  _left( v._left ),
  _bottom( v._bottom ),
  _width( v._width ),
  _height( v._height ),
  _screen( v._screen ),
  _camera( 0 ),
  _eyeSign( v._eyeSign ),
  _red( v._red ),
  _green( v._green ),
  _blue( v._blue ),
  _alpha( v._alpha ),
  _oglDrawBuffer( v._oglDrawBuffer ),
  _clearDepthBuffer( v._clearDepthBuffer),
  _clearColorBuffer( v._clearColorBuffer) {
  if (v._camera) {
    _camera = v._camera->clone();
  }
}

arViewport& arViewport::operator=( const arViewport& v ) {
  if (&v == this) {
    return *this;
  }

  _left = v._left;
  _bottom = v._bottom;
  _width = v._width;
  _height = v._height;
  _screen = v._screen;
  if (_camera) {
    delete _camera;
    _camera = 0;
  }
  if (v._camera) {
    _camera = v._camera->clone();
  }
  _camera = v._camera->clone();
  _eyeSign = v._eyeSign;
  _red = v._red;
  _green = v._green;
  _blue = v._blue;
  _alpha = v._alpha;
  _oglDrawBuffer = v._oglDrawBuffer;
  _clearDepthBuffer = v._clearDepthBuffer;
  _clearColorBuffer = v._clearColorBuffer;

  return *this;
}

arViewport::~arViewport() {
  if (_camera) {
    delete _camera;
  }
}

void arViewport::setViewport( arVector4& viewport ) {
  setViewport( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ] );
}

void arViewport::setViewport( float left, float bottom,
                              float width, float height ) {
  _left = left;
  _bottom = bottom;
  _width = width;
  _height = height;
}

arCamera* arViewport::setCamera(arCamera* camera) {
  if (_camera) {
    delete _camera;
    _camera = 0;
  }
  if (camera) {
    _camera = camera->clone();
  }
  return _camera;
}

arCamera* arViewport::getCamera() {
  return _camera;
}

// IT SEEMS LIKE THIS SHOULD GO AWAY... BEING FOLDED BACK IN TO THE
// CAMERA....
// No, I don't think so.
void arViewport::setEyeSign(float eyeSign) {
  _eyeSign = eyeSign;
}

float arViewport::getEyeSign() {
  return _eyeSign;
}

// The viewport sets the color mask upon drawing (used for anaglyph stereo).
void arViewport::setColorMask(GLboolean red, GLboolean green,
                              GLboolean blue, GLboolean alpha) {
  _red = red;
  _blue = blue;
  _green = green;
  _alpha = alpha;
}

// The viewport might clear the depth buffer upon drawing (used for
// anaplyph stereo).
void arViewport::clearDepthBuffer(bool flag) {
  _clearDepthBuffer = flag;
}

void arViewport::clearColorBuffer(bool flag) {
  _clearColorBuffer = flag;
}

void arViewport::activate() {
  // The viewport does not call glDrawBuffer(). The arGraphicsWindow performs
  // rendering passes in which it clears the appropriate buffer & the queries
  // its viewport list to determine which ones want to render into that buffer.

  // get the window size and set the viewport based on that and the
  // proportions stored herein.
  int params[4] = {0};
  glGetIntegerv( GL_VIEWPORT, (GLint*)params );

  // there's a whole complicated song and dance to ensure that, for instance,
  // viewports of (0, 0, 0.5, 1) and (0.5, 0, 0.5, 1) on a window of size 1024x768
  // give OpenGL viewports of (0, 0, 512, 768) and (512, 0, 512, 768) respectively
  const GLint left = int(params[2]*_left);
  const GLint bottom = int(params[3]*_bottom);
  const GLsizei width = int(params[2]*_width);
  const GLsizei height = int(params[3]*_height);

  // Save OpenGL state to be restored in deactivate()
  glPushAttrib( GL_VIEWPORT_BIT | GL_SCISSOR_BIT );

  glViewport( left, bottom, width, height );

  if (_camera) {
    _camera->setEyeSign( getEyeSign() );
    _camera->setScreen( getScreen() );
    _camera->loadViewMatrices();
  }

  // set the follow mask
  glColorMask(_red, _green, _blue, _alpha);

  if (_clearColorBuffer) {
    glScissor( left, bottom, width, height );
    glEnable( GL_SCISSOR_TEST );
  }
  if (_clearDepthBuffer && _clearColorBuffer) {
    // clear color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  } else if (_clearColorBuffer) {
    glClear( GL_COLOR_BUFFER_BIT );
  } else if (_clearDepthBuffer) {
    glClear( GL_DEPTH_BUFFER_BIT );
  }
}

void arViewport::deactivate() {
  glPopAttrib();
}

