//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_CLIENT_H
#define AR_SOUND_CLIENT_H

#include "arSoundDatabase.h"
#include "arDataClient.h"
#include "arDataType.h"
#include "arSyncDataClient.h"
#include <string>
#include "arSoundCalling.h"

#ifdef AR_USE_WIN_32
#define SZG_SOUND_CALLBACK __stdcall
#else
#define SZG_SOUND_CALLBACK
#endif

// Gets data from an arSoundServer at SZG_SOUND/{IP, port},
// looks for soundfiles in SZG_SOUND/path, and
// renders the arSoundDatabase which arSoundServer and arSoundClients share.

void* SZG_SOUND_CALLBACK ar_soundClientDSPCallback(void*, void*, int, int);
void ar_soundClientWaveformConnectionTask(void*);
bool ar_soundClientConnectionCallback(void*, arTemplateDictionary*);
bool ar_soundClientDisconnectCallback(void*);
bool ar_soundClientConsumptionCallback(void*, ARchar*);
bool ar_soundClientActionCallback(void*);
bool ar_soundClientNullCallback(void*);
bool ar_soundClientPostSyncCallback(void*);

class SZG_CALL arSoundClient{
  // Needs assignment operator and copy constructor, for pointer members.
  // Probably should be a singleton class, though.

  friend void* SZG_SOUND_CALLBACK ar_soundClientDSPCallback(void*, void*, int, int);
  friend void ar_soundClientWaveformConnectionTask(void*);
  friend bool ar_soundClientConnectionCallback(void*, arTemplateDictionary*);
  friend bool ar_soundClientDisconnectCallback(void*);
  friend bool ar_soundClientConsumptionCallback(void*, ARchar*);
  friend bool ar_soundClientActionCallback(void*);
  friend bool ar_soundClientNullCallback(void*);
  friend bool ar_soundClientPostSyncCallback(void*);

 public:
  arSoundClient();
  ~arSoundClient();

  bool configure(arSZGClient*);

  void   setBufferSize(int);
  void   setNetworks(string);
  bool   init() {
    _fSilent = !_initSound();
#ifdef EnableSound
    return !_fSilent;
#else
    return true;
#endif
  }
  bool   start(arSZGClient& client);
  string processMessage(const string& type, const string& body);

  void terminateSound();
  const string& getLabel() const;

  bool startDSP();
  void relayWaveform();
  void setDSPTap(void (*callback)(float*));
  bool microphoneVolume(int volume);

  // _soundDatabase interface
  bool empty() { return _soundDatabase.empty(); }
  void reset() { _soundDatabase.reset(); }
  arSyncDataClient _cliSync;
  string getPath() const { return _soundDatabase.getPath(); }
  int _getMode() const { return _soundDatabase.getMode(); }
  void _setMode(const int m) { _soundDatabase.setMode(m); }
  void setPath(const string& s) { _soundDatabase.setPath(s); }
  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory)
    { _soundDatabase.setDataBundlePath(bundlePathName, bundleSubDirectory); }
  void addDataBundlePathMap(const string& bundlePathName, const string& bundlePath)
    { _soundDatabase.addDataBundlePathMap(bundlePathName, bundlePath); }
  void setSpeakerObject(arSpeakerObject* s) { _speakerObject = s; }
  bool readFrame();
  bool silent() const
    { return _fSilent; }

 private:
  bool _render();
 protected:
  arSpeakerObject*     _speakerObject;
  arSoundDatabase      _soundDatabase;
  // The arSoundClient can provide the waveform to external objects.
  arDataTemplate       _waveTemplate;
  arStructuredData*    _waveData;
  arTemplateDictionary _dspLanguage;
  arThread             _connectionThread;
 public:
  float*               _waveDataPtr; // Darwin sometimes needs this public not protected
 protected:
  bool                 _dataServerRegistered;
  arDataServer         _dataServer;

#ifdef EnableSound
  FMOD_DSP* _DSPunit;
#endif
  bool            _dspStarted;
  void (*_dspTap)(float*);

#ifdef UNUSED
  arSignalObject  _signalObject;
  arLock         _bufferSwapLock;
#endif

  // Microphone output into the sound player.
#ifdef EnableSound
  FMOD_CHANNEL* _recordChannel;
#endif
  int             _microphoneVolume;
  float _rolloff;

  bool _fSilent; // True iff sound subsystem failed to initialize at runtime.
  bool _initSound();

  string _processStreamInfo(const string& body);
};

#endif
