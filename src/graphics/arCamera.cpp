//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arCamera.h"
#include "arGraphicsHeader.h"

// Not const because derived classes aren't.
arMatrix4 arCamera::getProjectionMatrix() {
  return arMatrix4();
}

// Not const because derived classes aren't.
arMatrix4 arCamera::getModelviewMatrix() {
  return arMatrix4();
}

void arCamera::loadViewMatrices() {
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glMultMatrixf(getProjectionMatrix().v);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glMultMatrixf(getModelviewMatrix().v);
}
