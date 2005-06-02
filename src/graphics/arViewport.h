//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VIEWPORT_H
#define AR_VIEWPORT_H

#include "arGraphicsHeader.h"
#include "arCamera.h"
#include "arGraphicsScreen.h"
// THIS MUST BE THE LAST SZG INCLUDE!
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
              bool clearZBuf );
  arViewport( const arViewport& x );
  arViewport& operator=( const arViewport& x );
  virtual ~arViewport();

  void setViewport( float left, float bottom,
                    float width, float height );
  void setScreen( const arGraphicsScreen& screen ) { _screen = screen; }
  arGraphicsScreen* getScreen() { return &_screen; }
  // NOTE: the viewport now owns its camera. 
  // It makes a copy here & returns the address of the copy
  arCamera* setCamera( arCamera* camera);
  arCamera* getCamera();
  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green, 
		    GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);
  // e.g. GL_BACK_LEFT
  void setDrawBuffer( GLenum buf ) { _oglDrawBuffer = buf; }
  GLenum getDrawBuffer() const { return _oglDrawBuffer; }
  void activate();

 private:
  // these define the viewport, in relative coordinates.
  // The floats are between 0 and 1. So the left half of the window would
  // be given by (0,0,0.5,0.5)
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
};

#endif        //  #ifndefARVIEWPORT_H

