//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PLAYER_NODE_H
#define AR_PLAYER_NODE_H

#include "arSoundNode.h"
#include "arSoundCalling.h"

// "Point of view" for a sound renderer (3D position, etc.)

class SZG_CALL arPlayerNode:public arSoundNode{
 public:
  arPlayerNode();
  ~arPlayerNode() {}

  bool render() { return true; }

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

#endif
