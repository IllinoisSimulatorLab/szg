//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VIEWER_NODE_H
#define AR_VIEWER_NODE_H

#include "arGraphicsNode.h"
#include "arHead.h"

/// Point of view (gaze direction, eye spacing, viewing transformation, etc.).

class SZG_CALL arViewerNode: public arGraphicsNode{
 public:
  arViewerNode();
  ~arViewerNode(){}

  void draw() {}

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arHead* getHead(){ return &_head; }
  void setHead(const arHead& head);

 protected:
  arHead _head;
  arStructuredData* _dumpData(const arHead& head);
};

#endif
