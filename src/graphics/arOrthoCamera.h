//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_ORTHO_CAMERA_H
#define AR_ORTHO_CAMERA_H

#include "arCamera.h"

class SZG_CALL arOrthoCamera: public arCamera{
 public:
  arOrthoCamera();
  virtual ~arOrthoCamera(){}

  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();

  // the parameters to glOrtho(...)
  float ortho[6];
  // the parameters to gluLookat(...)
  float lookat[9];
};

#endif
