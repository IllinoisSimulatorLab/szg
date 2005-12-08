//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PERSPECTIVE_CAMERA_H
#define AR_PERSPECTIVE_CAMERA_H

#include "arFrustumCamera.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"
#include <string>

class SZG_CALL arPerspectiveCamera: public arFrustumCamera{
 public:
  arPerspectiveCamera();
  arPerspectiveCamera(const float* const frust, const float* const look, int ID=-2);
  virtual arCamera* clone() const
    { return (arCamera*)new arPerspectiveCamera( frustum, lookat, cameraID ); }

  virtual arMatrix4 getProjectionMatrix();
  virtual std::string type( void ) const { return "arPerspectiveCamera"; }

  // hack to export values to python
  float getFrustumData(int i) const { return frustum[i]; }
  float getLookatData (int i) const { return lookat[i];  }

  // each camera has an ID
  int cameraID;

  // arguments to glFrustum(): left, right, bottom, top, near, far
  float frustum[6];

  // arguments to gluLookAt(): eye x,y,z; center x,y,z; up vector x,y,z
  float lookat[9];
};

#endif
