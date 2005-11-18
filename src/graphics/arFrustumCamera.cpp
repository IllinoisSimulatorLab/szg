//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFrustumCamera.h"
#include "arGraphicsHeader.h"

arFrustumCamera::arFrustumCamera(){
  // sensible defaults
  frustum[0] = -1;
  frustum[1] = 1;
  frustum[2] = -1;
  frustum[3] = 1;
  frustum[4] = -1;
  frustum[5] = 1;

  memset(lookat, 0, sizeof(lookat));
  lookat[5] = -1;
  lookat[7] = 1;
}

arFrustumCamera::arFrustumCamera( const float* const frust, const float* const look ) {
  memcpy( frustum, frust, 6*sizeof(float) );
  memcpy( lookat, look, 9*sizeof(float) );
}

arMatrix4 arFrustumCamera::getProjectionMatrix() const {
  const float& l = frustum[0];
  const float& r = frustum[1];
  const float& b = frustum[2];
  const float& t = frustum[3];
  const float& n = frustum[4];
  const float& f = frustum[5];

  // The default OpenGL viewing cube is centered on the origin,
  // with the negative z axis into the screen, up the positive
  // y axis, and x to the right. The clipping planes are the coordinate
  // planes at 1 and -1 for each axis.
  return arMatrix4(2/(r-l),  0,         0,         (r+l)/(r-l),
                   0,        2/(t-b),   0,         (t+b)/(t-b),
                   0,        0,         -2/(f-n),  (f+n)/(n-f),
                   0,        0,         0,         1);
}

arMatrix4 arFrustumCamera::getModelviewMatrix() const {
  return ar_lookatMatrix(arVector3(lookat),
			 arVector3(lookat+3),
			 arVector3(lookat+6));
}

void arFrustumCamera::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glMultMatrixf( getProjectionMatrix().v );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glMultMatrixf( getModelviewMatrix().v );
}
