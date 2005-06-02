//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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
  arSoundFile() : _psamp(NULL), _psampTrigger(NULL) {}
  ~arSoundFile();

  bool read(const char* filename, bool fLoop);
  bool dummy();
  const string& name() const { return _name; } // convenient for debugging
  FSOUND_SAMPLE* psamp() { return _psamp; }

 private:
  string _name;
  FSOUND_SAMPLE* _psamp;        // looped
  FSOUND_SAMPLE* _psampTrigger; // non-looped

  // For dummy sound, when a soundfile fails to load.
  static bool _fInit;
  static short _buf[];
};

#endif
