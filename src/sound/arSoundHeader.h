//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_HEADER_H
#define AR_SOUND_HEADER_H

#include "arSoundCalling.h"

// IDs: type of an arSoundNode, for efficient casting.

enum{
  AR_S_TRANSFORM_NODE = 0,
  AR_S_FILE_NODE,
  AR_S_PLAYER_NODE,
  AR_S_SPEECH_NODE,
  AR_S_STREAM_NODE
};

#endif
