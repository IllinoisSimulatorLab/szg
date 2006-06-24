//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STREAM_NODE_H
#define AR_STREAM_NODE_H

#include "arSoundNode.h"
#include "arSoundDatabase.h"
#include "arSoundCalling.h"

/// Streams a file from disk. This is good for long pieces of music,
/// whereby arSoundFileNode is better for short clips that are either
/// triggered or are looped.

class SZG_CALL arStreamNode: public arSoundNode{
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
  FMOD::Channel* _channel;
  FMOD::Sound* _stream;
#else
  void* _channel;
  void* _stream;
#endif

  // Used to determine if a change to this node should open up a new stream.
  string _fileName;
  // For detecting if the stream source has changed after receiveData().
  string _fileNamePrev;
  bool _paused;

  // What is the current loundness of the sound?
  // Here, we follow the convention of arSoundFileNode, where amplitude
  // can range between 0 and 100, with 1 giving the sound at its
  // default loudness.
  float _amplitude;
  int _msecRequested;
  unsigned _msecNow;
  unsigned _msecDuration;
  bool _complained;
};

#endif
