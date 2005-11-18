//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arOrthoCamera.h"
#include "arGraphicsHeader.h"

arOrthoCamera::arOrthoCamera( const float* const frust, const float* const look ) {
  memcpy( frustum, frust, 6*sizeof(float) );
  memcpy( lookat, look, 9*sizeof(float) );
}
