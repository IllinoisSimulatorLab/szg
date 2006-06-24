//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundClient.h"
#include "arStreamNode.h"
#include "arLogStream.h"
#include "fmodStub.h"

// AARGH! The DSP callback DOES NOT let us pass in an object. Consequently,
// we need this global... and, consequently, we cannot have more than ONE
// arSoundClient in a process!
arSoundClient* __globalSoundClient;

void* SZG_SOUND_CALLBACK ar_soundClientDSPCallback(void*,
						   void* currentBuffer,
						   int bufferLength,
						   int){
#ifndef AR_USE_DARWIN
  float* decoded_ptr = (float*) currentBuffer;
#else
  // DOES THIS MAKE SENSE ?????
  float decoded_ptr[2048];
  for (int i=0; i<1024; i++){
    decoded_ptr[2*i] = ((int*)currentBuffer)[2*i];
    decoded_ptr[2*i+1] = decoded_ptr[2*i];
  }
#endif
  // conveniently, the default buffer length is 1024
  // probably not true for every fmod implementation ????
  
  // to make things a little less verbose, let's do the following...
  arSoundClient* g = __globalSoundClient;
  float* ptr = g->_waveDataPtr;
  
  // NOTE: ASSUMING THAT THE BUFFER LENGTH IS 1024
  for (int i=0; i<bufferLength; i++){
    // adding the right and left channel; together...
    // a little bit dubious
    ptr[i] = decoded_ptr[2*i] + decoded_ptr[2*i+1];
  }
  
  g->relayWaveform();

  // Buffer must go to the next DSP in the chain.
  return currentBuffer;
}

void ar_soundClientWaveformConnectionTask(void* soundClient){
  arSoundClient* s = (arSoundClient*) soundClient;
  // Don't keep trying infinitely
  while (s->_dataServer.acceptConnection() != NULL){
    // Don't print anything here... it's a little obnoxious
  }
}

bool ar_soundClientConnectionCallback(void*, arTemplateDictionary*){
  return true;
}
 
bool ar_soundClientDisconnectCallback(void* client){
  arSoundClient* c = (arSoundClient*) client;
  ar_log_remark() << "arSoundClient remark: disconnected from server.\n";
  // Delete the bundle path information. This
  // is unique to each connection. This information lets
  // an app have its sound files in a flexible location (i.e.
  // NOT on the sound path).
  c->setDataBundlePath("NULL","NULL");
  c->reset();
  // NOTE: DO NOT CALL skipConsumption from here! That is done in the
  // arSyncDataClient proper.
  return true;
}

arSoundClient::arSoundClient():
  _waveTemplate("wave"),
  _dataServerRegistered(false),
  _dataServer(5000),
  _dspStarted(false),
  _dspTap(NULL){
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
  if (!_fSilent)
    FSOUND_Close();
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
  ((arSoundClient*)client)->_render();
  return true;
}

bool ar_soundClientPostSyncCallback(void*){
  // The arSoundClient/ arSoundServer communication is not
  // naturally throttled by anything other than the ping time over the
  // network (while arGraphicsClient/ arGraphicsServer is throttled by
  // buffer swap). fmod will CRASH if we do not throttle ourselves
  // (ping times can be 0.3 msec on LAN, leading to thousands of
  // updates per second. 
  // NOTE: the way things are designed, the sound synchronizations and
  // graphics synchronizations are completely independent. Consequently,
  // we can throttle sound updates to 50 times per second without
  // impacting graphics updates.
  FSOUND_Update();
  // In the framework standalone case we want 
  // to be able to run the sound without this throttle.
  // So build it in to SoundRender instead and remove it here.
  //;;;; ar_usleep(20000);
  return true;
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

void arSoundClient::_render() {
  _soundDatabase.setPlayTransform(_speakerObject);
  _soundDatabase.render();
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
  if (type == "szg_sound_stream_info"){
    return _processStreamInfo(body); 
  }
  
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

void arSoundClient::startDSP(){
  if (_dspStarted)
    return;
  _dspStarted = true;
#ifdef EnableSound
  ar_log_remark() << "arSoundClient remark: DSP started.\n";
  _DSPunit = FSOUND_DSP_Create(ar_soundClientDSPCallback,
			       FSOUND_DSP_DEFAULTPRIORITY_USER+20, 0);
  FSOUND_DSP_SetActive(_DSPunit,1);
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

bool arSoundClient::_initSound(){
#ifndef EnableSound
  ar_log_critical() << "syzygy warning: FMOD disabled, compiled with stub.\n";
  return false;

#else

#ifdef AR_USE_LINUX
  FSOUND_SetOutput(FSOUND_OUTPUT_OSS);
#endif
#ifdef AR_USE_WIN_32
  FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
#endif
#ifdef AR_USE_DARWIN
  FSOUND_SetOutput(FSOUND_OUTPUT_MAC);
#endif
#ifndef AR_USE_DARWIN
  // actually not needed, according to fmod docs
  if (FSOUND_SetMixer(FSOUND_MIXER_QUALITY_FPU) == 0){
    ar_log_error() << "arSoundClient error: Set mixer request failed!\n";
    return false;
  }
#else
  if (FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT) == 0){
    ar_log_error() << "arSoundClient error: Set mixer request failed!\n";
    return false;
  }
#endif
  ar_log_remark() << getLabel() << " remark: FMOD using output " 
                  << FSOUND_GetOutput() << ar_endl;
  // EXPERIMENTING WITH CHANGING THE FMOD INIT IN ARSOUNDCLIENT SO THAT
  // IT IS LIKE THE INIT IN THE LIGHT SHOW...
  //if (!FSOUND_Init(44100, 20, FSOUND_INIT_USEDEFAULTMIDISYNTH))
  if (!FSOUND_Init(44100, 32, 0)){
    ar_log_error() << getLabel() << " error: FMOD audio failed to initialize.\n";
    return false;
  }

  FSOUND_3D_SetDistanceFactor(3.28); // arFramework uses feet; FMOD uses meters
  FSOUND_3D_SetRolloffFactor(.8);
  // .8 puts sounds very quiet when 70 feet away
  // .008 is very slight rolloff with distance

  return true;
#endif
}

string arSoundClient::_processStreamInfo(const string& body){
  int nodeID = -1;
  if (!ar_stringToIntValid(body, nodeID)){
    ar_log_error() << "arSoundClient error: stream_info message had invalid ID.\n";
    return string("SZG_ERROR");
  }

  // AARGH! just thought of something... THIS IS NOT SAFE WITH RESPECT
  // TO DATABASE NODES BEING DELETED! (MAYBE IT ISN"T EVEN SAFE WITH RESPECT
  // TO DATBASE NODES BEING ADDED... i.e. is getNode(...) OK?)
  arDatabaseNode* node = _soundDatabase.getNode(nodeID);
  if (!node || 
      node->getTypeCode() != AR_S_STREAM_NODE){
    ar_log_error() << "arSoundClient error: stream_info message failed to find node.\n";
    return string("SZG_ERROR");
  }

  // After the last check, we know the following cast is valid.
  arStreamNode* streamNode = (arStreamNode*) node;
  stringstream result;
  result << streamNode->getCurrentTime() << "/"
	 << streamNode->getStreamLength();
  return result.str();
}
