//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PLAYER_NODE_H
#define AR_PLAYER_NODE_H

#include "arSoundNode.h"

/// "Point of view" for a sound renderer (3D position, etc.)

class arPlayerNode:public arSoundNode{
 public:
  arPlayerNode();
  ~arPlayerNode(){}

  void render() {}

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

#endif
