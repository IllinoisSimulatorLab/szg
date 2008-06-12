//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VIEWER_NODE_H
#define AR_VIEWER_NODE_H

#include "arGraphicsNode.h"
#include "arHead.h"
#include "arGraphicsCalling.h"

// Point of view (gaze direction, eye spacing, viewing transformation, etc.).
// A friend of arHead.

class SZG_CALL arViewerNode: public arGraphicsNode{
 public:
  arViewerNode();
  virtual ~arViewerNode() {}

  void draw(arGraphicsContext*) {}

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arHead* getHead() { return &_head; }
  void setHead(const arHead& head);

 protected:
  arHead _head;
  arStructuredData* _dumpData(const arHead& head, const bool owned);
};

#endif
