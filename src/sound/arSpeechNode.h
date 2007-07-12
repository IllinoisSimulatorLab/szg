//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPEECH_NODE_H
#define AR_SPEECH_NODE_H

#include "arSoundNode.h"
#include "arSoundCalling.h"

#ifndef AR_USE_WIN_32
#undef EnableSpeech
// Windows SAPI is the only implementation so far.
#endif

#ifdef EnableSpeech
#include "arSapi.h"
#endif

// Utterance in the scene graph.

class SZG_CALL arSpeechNode : public arSoundNode{
  public:
    // Needs assignment operator and copy constructor, for pointer member.
    arSpeechNode();
    ~arSpeechNode();
    
    void initialize( arDatabase* owner );
    bool render() { return true; }
    arStructuredData* dumpData();
    bool receiveData(arStructuredData*);

  private:
    void _initVoice();
    void _deleteVoice();
    void _speak( const std::string& text );
#ifdef EnableSpeech
    ISpVoice* _voice;
#endif
};

#endif
