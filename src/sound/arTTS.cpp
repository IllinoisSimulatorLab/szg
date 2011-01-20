//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTTS.h"
#include "arLogStream.h"

arTTS::arTTS() : 
  _voice(NULL) {
}

arTTS::~arTTS() {
  _deleteVoice();
}

void arTTS::init() {
  _initVoice();
}

void arTTS::_initVoice() {
#ifdef EnableSpeech
  _voice = NULL;
  if (FAILED(::CoInitialize(NULL))) {
    ar_log_error() << "arTTS failed to CoInitialize().\n";
    return;
  }
  const HRESULT hr =
    CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&_voice );
  if ( !SUCCEEDED( hr ) ) {
    ar_log_error() <<
      "arTTS failed to CoCreateInstance(). Is Microsoft Speech SDK installed?\n";
    _voice = NULL;
    ::CoUninitialize();
    return;
  }
#endif  
}

void arTTS::_deleteVoice() {
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


void arTTS::speak( const std::string& speechText ) {
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
      ar_log_error() << "arTTS out of memory.\n";
      return;
    }
    ar_log_remark() << "arTTS saying '" << speechText << "'.\n";
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
