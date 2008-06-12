//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VISIBILITY_NODE_H
#define AR_VISIBILITY_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsCalling.h"

// Toggle visibility of subtree under this node.

class SZG_CALL arVisibilityNode: public arGraphicsNode{
 public:
  arVisibilityNode();
  virtual ~arVisibilityNode() {}

  void draw(arGraphicsContext*) {}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  bool getVisibility();
  void setVisibility(bool visibility);

 protected:
  bool _visibility;
  arStructuredData* _dumpData(bool visibility, bool owned);
};

#endif
