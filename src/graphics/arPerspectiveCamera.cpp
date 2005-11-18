//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPerspectiveCamera.h"
#include "arGraphicsHeader.h"

arPerspectiveCamera::arPerspectiveCamera(){
  // some sensible defaults
  frustum[4] = 0.1;
}

arPerspectiveCamera::arPerspectiveCamera( const float* const frust, const float* const look, int ID ) {
  memcpy( frustum, frust, 6*sizeof(float) );
  memcpy( lookat, look, 9*sizeof(float) );
  cameraID = ID;
}

arMatrix4 arPerspectiveCamera::getProjectionMatrix() const {
  const float& l = frustum[0];
  const float& r = frustum[1];
  const float& b = frustum[2];
  const float& t = frustum[3];
  const float& n = frustum[4];
  const float& f = frustum[5];
  
  // Lifted from the OpenGL textbook.
  return arMatrix4( 2*n/(r-l),   0,           (r+l)/(r-l),     0,
                    0,           2*n/(t-b),   (t+b)/(t-b),     0,
                    0,           0,           -(f+n)/(f-n),    -2*f*n/(f-n),
                    0,           0,           -1,              0);
}
