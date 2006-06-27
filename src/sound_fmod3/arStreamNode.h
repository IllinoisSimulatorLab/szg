//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STREAM_NODE_H
#define AR_STREAM_NODE_H

#include "arSoundNode.h"
#include "arSoundDatabase.h"
#include "arSoundCalling.h"

// Stream a file from disk. This is good for long pieces of music,
// whereby arSoundFileNode is better for short clips that are either
// triggered or are looped.

class SZG_CALL arStreamNode: public arSoundNode{
 public:
  arStreamNode();
  ~arStreamNode();

  void render();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  int getCurrentTime(){ return _currentTime; }
  int getStreamLength(){ return _streamLength; }

 private:
  // The FMOD channel upon which sound is playing.
  int _channel;
  // The FMOD stream
#ifdef EnableSound
  FSOUND_STREAM* _stream;
#else
  void* _stream;
#endif
  // The file name of the stream. By using this, we can determine if a
  // change to this node should open up a new stream or not.
  string _fileName;
  // The previous file name. Allows us to detect if the stream source has
  // changed or not after a receiveData()...
  string _oldFileName;
  // Is the sound running or not?
  int   _paused;
  // What is the current loundness of the sound?
  // Here, we follow the convention of arSoundFileNode, where amplitude
  // can range between 0 and 100, with 1 giving the sound at its
  // default loudness.
  float  _amplitude;
  // This is the requested time of the stream
  int    _requestedTime;
  // This is the current in the stream, in milliseconds.
  int    _currentTime;
  // This is the length of the stream, in milliseconds.
  int    _streamLength;
  bool   _complained;
};

#endif
