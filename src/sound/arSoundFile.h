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
#include "arSoundCalling.h"

using namespace std; // for "string"

// Soundfile to play (in the scene graph).

class SZG_CALL arSoundFile {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arSoundFile()
#ifdef EnableSound
    : _psamp(NULL), _psampTrigger(NULL)
#endif
    {}
  ~arSoundFile();

  bool read(const char* filename, bool fLoop, const int renderMode);
  bool dummy();
  const string& name() const { return _name; } // for debugging
#ifdef EnableSound
  FMOD_SOUND* psamp() { return _psamp; }
#endif

 private:
  string _name;
#ifdef EnableSound
  FMOD_SOUND* _psamp;        // looped
  FMOD_SOUND* _psampTrigger; // non-looped
#endif
};

#endif
