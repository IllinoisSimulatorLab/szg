//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRUSTUM_CAMERA_H
#define AR_FRUSTUM_CAMERA_H

#include "arCamera.h"
#include "arGraphicsCalling.h"

#include <string>

class SZG_CALL arFrustumCamera: public arCamera{
 public:
  arFrustumCamera();
  arFrustumCamera( const float* const frust, const float* const look );
  virtual ~arFrustumCamera() {}
  virtual arCamera* clone() const
    { return (arCamera*) new arFrustumCamera( frustum, lookat ); }

  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual std::string type() const { return "arFrustumCamera"; }

  void setFrustum( const float* frust )
    { memcpy(frustum, frust, 6*sizeof(float)); }
  void setLook   ( const float* look )
    { memcpy(lookat, look, 9*sizeof(float)); }
  void setSides  ( const arVector4& sides )
    { memcpy(frustum, sides.v, 4*sizeof(float)); }
  void setPosition( const arVector3& pos )
    { memcpy(lookat, pos.v, 3*sizeof(float)); }
  void setTarget( const arVector3& target )
    { memcpy(lookat+3, target.v, 3*sizeof(float)); }
  void setUp( const arVector3& up )
    { memcpy(lookat+6, up.v, 3*sizeof(float)); }

  void setSides(float left, float right, float bottom, float top) {
    frustum[0] = left;
    frustum[1] = right;
    frustum[2] = bottom;
    frustum[3] = top;
  }

  void setPosition(float x, float y, float z) {
    lookat[0] = x;
    lookat[1] = y;
    lookat[2] = z;
  }

  void setTarget(float x, float y, float z) {
    lookat[3] = x;
    lookat[4] = y;
    lookat[5] = z;
  }

  void setUp(float x, float y, float z) {
    lookat[6] = x;
    lookat[7] = y;
    lookat[8] = z;
  }

  void setNearFar(float nearClip, float farClip) {
    frustum[4] = nearClip;
    frustum[5] = farClip;
  }

  arVector4 getSides() const { return arVector4(frustum); }
  arVector3 getPosition() const { return arVector3(lookat); }
  arVector3 getTarget() const { return arVector3(lookat+3); }
  arVector3 getUp() const { return arVector3(lookat+6); }
  float getNear() const { return frustum[4]; }
  float getFar() const { return frustum[5]; }

  // Parameters to the projection matrix call, glOrtho or glFrustum.
  float frustum[6];
  // Parameters to gluLookat.
  float lookat[9];
};

#endif
