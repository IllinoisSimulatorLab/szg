//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TRANSFORM_NODE_H
#define AR_TRANSFORM_NODE_H

#include "arGraphicsNode.h"
#include "arMath.h"

/// 4x4 matrix transformation (an articulation in the scene graph).

class arTransformNode: public arGraphicsNode{
 public:
  arTransformNode();
  ~arTransformNode(){}

  void draw();

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  /// Data access functions specific to arTransformNode
  arMatrix4 getTransform();
  void setTransform(const arMatrix4& transform);

 protected:
  arMatrix4 _transform;
  arMutex   _accessLock;

  arStructuredData* _dumpData(const arMatrix4& transform);
};

#endif
