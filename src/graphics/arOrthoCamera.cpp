//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arOrthoCamera.h"
#include "arGraphicsHeader.h"

arOrthoCamera::arOrthoCamera( const float* const frust, const float* const look ) {
  memcpy( frustum, frust, 6*sizeof(float) );
  memcpy( lookat, look, 9*sizeof(float) );
}
