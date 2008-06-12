//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TRANSFORM_NODE_H
#define AR_TRANSFORM_NODE_H

#include "arGraphicsNode.h"
#include "arMath.h"
#include "arGraphicsCalling.h"

// 4x4 matrix transformation (an articulation in the scene graph).

class SZG_CALL arTransformNode: public arGraphicsNode{
 public:
  arTransformNode();
  virtual ~arTransformNode() {}

  void draw(arGraphicsContext*);

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  // Data access functions specific to arTransformNode
  arMatrix4 getTransform();
  void setTransform(const arMatrix4& transform);

 protected:
  arMatrix4 _transform;

  arStructuredData* _dumpData(const arMatrix4& transform, bool owned);
};

#endif
