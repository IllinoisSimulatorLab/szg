//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundClient.h"
#include "arStreamNode.h"
#include "arLogStream.h"
#include "fmodStub.h"

// AARGH! The DSP callback DOES NOT let us pass in an object. Consequently,
// we need this global... and, consequently, we cannot have more than ONE
// arSoundClient in a process!
arSoundClient* __globalSoundClient;

#ifdef EnableSound
FMOD::System* ar_fmod() {
  static FMOD::System* s = NULL;
  static bool fFailed = false;
  // if fFailed, returning NULL will likely crash this exe.  Oh well,
  // they can read the diagnostic.
  if (s || fFailed)
    return s;
  const FMOD_RESULT r = FMOD::System_Create(&s);
  if (r != FMOD_OK) {
    fFailed = true;
    ar_log_error() << "failed to create fmod: " << FMOD_ErrorString(r) << ar_endl;
    s = NULL;
  }
  return s;
}

bool ar_fmodcheck3(const FMOD_RESULT r, const char* file, const int line) {
  if (r == FMOD_OK)
    return true;

  // Skip "../../../" .
  file += strspn(file, "./");

  // Skip "src/".
  if (!strncmp(file, "src/", 4))
    file += 4;

  ar_log_error() << file << ":" << line <<
    " fmod: " << FMOD_ErrorString(r) << ar_endl;
  return false;
}
#endif

#ifdef AR_USE_WIN_32
#define SZG_CALLBACK __stdcall
#else
#define SZG_CALLBACK
#endif

#ifdef EnableSound
FMOD_RESULT SZG_CALLBACK ar_soundClientDSPCallback(
    FMOD_DSP_STATE* /*pState*/,
    float *  bufSrc, 
    float *  bufDst, 
    unsigned length, 
    int  inchannels, 
    int  /*outchannels*/){
  for (unsigned i=0; i<length; i++){
    // average the right and left channel
    // incorrectly assumes inchannels is 2, outchannels is 1
    __globalSoundClient->_waveDataPtr[i] 
      = 14000*(bufSrc[inchannels*i] + bufSrc[inchannels*i +1]);
  }
  for (unsigned i=0; i<length*inchannels; i++){
    bufDst[i] = bufSrc[i];
  }
  
  // abbreviation
  arSoundClient* g = __globalSoundClient;
  g->relayWaveform(); // Forward buffer to the next DSP in the chain.
  return FMOD_OK;
}
#endif

void ar_soundClientWaveformConnectionTask(void* soundClient){
  arSoundClient* s = (arSoundClient*) soundClient;
  while (s->_dataServer.acceptConnection() != NULL) {
  }
}

bool ar_soundClientConnectionCallback(void*, arTemplateDictionary*){
  return true;
}
 
bool ar_soundClientDisconnectCallback(void* client){
  arSoundClient* c = (arSoundClient*)client;
  ar_log_remark() << "arSoundClient remark: disconnected from server.\n";
  // Delete the bundle path, which is unique to each connection so
  // an app's sound files be other than "on the sound path."
  c->setDataBundlePath("NULL","NULL");
  c->reset();
  // Call skipConsumption from arSyncDataClient, not from here.
  return true;
}

arSoundClient::arSoundClient():
  _waveTemplate("wave"),
  _dataServerRegistered(false),
  _dataServer(5000),
  _dspStarted(false),
  _dspTap(NULL),
#ifdef EnableSound
  _recordChannel(NULL),
#endif
  _microphoneVolume(0){
  // Set-up the language.
  _waveTemplate.add("data",AR_FLOAT);
  _dspLanguage.add(&_waveTemplate);
  _waveData = new arStructuredData(&_waveTemplate);
  // Make sure we have enough storage. 
  _waveData->setDataDimension("data",1024);
  // The data ptr never changes, so let's get a hold of it here.
  _waveDataPtr = (float*) _waveData->getDataPtr("data",AR_FLOAT);
  _cliSync.setConnectionCallback(ar_soundClientConnectionCallback);
  _cliSync.setDisconnectCallback(ar_soundClientDisconnectCallback);
  _cliSync.setConsumptionCallback(ar_soundClientConsumptionCallback);
  _cliSync.setActionCallback(ar_soundClientActionCallback);
  _cliSync.setNullCallback(ar_soundClientNullCallback);
  _cliSync.setPostSyncCallback(ar_soundClientPostSyncCallback);
  _cliSync.setBondedObject(this);
  // AARGH! must set this so we can get it into the fmod callback
  __globalSoundClient = this;
}

