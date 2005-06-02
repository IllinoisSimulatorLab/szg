//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_ORTHO_CAMERA_H
#define AR_ORTHO_CAMERA_H

#include "arCamera.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arOrthoCamera: public arCamera{
 public:
  arOrthoCamera();
  arOrthoCamera( const float* const orth, const float* const look );
  virtual ~arOrthoCamera(){}
  virtual arCamera* clone() const { return (arCamera*) new arOrthoCamera( ortho, lookat ); }

  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();

  void setSides(float left, float right, float bottom, float top){
    ortho[0] = left;
    ortho[1] = right;
    ortho[2] = bottom;
    ortho[3] = top;
  }

  void setNearFar(float nearClip, float farClip){
    ortho[4] = nearClip;
    ortho[5] = farClip;
  }

  void setPosition(float x, float y, float z){
    lookat[0] = x;
    lookat[1] = y;
    lookat[2] = z;
  }

  void setTarget(float x, float y, float z){
    lookat[3] = x;
    lookat[4] = y;
    lookat[5] = z;
  }
 
  void setUp(float x, float y, float z){
    lookat[6] = x;
    lookat[7] = y;
    lookat[8] = z;
  }

  // the parameters to glOrtho(...)
  float ortho[6];
  // the parameters to gluLookat(...)
  float lookat[9];
};

#endif
