//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundFile.h"

#include <fcntl.h>
#include <math.h>
#ifdef AR_USE_WIN_32
  #include <io.h>
#else
  #include <unistd.h>
#endif

bool arSoundFile::read(const char* filename, bool fLoop) {
  // bug: check that fmod's been init()'d in arSoundClient.cpp, to avoid a crash.

  // If can't read file, don't complain.  Our caller arSoundDatabase::addFile()
  // may try another filename from the path in a moment.
  if (!filename)
    return false;
  const int fd = open(filename, O_RDONLY);
  if (fd < 0)
    return false;

  close(fd);
  _name = filename;

#ifndef EnableSound
  fLoop = 0; // avoid compiler warning
#endif

#ifdef EnableSound
  if (!ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), filename,
    FMOD_3D | (fLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF),
    0, &_psamp)) || !_psamp) {
    cerr << "arSoundFile error: failed to load file '" << filename << "'.\n";
#ifdef AR_USE_WIN_32
    cerr << "  (Check Windows' sound control panel's Preferred Device settings.)\n";
#endif
    return dummy();
  }
#endif
  return true;
}

bool arSoundFile::_fInit = false;
short arSoundFile::_buf[6000];

bool arSoundFile::dummy(){
  // A special _psamp to indicate a missing .wav file.
  if (!_fInit) {
    _fInit = true;
    const int csamp = sizeof(_buf) / sizeof(*_buf);
    for (int i=0; i<csamp; ++i) {
      const float ramp = 1. - i / float(csamp); // 0 to 1
      const float ampl = 20000 * 0.6 * (.1 + .9 * ramp);
      const float period = 200. + 200. * ramp;
      _buf[i] = short(ampl * sin(2.*M_PI * ramp * period));
    }
  }

#ifdef EnableSound
  FMOD_CREATESOUNDEXINFO ex = {0};
  ex.cbsize = sizeof(ex);
  ex.length = sizeof(_buf);
  ex.numchannels = 1;
  ex.defaultfrequency = 44100;
  ex.format = FMOD_SOUND_FORMAT_PCM16; // PCMFLOAT and float-not-short oughta work?
  return ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), 
      (const char*)_buf, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &ex, &_psamp)) &&
    _psamp != NULL;
#else
  return true;
#endif
}

arSoundFile::~arSoundFile(){
#ifdef EnableSound
  if (_psamp)
    (void)ar_fmodcheck( FMOD_Sound_Release( _psamp ));
#endif
}
