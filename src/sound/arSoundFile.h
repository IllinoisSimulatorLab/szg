//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUNDFILE_H
#define AR_SOUNDFILE_H

#include <stdlib.h>
#include <string>
#include <iostream>
#include "fmodStub.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arSoundCalling.h"
using namespace std; // needed for "string"

/// Soundfile to play (in the scene graph).

class SZG_CALL arSoundFile {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arSoundFile()
#ifdef EnableSound
    : _psamp(NULL), _psampTrigger(NULL)
#endif
    {}
  ~arSoundFile();

  bool read(const char* filename, bool fLoop);
  bool dummy();
  const string& name() const { return _name; } // convenient for debugging
#ifdef EnableSound
  FMOD::Sound* psamp() { return _psamp; }
#endif

 private:
  string _name;
#ifdef EnableSound
  FMOD::Sound* _psamp;        // looped
  FMOD::Sound* _psampTrigger; // non-looped
#endif

  // For dummy sound, when a soundfile fails to load.
  static bool _fInit;
  static short _buf[];
};

#endif
