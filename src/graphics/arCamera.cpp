//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arCamera.h"
#include "arGraphicsHeader.h"

arMatrix4 arCamera::getProjectionMatrix() {
  return ar_identityMatrix();
}

arMatrix4 arCamera::getModelviewMatrix() {
  return ar_identityMatrix();
}

void arCamera::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  arMatrix4 temp = getProjectionMatrix();
  glMultMatrixf( temp.v );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  temp = getModelviewMatrix();
  glMultMatrixf( temp.v );
}

