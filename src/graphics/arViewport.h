//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VIEWPORT_H
#define AR_VIEWPORT_H

#include "arGraphicsHeader.h"
#include "arCamera.h"

class arViewport {
 public:
  arViewport();
  arViewport( const arViewport& x );
  arViewport& operator=( const arViewport& x );
  virtual ~arViewport();

  void setViewport( float left, float bottom,
                    float width, float height );
  void setCamera(arCamera* camera);
  arCamera* getCamera();
  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green, 
		    GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);

  void activate();

 private:
  // these define the viewport, in relative coordinates.
  // The floats are between 0 and 1. So the left half of the window would
  // be given by (0,0,0.5,0.5)
  float _left;
  float _bottom;
  float _width;
  float _height;
  // the viewport can reference an arCamera, which might be used
  // by the drawing object to put transforms on the stack. NOTE:
  // as of yet, the frameworks DO NOT use this feature. BUT this feature
  // will be required for drawing multiple viewports for (say) different
  // walls in the same window.
  // THERE ARE PROBLEMS IN THE WAY THIS POINTER IS COPIED AROUND IN THE
  // COPY CONSTRUCTORS, ETC. HOWEVER, IT IS NOT YET USED IN THE FRAMEWORKS
  // SO THAT DOES NOT MATTER (MUCH)... Maybe the pointer *should* be
  // copied around though...
  arCamera* _camera;
  // with the current confused state of cameras in Syzygy, one also
  // needs to include the eye sign here (i.e. if -1 it is the left eye
  // and if 1 it is the right eye)
  float _eyeSign;
  // so that we can draw anaglyph, we also need to deal with the color
  // mask!
  GLboolean _red;
  GLboolean _green;
  GLboolean _blue;
  GLboolean _alpha;
  // as a KLUDGE we can make the viewport clear the depth buffer on activate.
  // this is needed for anaglyph (where the depth buffer must be cleared
  // between drawing the red and the blue image)
  bool _clearDepthBuffer;
};

#endif        //  #ifndefARVIEWPORT_H

