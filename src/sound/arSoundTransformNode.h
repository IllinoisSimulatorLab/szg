//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_TRANSFORM_NODE_H
#define AR_SOUND_TRANSFORM_NODE_H

#include "arSoundNode.h"

/// OpenGL-style transformation matrix in the scene graph for sound.

class arSoundTransformNode : public arSoundNode{
 public:
  arSoundTransformNode();
  ~arSoundTransformNode(){}

  void render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

#endif
