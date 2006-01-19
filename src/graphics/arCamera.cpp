//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arCamera.h"
#include "arGraphicsHeader.h"

// Not const because derived classes aren't.
arMatrix4 arCamera::getProjectionMatrix() {
  return ar_identityMatrix();
}

// Not const because derived classes aren't.
arMatrix4 arCamera::getModelviewMatrix() {
  return ar_identityMatrix();
}

void arCamera::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glMultMatrixf(getProjectionMatrix().v);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glMultMatrixf(getModelviewMatrix().v);
}
