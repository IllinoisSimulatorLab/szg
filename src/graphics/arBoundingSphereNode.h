//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BOUNDING_SPHERE_NODE_H
#define AR_BOUNDING_SPHERE_NODE_H

#include "arGraphicsNode.h"
#include "arRay.h"
#include "arMath.h"

/// Bounding sphere.

class SZG_CALL arBoundingSphereNode: public arGraphicsNode {
 public:
  arBoundingSphereNode();
  ~arBoundingSphereNode(){}

  void draw();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arBoundingSphere getBoundingSphere(){ return _boundingSphere; }
  void setBoundingSphere(const arBoundingSphere& b);

 protected:
  arBoundingSphere _boundingSphere;
  arStructuredData* _dumpData(const arBoundingSphere& b);
};

#endif
