//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BUMPMAP_NODE_H
#define AR_BUMPMAP_NODE_H

#include "arGraphicsNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Bump map loaded from a file.

class SZG_CALL arBumpMapNode: public arGraphicsNode{
 public:
  arBumpMapNode();
  virtual ~arBumpMapNode(){}

  void draw(arGraphicsContext*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

#endif
