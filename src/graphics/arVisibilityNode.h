//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VISIBILITY_NODE_H
#define AR_VISIBILITY_NODE_H

#include "arGraphicsNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Toggle visibility of subtree under this node.

class SZG_CALL arVisibilityNode: public arGraphicsNode{
 public:
  arVisibilityNode();
  ~arVisibilityNode(){}

  void draw(arGraphicsNode*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  bool getVisibility();
  void setVisibility(bool visibility);

 protected:
  bool _visibility;
  arStructuredData* _dumpData(bool visibility);
};

#endif
