//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// Looped sounds which vary in amplitude and xyz position.

#ifndef AR_SOUNDFILE_NODE_H
#define AR_SOUNDFILE_NODE_H

#include "arSoundNode.h"

/// Single sound in the scene graph.

class arSoundFileNode : public arSoundNode{
 public:
  // Needs assignment operator and copy constructor, for pointer member.
  arSoundFileNode();
  ~arSoundFileNode();

  void render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

 private:
  int _fInit;
  FSOUND_SAMPLE* _psamp;
  // The fmod channel upon which the sound is playing.
  int _channel;
  float _amplitudePrev;
  arVector3 _pointPrev;
  int _fLoop; // 0 or 1, normally;  -1 means trigger NOW.
  bool _fComplained[2];
  string _fileName;
  string _oldFileName;
  string _action;

  void _adjust(bool fForce = false);
};

#endif