void arSoundClient::terminateSound(){
#ifdef EnableSound
  if (!_fSilent)
    ar_fmod()->release();
#endif
}

arSoundClient::~arSoundClient(){
  // Don't delete _speakerObject, since we don't own it.
  delete _waveData;
}

/// Configure the sound rendering object (i.e. the arSoundClient) using
/// the Syzygy parameter database
bool arSoundClient::configure(arSZGClient* client){
  setPath(client->getAttribute("SZG_SOUND", "path"));
  return true;
}

bool ar_soundClientActionCallback(void* client){
  return ((arSoundClient*)client)->_render();
}

bool ar_soundClientPostSyncCallback(void*){
  // Only the LAN throttles arSoundClient/ arSoundServer communication,
  // causing thousands of updates per second.  So we need a throttle.
  // Standalone we want no 50fps throttle, so do it in SoundRender instead.
  // // ar_usleep(20000);

  // todo: with a local timer, call it no more than every 20 msec.
  return ar_fmodcheck(ar_fmod()->update());
}

bool ar_soundClientNullCallback(void*){
  return true;
}

bool ar_soundClientConsumptionCallback(void* client, ARchar* buffer){
  arSoundClient* c = (arSoundClient*) client;
  if (!c->_soundDatabase.handleDataQueue(buffer)) {
    ar_log_error() << c->getLabel() << " error: failed to consume buffer.\n";
    return false;
  }
  return true;
}

bool arSoundClient::_render() {
  _soundDatabase.setPlayTransform(_speakerObject);
  return _soundDatabase.render();
}

/// Sets the networks on which the arSoundClient will attempt to connect to the
/// server, in order of descending preference
void arSoundClient::setNetworks(string networks){
  _cliSync.setNetworks(networks);
}

bool arSoundClient::start(arSZGClient& client){
  // Register the service for sending out the waveform, if possible.
  const string serviceName(client.createComplexServiceName("SZG_WAVEFORM"));
  int port = -1;
  if (client.registerService(serviceName,"sound",1,&port)){
    _dataServer.setPort(port);
    _dataServer.setInterface("INADDR_ANY");
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // This material is cut-and-pasted from many other locations...
    for (int trials=0; trials < 10; ++trials){
      if (_dataServer.beginListening(&_dspLanguage)){
	client.confirmPorts(serviceName,"sound",1,&port);
	_connectionThread.beginThread(ar_soundClientWaveformConnectionTask, this);
	// We registered the service.
	_dataServerRegistered = true;
        break;
      }
      ar_log_error() << "arSoundClient error: failed to listen on brokered port.\n";
      client.requestNewPorts(serviceName,"input",1,&port);
    }
    // If no success, just let this feature go.
  }
  // Start the underlying data transfer from server to client
  _cliSync.setServiceName("SZG_SOUND");
  return _cliSync.init(client) && _cliSync.start();
}

string arSoundClient::processMessage(const string& type, 
				     const string& body){
  if (type == "szg_sound_stream_info")
    return _processStreamInfo(body); 
  
  // We need a way to say that the message type is not
  // supported. There will have to be a general message processing
  // mechanism for RPC... Without RPC, we could just have message handlers
  // attach to a stream of messages... it wouldn't matter if more than one
  // caught the message. However, when messages can have responses, this
  // is different.
  return string("SZG_UNHANDLED");
}

const string& arSoundClient::getLabel() const {
  static const string noname("arSoundClient");
  const string& s = _cliSync.getLabel();
  return s == "arSyncDataClient" ? noname : s;
}

