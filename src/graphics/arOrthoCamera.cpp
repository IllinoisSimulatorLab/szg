//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arOrthoCamera.h"
#include "arGraphicsHeader.h"

arOrthoCamera::arOrthoCamera(){
  // sensible defaults
  ortho[0] = -1;
  ortho[1] = 1;
  ortho[2] = -1;
  ortho[3] = 1;
  ortho[4] = -1;
  ortho[5] = 1;

  lookat[0] = 0;
  lookat[1] = 0;
  lookat[2] = 0;
  lookat[3] = 0;
  lookat[4] = 0;
  lookat[5] = -1;
  lookat[6] = 0;
  lookat[7] = 1;
  lookat[8] = 0;
}

arOrthoCamera::arOrthoCamera( const float* const orth, const float* const look ) {
  memcpy( ortho, orth, 6*sizeof(float) );
  memcpy( lookat, look, 9*sizeof(float) );
}

arMatrix4 arOrthoCamera::getProjectionMatrix(){
  float l = ortho[0];
  float r = ortho[1];
  float b = ortho[2];
  float t = ortho[3]; 
  float n = ortho[4];
  float f = ortho[5];
 
  // The default OpenGL viewing cube is centered on the origin,
  // with the negative z axis into the screen, up the positive
  // y axis, and x to the right. The clipping planes are the coordinate
  // planes at 1 and -1 for each axis.
  return arMatrix4(2/(r-l),  0,         0,         (r+l)/(r-l),
                   0,        2/(t-b),   0,         (t+b)/(t-b),
                   0,        0,         -2/(f-n),  (f+n)/(n-f),
                   0,        0,         0,         1);
}

arMatrix4 arOrthoCamera::getModelviewMatrix(){
  return ar_lookatMatrix(arVector3(lookat[0], lookat[1], lookat[2]),
			 arVector3(lookat[3], lookat[4], lookat[5]),
			 arVector3(lookat[6], lookat[7], lookat[8]));
}

void arOrthoCamera::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  arMatrix4 temp = getProjectionMatrix();
  glMultMatrixf( temp.v );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  temp = getModelviewMatrix();
  glMultMatrixf( temp.v );
}
