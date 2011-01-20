//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundDatabase.h"

arSpeechNode::arSpeechNode() : arSoundNode() {
  // RedHat 8.0 will not compile these two statements as initializers.
  // Leave them in the constructor's body.
  _typeCode = AR_S_SPEECH_NODE;
  _typeString = "speech";
}

arSpeechNode::~arSpeechNode() {
}

void arSpeechNode::initialize( arDatabase* owner ) {
  arSoundNode::initialize( owner );
  _tts.init();
}


arStructuredData* arSpeechNode::dumpData() {
  arStructuredData* pdata = _l.makeDataRecord(_l.AR_SPEECH);
  _dumpGenericNode(pdata, _l.AR_SPEECH_ID);

  // Stuff pdata's AR_SPEECH_TEXT with _commandBuffer[].
  const int len = _commandBuffer.size();
  pdata->setDataDimension(_l.AR_SPEECH_TEXT, len);
  ARchar* text = (ARchar*) pdata->getDataPtr(_l.AR_SPEECH_TEXT, AR_CHAR);
  for (int i=0; i<len; i++)
    text[i] = (ARint) _commandBuffer.v[i];
  return pdata;
}

/** @bug Can't update ampl or xyz of a triggered sound, once it's started.
 *  Workaround:  keep triggered sounds short.
 */

bool arSpeechNode::receiveData(arStructuredData* pdata) {
  if (pdata->getID() != _l.AR_SPEECH) {
    cout << "arSpeechNode error: expected "
         << _l.AR_SPEECH
         << " (" << _l._stringFromID(_l.AR_SPEECH) << "), not "
         << pdata->getID()
         << " (" << _l._stringFromID(pdata->getID()) << ")\n";
    return false;
  }

  // Stuff _commandBuffer[] with AR_SPEECH_TEXT.
  const string speechText(pdata->getDataString(_l.AR_SPEECH_TEXT));
  const int len = speechText.length();
  _commandBuffer.grow(len+1);
  for (int i=0; i<len; i++)
    _commandBuffer.v[i] = float(speechText.data()[i]);

  // Special processing for momentary events:
  // not propagating state from server to clients, but propagating a single event.
  // (basically, all speech events)
  if (!_owningDatabase->isServer()) {
    _tts.speak( speechText );
  }
  
  return true;
}