bool arSoundClient::startDSP(){
  if (_dspStarted)
    return true;
  _dspStarted = true;

#ifndef EnableSound
  return true;
#else

  ar_log_remark() << "arSoundClient remark: DSP started.\n";
  FMOD::Sound* samp1 = NULL;
  // memory leak: should (void)ar_fmodcheck(_stream->release()) in stopDSP() or ~arSoundClient().

  FMOD_CREATESOUNDEXINFO x = {0};
  x.cbsize = sizeof(x);
  x.numchannels = 1;
  x.format = FMOD_SOUND_FORMAT_PCM16;
  x.defaultfrequency = 44100;
  x.length = 44100 * sizeof(short) * x.numchannels * 5;
  if (!ar_fmodcheck(ar_fmod()->createSound(NULL, FMOD_3D | FMOD_SOFTWARE | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &x, &samp1)))
    return false;

  // Allow playing from the mic.  (Mic comes from setRecordDriver and windows' audio control panel.)
  if (!ar_fmodcheck(ar_fmod()->recordStart(samp1, true))) {
    ar_log_error() << "failed to start recording.\n";
    // Don't return false here. Give the DSP a chance to start.
  }

  // Start playing the sample paused.  Set its volume.  Unpause it.
  if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, samp1, true, &_recordChannel)) ||
      !ar_fmodcheck(_recordChannel->setVolume(_microphoneVolume)) ||
      !ar_fmodcheck(_recordChannel->setPaused(false))) {
    return false;
  }

  // Start the DSP.
  FMOD_DSP_DESCRIPTION d = {0};
  d.read = ar_soundClientDSPCallback;
  return ar_fmodcheck(ar_fmod()->createDSP(&d, &_DSPunit)) &&
         ar_fmodcheck(ar_fmod()->addDSP(_DSPunit)) &&
         ar_fmodcheck(_DSPunit->setActive(true));
#endif
}

void arSoundClient::relayWaveform(){
  if (_dspTap)
    _dspTap(_waveDataPtr);

  if (_dataServerRegistered){
    // Post the data to the network.
    _dataServer.sendData(_waveData);
  }
}

void arSoundClient::setDSPTap(void (*callback)(float*)){
  _dspTap = callback;
}

bool arSoundClient::microphoneVolume(int volume){
  if (volume < 0){
    _microphoneVolume = 0;
  }
  else if (volume > 255){
    _microphoneVolume = 255;
  }
  else{
    _microphoneVolume = volume;
  }
#ifdef EnableSound
  if (_recordChannel){
    if (!ar_fmodcheck(_recordChannel->setVolume(_microphoneVolume / 255)))
      return false;

    // bug: don't make the same call twice in a row.
    if (_microphoneVolume == 0){
      // Save CPU, it's silent anyways.
      if (!ar_fmodcheck(_recordChannel->setPaused(true)))
        return false;
    }
    else{
      if (!ar_fmodcheck(_recordChannel->setPaused(false)))
        return false;
    }
  }
#endif
  return true;
}

//;;;; mp3 more than 100KB: open as ar_fmod()->createStream not ar_fmod()->createSound.
//;;;; And maybe nonblocking too.
//;;;; test with butterfly dance piece.

bool arSoundClient::_initSound(){
#ifndef EnableSound
  ar_log_critical() << "Silent, compiled with stub FMOD.\n";
  return false;

#else

  if (!ar_fmodcheck(ar_fmod()->setSoftwareFormat(44100, FMOD_SOUND_FORMAT_PCM16, 0, 0, FMOD_DSP_RESAMPLER_LINEAR))) {
    ar_log_error() << "fmod audio failed to init.\n";
    return false;
  }

  const float feetNotMeters = 3.28;
  const float rolloff = 0.8; // .8 is faint, 70 feet away.  .008 is much more gradual.
  const int numVirtualVoices = 100;
  return ar_fmodcheck(ar_fmod()->set3DSettings(1.0, feetNotMeters, rolloff)) &&
         ar_fmodcheck(ar_fmod()->init(
	   numVirtualVoices, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0));

  // ar_fmodcheck(ar_fmod()->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));
  // FMOD_OUTPUTTYPE_ASIO or FMOD_OUTPUTTYPE_DSOUND
  // linux: FMOD_OUTPUTTYPE_ALSA FMOD_OUTPUTTYPE_OSS

#endif
}

string arSoundClient::_processStreamInfo(const string& body){
  int nodeID = -1;
  if (!ar_stringToIntValid(body, nodeID)){
    ar_log_error() << "arSoundClient stream_info message had invalid ID.\n";
    return string("SZG_ERROR");
  }

  // getNode() may be unsafe here when database nodes are deleted, or even added.
  arDatabaseNode* node = _soundDatabase.getNode(nodeID);
  if (!node || node->getTypeCode() != AR_S_STREAM_NODE){
    ar_log_error() << "arSoundClient stream_info message found no node.\n";
    return string("SZG_ERROR");
  }

  // node->getTypeCode() confirms that this cast is valid.
  arStreamNode* tmp = (arStreamNode*)node;
  stringstream result;
  result << tmp->getCurrentTime() << "/" << tmp->getStreamLength();
  return result.str();
}
