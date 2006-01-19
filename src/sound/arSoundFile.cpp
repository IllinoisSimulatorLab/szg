//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundFile.h"
#include <fcntl.h>
#include <math.h>

#ifdef AR_USE_WIN_32
#include <io.h>
#include <iostream>
#else
#include <unistd.h>
#endif

// The FSOUND_XXX() calls in read() will segmentation-fault if FMOD hasn't
// been initialized yet in arSoundClient.cpp.

bool arSoundFile::read(const char* filename, bool fLoop) {
  int fd = filename ? open(filename, O_RDONLY) : -1;
  if (fd < 0){
    // Don't complain here.  Our caller arSoundDatabase::addFile()
    // will probably try another filename from the path in a moment.
    return false;
  }

  close(fd);
  _name = filename;
#ifndef EnableSound
  fLoop = 0; // avoid compiler warning
#endif

// FMOD 3.7 doesn't yet link with C++ code on the Mac and this
// function has a different signature
#ifdef USE_FMOD_3_6
  _psamp = FSOUND_Sample_Load(FSOUND_FREE, filename,
    FSOUND_SIGNED | FSOUND_16BITS | FSOUND_MONO |
      (fLoop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF), 0);
#else
  // NOTE: for stereo samples (like mp3's), fmod will not spatialize the
  // sound unless you force it to be mono w/ the FSOUND_FORCEMONO option.
  // This is new if fmod 3.6 or 3.7, not sure which. 
  // TO DO: we might like to be able to play non-spatialized sounds in stereo
  // sometime.... a new option or too will need to be added to do this.
  _psamp = FSOUND_Sample_Load(FSOUND_FREE, filename, 
    FSOUND_SIGNED | FSOUND_16BITS | FSOUND_MONO | FSOUND_FORCEMONO |
      (fLoop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF), 0, 0);
#endif

  if (!_psamp) {
    cerr << "arSoundFile error: failed to load file \""
         << filename << "\",\n"
#ifdef AR_USE_WIN_32
         << "\t(Check control panel's Preferred Device settings.)\n";
#else
	 << "\tmaybe wrong format (need .wav or .mp3).\n";
#endif
    return dummy();
    // FMOD supports wav mp3 ogg raw and others, but not aiff.
    // FMOD automatically converts stereo files to mono.
  }
  return true;
}

bool arSoundFile::_fInit = false;
short arSoundFile::_buf[6000];

bool arSoundFile::dummy(){
  // Create a special _psamp to indicate a missing .wav file.
  if (!_fInit) {
    _fInit = true;
    const int csamp = sizeof(_buf) / sizeof(short);
    for (int i=0; i<csamp; ++i) {
      const float ramp = 1. - i / float(csamp); // 0 to 1
      const float ampl = 20000. * (.1 + .9 * ramp);
      const float period = 200. + 200. * ramp;
      _buf[i] = short(ampl * sin(2.*3.141592 * ramp * period));
    }
  }

  // FMOD 3.7 doesn't yet link with C++ code on the Mac and the signature
  // for this function has changed
#ifdef USE_FMOD_3_6
  _psamp = FSOUND_Sample_Load(FSOUND_FREE, (const char*)_buf,
    FSOUND_SIGNED | FSOUND_16BITS | FSOUND_MONO | FSOUND_LOOP_OFF |
      FSOUND_LOADMEMORY | FSOUND_LOADRAW,0);
#else
  _psamp = FSOUND_Sample_Load(FSOUND_FREE, (const char*)_buf,
    FSOUND_SIGNED | FSOUND_16BITS | FSOUND_MONO | FSOUND_LOOP_OFF |
      FSOUND_LOADMEMORY | FSOUND_LOADRAW,0,0);
#endif
    // What sampling rate does it assume?
  return _psamp != NULL;
}

arSoundFile::~arSoundFile(){
  if (_psamp)
    FSOUND_Sample_Free(_psamp);
}
