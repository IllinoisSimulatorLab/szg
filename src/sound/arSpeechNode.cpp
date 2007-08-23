//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSpeechNode.h"
#include "arSoundDatabase.h"

arSpeechNode::arSpeechNode() : arSoundNode() {
  // RedHat 8.0 will not compile this if these statements are outside of
  // the body of the constructor.
  _typeCode = AR_S_SPEECH_NODE;
  _typeString = "speech";
}

arSpeechNode::~arSpeechNode(){
  _deleteVoice();
}

void arSpeechNode::initialize( arDatabase* owner ) {
  arSoundNode::initialize( owner );
  _initVoice();
}

void arSpeechNode::_initVoice() {
#ifdef EnableSpeech
  _voice = NULL;
  if (!_owningDatabase->isServer()) {
    if (FAILED(::CoInitialize(NULL))) {
      ar_log_error() << "arSpeechNode error: CoInitialize() failed.\n";
      return;
    }
    HRESULT hr = CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&_voice );
    if( !SUCCEEDED( hr ) ) {
      ar_log_error() << "arSpeechNode error: CoCreateInstance() failed. This probably means "
           << "that the Microsoft Speech SDK is not installed.\n";
      _voice = NULL;
      ::CoUninitialize();
      return;
    }
  }
#endif  
}

void arSpeechNode::_deleteVoice() {
#ifdef EnableSpeech
  if (_voice != NULL) {
#if defined(__cplusplus) && !defined(CINTERFACE)
    _voice->Release();
#else
    ISpVoice_Release( _voice );
#endif
    _voice = NULL;
    ::CoUninitialize();
  }
#endif
}

arStructuredData* arSpeechNode::dumpData(){
  arStructuredData* pdata = _l.makeDataRecord(_l.AR_SPEECH);
  _dumpGenericNode(pdata,_l.AR_SPEECH_ID);

  // Stuff pdata's AR_SPEECH_TEXT with _commandBuffer[].
  const int len = _commandBuffer.size();
  pdata->setDataDimension(_l.AR_SPEECH_TEXT,len);
  ARchar* text = (ARchar*) pdata->getDataPtr(_l.AR_SPEECH_TEXT,AR_CHAR);
  for (int i=0; i<len; i++)
    text[i] = (ARint) _commandBuffer.v[i];
  return pdata;
}

/** @bug Can't update ampl or xyz of a triggered sound, once it's started.
 *  Workaround:  keep triggered sounds short.
 */

bool arSpeechNode::receiveData(arStructuredData* pdata){
  if (pdata->getID() != _l.AR_SPEECH){
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
    _speak( speechText );
  }
  
  return true;
}

void arSpeechNode::_speak( const std::string& speechText ) {
#ifndef EnableSpeech
  (void)speechText; // avoid compiler warning
#else
  if (!_voice) {
    return;
  }

  int numChars = speechText.size();
  if (numChars > 0) {
    WCHAR* text = new WCHAR[numChars+1];
    if (!text) {
      ar_log_error() << "arSpeechNode error: memory panic.\n";
      return;
    }
    ar_log_remark() << "arSpeechNode saying '" << speechText << "'.\n";
    const char* src = speechText.c_str();
    for (int i=0; i<numChars; i++) {
      text[i] = (WCHAR)src[i];
    }
    text[numChars] = (WCHAR)0;

#if defined(__cplusplus) && !defined(CINTERFACE)
    (void)_voice->Speak( (const WCHAR *)text, SPF_ASYNC | SPF_IS_XML, NULL );
#else
    (void)ISpVoice_Speak( _voice, (const WCHAR *)text, SPF_ASYNC | SPF_IS_XML, NULL );
#endif
    
    delete[] text;
  } else {
#if defined(__cplusplus) && !defined(CINTERFACE)
    (void)_voice->Speak( (const WCHAR *)NULL, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL );
#else
    (void)ISpVoice_Speak( _voice, (const WCHAR *)NULL, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL );
#endif
  }
#endif
}
