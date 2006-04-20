//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
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
#ifdef AR_USE_WIN_32
  _voice = NULL;
  if (!_owningDatabase->isServer()) {
    if (FAILED(::CoInitialize(NULL))) {
      cerr << "arSpeechNode error: CoInitialize() failed.\n";
      return;
    }
    HRESULT hr = CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&_voice );
    if( !SUCCEEDED( hr ) ) {
      cerr << "arSpeechNode error: CoCreateInstance() failed. This probably means "
           << "that the Microsoft Speech SDK is not installed.\n";
      _voice = NULL;
      ::CoUninitialize();
      return;
    }
  }
#endif
#endif  
}

void arSpeechNode::_deleteVoice() {
#ifdef EnableSpeech
#ifdef AR_USE_WIN_32
  if (_voice != NULL) {
    _voice->Release();
    _voice = NULL;
    ::CoUninitialize();
  }
#endif
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
#ifdef EnableSpeech
#ifdef AR_USE_WIN_32
  if (_voice == NULL) {
    return;
  }
  int numChars = speechText.size();
  if (numChars > 0) {
    WCHAR* text = new WCHAR[numChars+1];
    if (!text) {
      cerr << "arSpeechNode error: memory panic.\n";
      return;
    }
    cout << "arSpeechNode remark: saying(" << speechText << ")" << endl;
    const char* txt = speechText.c_str();
    for (int i=0; i<numChars; i++) {
      text[i] = (WCHAR)txt[i];
    }
    text[numChars] = (WCHAR)0;

    HRESULT hr = _voice->Speak( (const WCHAR *)text, SPF_ASYNC | SPF_IS_XML, NULL );
    
    delete[] text;
  } else {
    HRESULT hr = _voice->Speak( (const WCHAR *)NULL, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL );
  }
#endif
#endif
}

    
