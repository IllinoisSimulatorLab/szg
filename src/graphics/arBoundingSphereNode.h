//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BOUNDING_SPHERE_NODE_H
#define AR_BOUNDING_SPHERE_NODE_H

#include "arGraphicsNode.h"
#include "arRay.h"
#include "arMath.h"
#include "arGraphicsCalling.h"

// Bounding sphere.

class SZG_CALL arBoundingSphereNode: public arGraphicsNode {
 public:
  arBoundingSphereNode();
  virtual ~arBoundingSphereNode() {}

  void draw(arGraphicsContext*);
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arBoundingSphere getBoundingSphere();
  void setBoundingSphere(const arBoundingSphere& b);

 protected:
  arBoundingSphere _boundingSphere;
  arStructuredData* _dumpData(const arBoundingSphere& b, bool owned);
};

#endif
