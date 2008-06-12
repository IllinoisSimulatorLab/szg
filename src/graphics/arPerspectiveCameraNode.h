//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PERSPECTIVE_CAMERA_NODE_H
#define AR_PERSPECTIVE_CAMERA_NODE_H

#include "arGraphicsNode.h"
#include "arPerspectiveCamera.h"
#include "arGraphicsCalling.h"

class SZG_CALL arPerspectiveCameraNode:public arGraphicsNode{
 public:
  arPerspectiveCameraNode();
  virtual ~arPerspectiveCameraNode() {}

  void draw(arGraphicsContext*) {}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
  void deactivate();

  arPerspectiveCamera getCamera();
  void setCamera(const arPerspectiveCamera& camera);

 protected:
  arPerspectiveCamera _nodeCamera;
  arStructuredData* _dumpData(const arPerspectiveCamera& camera, bool owned);
};

#endif
