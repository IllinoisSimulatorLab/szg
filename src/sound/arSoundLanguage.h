//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_LANGUAGE_H
#define AR_SOUND_LANGUAGE_H

#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arDatabaseLanguage.h"

/// Language for the sound scene graph.

class SZG_CALL arSoundLanguage:public arDatabaseLanguage{
 public:
  arSoundLanguage();
  ~arSoundLanguage(){}

 // can these ints be made non-public?
  int AR_TRANSFORM;
  int AR_TRANSFORM_ID;
  int AR_TRANSFORM_MATRIX;

  int AR_FILEWAV;
  int AR_FILEWAV_ID;
  int AR_FILEWAV_FILE;
  int AR_FILEWAV_LOOP;
  int AR_FILEWAV_AMPL;
  int AR_FILEWAV_XYZ;

  int AR_PLAYER;
  int AR_PLAYER_ID;
  // transformation matrix of head
  int AR_PLAYER_MATRIX;     
  // distance from motiontracking sensor to midpoint between eyes?     
  int AR_PLAYER_MID_EYE_OFFSET;  
  int AR_PLAYER_UNIT_CONVERSION;
  
  int AR_SPEECH;
  int AR_SPEECH_ID;
  int AR_SPEECH_TEXT;

  int AR_STREAM;
  int AR_STREAM_ID;
  int AR_STREAM_FILE;
  int AR_STREAM_PAUSED;
  int AR_STREAM_AMPLITUDE;
  int AR_STREAM_TIME;

 protected:
  arDataTemplate _transform;
  arDataTemplate _fileWav;
  arDataTemplate _player;
  arDataTemplate _speech;
  arDataTemplate _stream;

public:
  const char* _stringFromID(int id);
};

#endif
