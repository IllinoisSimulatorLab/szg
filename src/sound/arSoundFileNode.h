//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Looped sounds which vary in amplitude and xyz position.

#ifndef AR_SOUNDFILE_NODE_H
#define AR_SOUNDFILE_NODE_H

#include "arSoundNode.h"
#include "arSoundCalling.h"

// Single sound in the scene graph, triggered or looped.
// Prefer arStreamNode for a long piece of music.

class SZG_CALL arSoundFileNode : public arSoundNode{
 public:
  // Needs assignment operator and copy constructor, for pointer member.
  arSoundFileNode();
  ~arSoundFileNode();

  bool render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

 private:
  int _fInit;
#ifdef EnableSound
  FMOD_SOUND* _psamp;
  FMOD_CHANNEL* _channel;
#endif
  // _fLoop is a HACK. It variously means
  //  "start the loop", "pause the loop", "trigger the sound", and
  //  "prepare to retrigger the sound".
  int _fLoop;
  bool _fComplained[4];
  string _fileName;
  string _oldFileName;
  string _action;
  float _triggerAmplitude;
  arVector3 _triggerPoint;

  bool _adjust(bool useTriggered = false);
};

#endif
