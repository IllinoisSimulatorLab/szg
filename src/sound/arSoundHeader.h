//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_HEADER_H
#define AR_SOUND_HEADER_H

// These IDs are used by the arSoundNodes. They identify the node type
// and allow for efficient casting.

enum{
  AR_S_TRANSFORM_NODE = 0,
  AR_S_FILE_NODE = 1,
  AR_S_PLAYER_NODE = 2,
  AR_S_SPEECH_NODE = 3,
  AR_S_STREAM_NODE = 4
};

#endif
