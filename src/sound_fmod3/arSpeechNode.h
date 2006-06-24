//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPEECH_NODE_H
#define AR_SPEECH_NODE_H

#ifdef EnableSpeech
  #ifdef AR_USE_WIN_32
    #include <sapi.h>
  #endif
#endif

#include "arSoundNode.h"
#include "arSoundCalling.h"

// Utterance in the scene graph.

class SZG_CALL arSpeechNode : public arSoundNode{
  public:
    // Needs assignment operator and copy constructor, for pointer member.
    arSpeechNode();
    ~arSpeechNode();
    
    void initialize( arDatabase* owner );
    void render();
    arStructuredData* dumpData();
    bool receiveData(arStructuredData*);

  private:
    void _initVoice();
    void _deleteVoice();
    void _speak( const std::string& text );
#ifdef AR_USE_WIN_32
  #ifdef EnableSpeech
    ISpVoice* _voice;
  #endif
#endif
};

#endif        //  #ifndefARSPEECHNODE_H

