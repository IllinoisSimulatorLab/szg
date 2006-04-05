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
#include "arThread.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arSoundCalling.h"

#ifdef AR_USE_WIN_32
#define SZG_SOUND_CALLBACK __stdcall
#else
#define SZG_SOUND_CALLBACK
#endif

/// Gets data from an arSoundServer at SZG_SOUND/{IP,port},
/// looks for soundfiles in SZG_SOUND/path, and
/// renders the arSoundDatabase which arSoundServer and arSoundClients share.

class SZG_CALL arSoundClient{
  // Needs assignment operator and copy constructor, for pointer members.
  // Probably should be a singleton class, though.
  friend void* SZG_SOUND_CALLBACK 
    ar_soundClientDSPCallback(void*,void*,int,int);
  friend void ar_soundClientWaveformConnectionTask(void*);
  friend bool
    ar_soundClientConnectionCallback(void*, arTemplateDictionary*);
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
  // It might seem a little odd that this returns always, but we have no way
  // of ditinguishing between "no fmod support" and "failure to init fmod".
  // In the former case, it should probably "succeed".
  bool   init(){ _fSilent = !_initSound(); return true; }
  bool   start(arSZGClient& client);
  string processMessage(const string& type, const string& body);

  void terminateSound();
  const string& getLabel() const;

  void startDSP();
  void relayWaveform();
  void setDSPTap(void (*callback)(float*));
  void microphoneVolume(int volume);

  // _soundDatabase interface
  bool empty() { return _soundDatabase.empty(); }
  void reset() { _soundDatabase.reset(); }
  arSyncDataClient _cliSync;
  // THIS METHOD IS NO LONGER SUPPORTED AT THE DATABASE LEVEL. IT IS A
  // RELIC OF A PREVIOUS TIME!
  //int getFrameID() const { return _soundDatabase.getFrameID(); }
  string getPath(){ return _soundDatabase.getPath(); }
  void setPath(const string& s) { _soundDatabase.setPath(s); }
  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory)
    { _soundDatabase.setDataBundlePath(bundlePathName, bundleSubDirectory); }
  void addDataBundlePathMap(const string& bundlePathName, const string& bundlePath)
    { _soundDatabase.addDataBundlePathMap(bundlePathName, bundlePath); }

  void setSpeakerObject(arSpeakerObject* s) { _speakerObject = s; }

  bool readFrame();

  bool silent() const { return _fSilent; }
  
 private:
  void _render();
 protected:
  arSpeakerObject*     _speakerObject;
  arSoundDatabase      _soundDatabase;
  // The arSoundClient can provide the waveform to external objects.
  arDataTemplate       _waveTemplate;
  arStructuredData*    _waveData;
  arTemplateDictionary _dspLanguage;
  arThread             _connectionThread;
  float*               _waveDataPtr;
  bool                 _dataServerRegistered;
  arDataServer         _dataServer;

  FSOUND_DSPUNIT* _DSPunit;
  bool            _dspStarted;
  void (*_dspTap)(float*);

  arSignalObject  _signalObject;
  arMutex         _bufferSwapLock;

  // Microphone output into the sound player.
  int             _recordChannel;
  int             _microphoneVolume;

  bool _fSilent; // True iff sound subsystem failed to initialize at runtime.
  bool _initSound();

  string _processStreamInfo(const string& body);
};

#endif
