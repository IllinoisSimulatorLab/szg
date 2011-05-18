//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VIEWPORT_H
#define AR_VIEWPORT_H

#include "arGraphicsHeader.h"
#include "arCamera.h"
#include "arGraphicsScreen.h"
#include "arGraphicsCalling.h"

class SZG_CALL arViewport {
 public:
  arViewport();
  arViewport( float left, float bottom, float width, float height,
              const arGraphicsScreen& screen,
              arCamera* cam,
              float eyeSign,
              GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha,
              GLenum oglDrawBuf,
              bool clearZBuf,
              bool clearColBuf=false );
  arViewport( const float* lbwh,
              const arGraphicsScreen& screen,
              arCamera* cam,
              float eyeSign,
              const GLboolean* rgba,
              GLenum oglDrawBuf,
              bool clearZBuf,
              bool clearColBuf=false );
  arViewport( const arViewport& x );
  arViewport& operator=( const arViewport& x );
  virtual ~arViewport();

  void setViewport( arVector4& viewport );
  void setViewport( float left, float bottom, float width, float height );
  arVector4 getViewport() const { return arVector4(_left, _bottom, _width, _height); }
  void setScreen( const arGraphicsScreen& screen ) { _screen = screen; }
  arGraphicsScreen* getScreen() { return &_screen; }
  // The viewport owns its camera.  It makes a copy here and returns the copy's address.
  arCamera* setCamera( arCamera* camera);
  arCamera* getCamera();
  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green,
                    GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);
  void clearColorBuffer(bool flag);
  // e.g. GL_BACK_LEFT
  void setDrawBuffer( GLenum buf ) { _oglDrawBuffer = buf; }
  GLenum getDrawBuffer() const { return _oglDrawBuffer; }
  void activate();
  void deactivate();

 private:
  // Viewport in normalized coordinates.
  // For example, the left half of the window is (0, 0, 0.5, 0.5).
  float _left;
  float _bottom;
  float _width;
  float _height;
  arGraphicsScreen _screen;
  arCamera* _camera;
  // each viewport has an eyeSign associated with it
  // (i.e. if -1 it is the left eye
  // and if 1 it is the right eye, 0 if no eye offset is needed)
  float _eyeSign;
  // so that we can draw anaglyph, we also need to deal with the color
  // mask!
  GLboolean _red;
  GLboolean _green;
  GLboolean _blue;
  GLboolean _alpha;
  GLenum _oglDrawBuffer;
  // as a KLUDGE we can make the viewport clear the depth buffer on activate.
  // this is needed for anaglyph (where the depth buffer must be cleared
  // between drawing the red and the blue image)
  bool _clearDepthBuffer;
  // If we have one viewport superimposed on a larger one, we need to be able
  // to clear the color buffer in the area of that viewport.
  bool _clearColorBuffer;
};

#endif        //  #ifndefARVIEWPORT_H
