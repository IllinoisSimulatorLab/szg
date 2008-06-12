//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundFile.h"

#include "arSoundDatabase.h" // for mode_vss etc.

#include <fcntl.h>
#include <math.h>
#ifdef AR_USE_WIN_32
  #include <io.h>
#else
  #include <unistd.h>
#endif

bool arSoundFile::read(const char* filename, bool fLoop, const int renderMode) {
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
  fLoop = renderMode; // avoid compiler warning
#endif

#ifdef EnableSound
  if (renderMode == mode_fmodplugins) {
    // copypaste from FMOD_3D case
    if (!ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), filename,
	FMOD_SOFTWARE, // FMOD_2D | (fLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF),
	0, &_psamp)) || !_psamp) {
      goto LFailFmod;
    }
    return true;
  }

  if (!ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), filename,
    FMOD_3D | (fLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF), 0, &_psamp)) || !_psamp) {
LFailFmod:
    ar_log_error() << "arSoundFile failed to load file '" << filename << "'.\n";
#ifdef AR_USE_WIN_32
    ar_log_error() << "  (Check Windows' sound control panel's Preferred Device settings.)\n";
#endif
    return dummy();
  }
#endif
  return true;
}

#ifdef EnableSound
static FMOD_RESULT F_CALLBACK pcmreadcallback(FMOD_SOUND *sound, void *data, unsigned cb)
{
  const unsigned cs = cb/2;
  signed short *ps = (signed short *)data;
  static float t1 = 0., v1 = 0.;
  for (unsigned is=0; is<cs; ++is)
  {
    *ps++ = (signed short)(sin(t1) * 20000.);
    t1 += .001 + v1;
    v1 += sin(t1) * .0002;
  }

  return FMOD_OK;
}

static FMOD_RESULT F_CALLBACK pcmsetposcallback(FMOD_SOUND *, int, unsigned, FMOD_TIMEUNIT)
{
    return FMOD_OK;
}
#endif

bool arSoundFile::dummy() {
#ifdef EnableSound
  FMOD_CREATESOUNDEXINFO ex = {0};
  ex.cbsize = sizeof(ex);
  ex.decodebuffersize = 44100;
  ex.length = 44100;
  ex.numchannels = 1;
  ex.defaultfrequency = 44100;
  ex.format = FMOD_SOUND_FORMAT_PCM16;
  ex.pcmreadcallback = pcmreadcallback;
  ex.pcmsetposcallback = pcmsetposcallback;
  return ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(),
      0, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &ex, &_psamp)) &&
    _psamp != NULL;
#else
  return true;
#endif
}

arSoundFile::~arSoundFile() {
#ifdef EnableSound
  if (_psamp)
    (void)ar_fmodcheck( FMOD_Sound_Release( _psamp ));
#endif
}
