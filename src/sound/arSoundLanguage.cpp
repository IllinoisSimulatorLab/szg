//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundLanguage.h"

arSoundLanguage::arSoundLanguage():
  _transform("transform"),
  _fileWav("fileWav"),
  _player("player"),
  _speech("speech"),
  _stream("stream"),
  _soundAdmin("sound_admin"){
  
  AR_TRANSFORM_ID = _transform.add("ID",AR_INT);
  AR_TRANSFORM_MATRIX = _transform.add("matrix",AR_FLOAT);
  AR_TRANSFORM = _dictionary.add(&_transform);

  AR_FILEWAV_ID = _fileWav.add("ID",AR_INT);
  AR_FILEWAV_FILE = _fileWav.add("file",AR_CHAR);
  AR_FILEWAV_LOOP = _fileWav.add("loop",AR_INT);
  AR_FILEWAV_AMPL = _fileWav.add("amplitude",AR_FLOAT);
  AR_FILEWAV_XYZ = _fileWav.add("xyz",AR_FLOAT);
  AR_FILEWAV = _dictionary.add(&_fileWav);

  AR_PLAYER_ID = _player.add("ID", AR_INT);
  AR_PLAYER_MATRIX = _player.add("matrix", AR_FLOAT);
  AR_PLAYER_MID_EYE_OFFSET = _player.add("mid eye offset", AR_FLOAT);
  AR_PLAYER_UNIT_CONVERSION = _player.add("unit conversion", AR_FLOAT);
  AR_PLAYER = _dictionary.add(&_player);
  
  AR_SPEECH_ID = _speech.add("ID", AR_INT);
  AR_SPEECH_TEXT = _speech.add("text",AR_CHAR);
  AR_SPEECH = _dictionary.add(&_speech);

  AR_STREAM_ID = _stream.add("ID", AR_INT);
  AR_STREAM_FILE = _stream.add("file", AR_CHAR);
  AR_STREAM_PAUSED = _stream.add("paused", AR_INT);
  AR_STREAM_AMPLITUDE = _stream.add("amplitude", AR_FLOAT);
  AR_STREAM_TIME = _stream.add("time", AR_INT);
  AR_STREAM = _dictionary.add(&_stream);

  AR_SOUND_ADMIN_ACTION = _soundAdmin.add("action", AR_CHAR);
  AR_SOUND_ADMIN_NODE_ID = _soundAdmin.add("node_ID", AR_INT);
  AR_SOUND_ADMIN_NAME = _soundAdmin.add("name", AR_CHAR);
  AR_SOUND_ADMIN = _dictionary.add(&_soundAdmin);
}

const char* arSoundLanguage::_stringFromID(int id)
{
  // This function is slow, but that's okay: it's only for debugging printf's.
  const int cnames = 8;
  static const char* names[cnames+1] = {
    "AR_FILEWAV",
    "AR_TRANSFORM",
    "AR_PLAYER",
    "AR_SPEECH",
    "AR_STREAM",
    "AR_MAKE_NODE",
    "AR_SOUND_ADMIN"
    "(unknown!)"
    };
  const int ids[cnames] = {
    AR_FILEWAV,
    AR_TRANSFORM,
    AR_PLAYER,
    AR_SPEECH,
    AR_STREAM,
    AR_MAKE_NODE,
    AR_SOUND_ADMIN
    };
  for (int i=0; i<cnames; ++i)
    if (id == ids[i])
      return names[i];
  return names[cnames]; // id was not found
}
