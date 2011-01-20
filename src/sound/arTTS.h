//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TTS_H
#define AR_TTS_H

#include "arSoundCalling.h"
#include <string>

#ifndef AR_USE_WIN_32
#undef EnableSpeech
// Windows SAPI is the only implementation so far.
#endif

#ifdef EnableSpeech
#include "arSapi.h"
#endif

// Utterance in the scene graph.

class SZG_CALL arTTS {
  public:
    arTTS();
    ~arTTS();
    
    void init();
    void speak( const std::string& text );

  private:
    void _initVoice();
    void _deleteVoice();
#ifdef EnableSpeech
    ISpVoice* _voice;
#endif
};

#endif
