//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PERSPECTIVE_CAMERA_H
#define AR_PERSPECTIVE_CAMERA_H

#include "arFrustumCamera.h"
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
};

#endif
