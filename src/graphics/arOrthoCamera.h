//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_ORTHO_CAMERA_H
#define AR_ORTHO_CAMERA_H

#include "arFrustumCamera.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"
#include <string>

class SZG_CALL arOrthoCamera: public arFrustumCamera{
 public:
  // arFrustumCamera constructor already gives reasonable arOrthoCamera 
  // defaults.
  arOrthoCamera(){}
  arOrthoCamera(const float* const frust, const float* const look);
  virtual arCamera* clone() const
    { return (arCamera*) new arOrthoCamera( frustum, lookat ); }

  virtual std::string type( void ) const { return "arOrthoCamera"; }
};

#endif
