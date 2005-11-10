//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PERSPECTIVE_CAMERA_H
#define AR_PERSPECTIVE_CAMERA_H

#include "arCamera.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"
#include <string>

class SZG_CALL arPerspectiveCamera: public arCamera{
 public:
  arPerspectiveCamera();
  arPerspectiveCamera( const float* const frust, const float* const look, int ID=-2 );
  virtual ~arPerspectiveCamera(){}
  virtual arCamera* clone() const { return (arCamera*)new arPerspectiveCamera( frustum, lookat, cameraID ); }

  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();
  virtual std::string type( void ) const { return "arPerspectiveCamera"; }

  void setFrustum( float* frust ) { memcpy( frustum, frust, 6*sizeof(float) ); }
  void setLook( float* look ) { memcpy( lookat, look, 9*sizeof(float) ); }

  void setSides( arVector4& sides ) { setSides( sides[ 0 ], sides[ 1 ],
                                                sides[ 2 ], sides[ 3 ] ); }

  void setSides(float left, float right, float bottom, float top){
    frustum[0] = left;
    frustum[1] = right;
    frustum[2] = bottom;
    frustum[3] = top;
  }

  arVector4 getSides() const { return arVector4(frustum[0], frustum[1],
					 frustum[2], frustum[3]); }

  void setNearFar(float nearClip, float farClip){
    frustum[4] = nearClip;
    frustum[5] = farClip;
  }

  float getNear() const { return frustum[4]; }
  float getFar() const { return frustum[5]; }

  void setPosition( arVector3& pos ) { setPosition( pos[ 0 ], pos[ 1 ], pos[ 2 ] ); }

  void setPosition(float x, float y, float z){
    lookat[0] = x;
    lookat[1] = y;
    lookat[2] = z;
  }

  arVector3 getPosition() const { return arVector3(lookat[0], lookat[1], lookat[2]); }

  void setTarget( arVector3& target ) { setTarget( target[ 0 ], target[ 1 ], target[ 2 ] ); }

  void setTarget(float x, float y, float z){
    lookat[3] = x;
    lookat[4] = y;
    lookat[5] = z;
  }
 
  arVector3 getTarget(){ return arVector3(lookat[3], lookat[4], lookat[5]); }

  void setUp( arVector3& up ) { setUp( up[ 0 ], up[ 1 ], up[ 2 ] ); }

  void setUp(float x, float y, float z){
    lookat[6] = x;
    lookat[7] = y;
    lookat[8] = z;
  }

  arVector3 getUp() const { return arVector3(lookat[6], lookat[7], lookat[8]); }

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
