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
 
  // from the OpenGL textbook
  return arMatrix4(2/(r-l),  0,         0,         (r+l)/(r-l),
                   0,        2/(t-b),   0,         (t+b)/(t-b),
                   0,        0,         -2/(f-n),  (f+n)/(f-n),
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
  // THIS DOES NOT WORK!
  //arMatrix4 temp = getProjectionMatrix();
  //glMultMatrixf( temp.v );
  glOrtho(ortho[0], ortho[1], ortho[2], ortho[3], ortho[4], ortho[5]);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  arMatrix4 temp = getModelviewMatrix();
  glMultMatrixf( temp.v );
}
