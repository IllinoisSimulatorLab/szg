//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPEECH_NODE_H
#define AR_SPEECH_NODE_H

#include "arSoundNode.h"
#include "arTTS.h"
#include "arSoundCalling.h"


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
    arTTS _tts;
};

#endif
