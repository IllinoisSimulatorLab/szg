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
  frustum[0] = -1;
  frustum[1] = 1;
  frustum[2] = -1;
  frustum[3] = 1;
  frustum[4] = 0.1;
  frustum[5] = 1;

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

arMatrix4 arPerspectiveCamera::getProjectionMatrix(){
  float l = frustum[0];
  float r = frustum[1];
  float b = frustum[2];
  float t = frustum[3];
  float n = frustum[4];
  float f = frustum[5];
  
  // lifted from the OpenGL textbook
  return arMatrix4( 2*n/(r-l),   0,           (r+l)/(r-l),     0,
                    0,           2*n/(t-b),   (t+b)/(t-b),     0,
                    0,           0,           -(f+n)/(f-n),    -2*f*n/(f-n),
                    0,           0,           -1,              0);
}

arMatrix4 arPerspectiveCamera::getModelviewMatrix(){
  return ar_lookatMatrix(arVector3(lookat[0], lookat[1], lookat[2]),
			 arVector3(lookat[3], lookat[4], lookat[5]),
			 arVector3(lookat[6], lookat[7], lookat[8]));
}

void arPerspectiveCamera::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  // AARGH! the projection matrix calculation above is wrong!
  glFrustum(frustum[0], frustum[1], frustum[2],
	    frustum[3], frustum[4], frustum[5]);
  //arMatrix4 temp = getProjectionMatrix();
  //glMultMatrixf( temp.v );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  arMatrix4 temp = getModelviewMatrix();
  glMultMatrixf( temp.v );
}
