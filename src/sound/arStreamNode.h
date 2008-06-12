//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STREAM_NODE_H
#define AR_STREAM_NODE_H

#include "arSoundNode.h"
#include "arSoundDatabase.h"
#include "arSoundCalling.h"

// Stream a (long music) file from disk.
// Prefer arSoundFileNode for a short clip that is triggered or looped.

class SZG_CALL arStreamNode: public arSoundNode {
 public:
  arStreamNode();
  ~arStreamNode();

  bool render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  int getCurrentTime() const
    { return _msecNow; }
  int getStreamLength() const
    { return _msecDuration; }

 private:
#ifdef EnableSound
  FMOD_CHANNEL* _channel;
  FMOD_SOUND* _stream;
#else
  void* _channel;
  void* _stream;
#endif

  // For determining if a change to this node should open up a new stream.
  string _fileName;
  // For detecting if the stream source has changed after receiveData().
  string _fileNamePrev;
  bool _paused;

  // Current loudness.
  // Like arSoundFileNode::render, amplitude is in [0, 100], with 1 the default.
  float _amplitude;

  int _msecRequested;
  unsigned _msecNow;
  unsigned _msecDuration;
  bool _complained;
};

#endif
