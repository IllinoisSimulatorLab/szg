//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PERSPECTIVE_CAMERA_H
#define AR_PERSPECTIVE_CAMERA_H

#include "arCamera.h"

class SZG_CALL arPerspectiveCamera: public arCamera{
 public:
  arPerspectiveCamera();
  virtual ~arPerspectiveCamera(){}

  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();

  void setSides(float left, float right, float bottom, float top){
    frustum[0] = left;
    frustum[1] = right;
    frustum[2] = bottom;
    frustum[3] = top;
  }

  void setNearFar(float nearClip, float farClip){
    frustum[4] = nearClip;
    frustum[5] = farClip;
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

  // something so that we can get the values out in python... probably
  // a less hack-like way to do this
  float getFrustumData(int i){ return frustum[i]; }
  float getLookatData(int i){ return lookat[i]; }

  // each camera has an ID
  int cameraID;
  // the arguments to glFrustum...
  // left, right, bottom, top, near, far
  float frustum[6];
  // the arguments to gluLookAt
  // eye x,y,z; center x,y,z; up vector x,y,z
  float lookat[9];
};

#endif
