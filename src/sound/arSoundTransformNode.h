//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_TRANSFORM_NODE_H
#define AR_SOUND_TRANSFORM_NODE_H

#include "arSoundNode.h"
#include "arSoundCalling.h"

// OpenGL-style transformation matrix in the scene graph for sound.

class SZG_CALL arSoundTransformNode : public arSoundNode{
 public:
  arSoundTransformNode();
  ~arSoundTransformNode() {}

  bool render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

#endif
