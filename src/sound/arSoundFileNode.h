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
  int _chan;
  float _amplitudePrev;
  arVector3 _pointPrev;
  int _fLoop; // 0 or 1, normally;  -1 means trigger NOW.
  bool _fComplained[2];
  inline void _adjust(bool fForce = false);

  // Keep track of which channel each node uses.
  enum { _numchans = 30 };
  static bool _chanUsed[_numchans]; // initially all false
  int _getChan();
  void _useChan(int chan) { _chanUsed[chan] = true; }
  void _freeChan(int chan) { _chanUsed[chan] = false; }
  void _loopStart();
  void _loopStop();
  void _triggerStart();
};

#endif
