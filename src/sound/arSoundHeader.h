//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_HEADER_H
#define AR_SOUND_HEADER_H

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arSoundCalling.h"

// IDs: type of an arSoundNode, for efficient casting.

enum{
  AR_S_TRANSFORM_NODE = 0,
  AR_S_FILE_NODE = 1,
  AR_S_PLAYER_NODE = 2,
  AR_S_SPEECH_NODE = 3,
  AR_S_STREAM_NODE = 4
};

#endif
