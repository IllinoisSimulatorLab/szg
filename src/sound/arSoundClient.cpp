//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundClient.h"
#include "arStreamNode.h"
#include "arLogStream.h"
#include "fmodStub.h"

// This global passes an object to the DSP (instead of an arg, (still?)
// not supported).  Thus, at most one arSoundClient per process.
arSoundClient* __globalSoundClient = NULL;

#ifdef EnableSound
FMOD_SYSTEM* ar_fmod() {
  static FMOD_SYSTEM* s = NULL;
  static bool fFailed = false;
  // If fFailed, returning NULL will likely crash.
  // Oh well, they can read stderr.
  if (s || fFailed)
    return s;

  const FMOD_RESULT r = FMOD_System_Create(&s);
  if (r != FMOD_OK) {
    fFailed = true;
    // ar_log_xxx() fails silently in here,
    // perhaps because it's too early, still in constructors, before main().
    cerr << "arSoundClient failed to create fmod: " << FMOD_ErrorString(r) << ".\n";
    s = NULL;
    return s;
  }

  unsigned t;
  if (!ar_fmodcheck( FMOD_System_GetVersion( ar_fmod(), &t ))) {
    cerr << "arSoundClient failed to verify fmod version.\n"; // not ar_log_xxx
    s = NULL;
    return s;
  }
  char versionString[100];
  sprintf( versionString, "%x", t );
  if (t < FMOD_VERSION) {
    ar_log_error() << "fmod dll has version " << versionString <<
      ", expected at least " << FMOD_VERSION << ".\n"; // not ar_log_xxx
    s = NULL;
  } else {
    ar_log_debug() << "fmod version: " << versionString << ar_endl;
  }
  // FMOD_VERSION == 0x40305 is known to work as of June 2008.
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

  ar_log_error() << file << ":" << line << " fmod: " << FMOD_ErrorString(r) << "\n";
  return false;
}
#endif

#ifdef AR_USE_WIN_32
#define SZG_CALLBACK __stdcall
#else
#define SZG_CALLBACK
#endif

#ifdef EnableSound

const unsigned iFIRMac = 200; // Tidemann's fir_length
const unsigned iFIRMax = 256; // max interaural time delay
const unsigned cEle = 50; // todo: read from datafile
const unsigned cAzi = 25; // todo: read from datafile

class dspHRTFCIPIC_state
{
 public:
  int azumuth;
  int elevation;
  int distance;

  // circular buffer
  float rgxFIR[iFIRMax];        // Tidemann's fir_state
  int iFIR;                            // Tidemann's fir_state_index
  float xL;                        // left channel of input

  dspHRTFCIPIC_state() :
    iFIR(0),
    xL(0.)
    {}
};
dspHRTFCIPIC_state dspHRTF;
// todo: member of arSoundClient? of g?  &dspHRTF passed in, a la hrtfFilter_CIPIC::firFilter?

FMOD_RESULT SZG_CALLBACK ar_soundClientDSPCallback(
    FMOD_DSP_STATE* /*pState*/,
    float *  bufSrc,
    float *  bufDst,
    unsigned cSamp,
    int  inchannels,
    int  outchannels) {

  arSoundClient* g = __globalSoundClient; // abbreviation
  unsigned iSamp;

  switch (g->_getMode()) {
  default:
    break;

  case mode_fmodplugins: {
    // One filter for *all* sounds, not one filter per sound.
    // Bogusly localize multiple simultaneous sounds to one location.

    if (inchannels != 2 || outchannels != 2) {
      ar_log_error() << "fmodplugin internal error: wrong number of channels.\n";
      return FMOD_OK;
    }

    if (iFIRMac > iFIRMax) {
      ar_log_error() << "fmodplugin: recompile with larger FIR buffer.\n";
      return FMOD_OK;
    }

    unsigned iAzi = 12; // Tidemann's azumuth_index
    unsigned iEle = 25; // Tidemann's elevation_index
    // derive from cEle cAzi?

    // todo: attenuate inversely with distance; that's accurate enough when distance >1 m.

    dspHRTF.xL = 0.;
    int& iFIR = dspHRTF.iFIR;
    for (iSamp=0; iSamp<cSamp; ++iSamp) {
      ++iFIR %= iFIRMax;
      int jFIR = iFIR;
      dspHRTF.rgxFIR[iFIR] = dspHRTF.xL;

      // Monophonic input.  Discard right channel.
      dspHRTF.xL = *bufSrc++;
      bufSrc++;

      // Convolute to get output.
      double yL = 0;
      double yR = 0;

      // Head-Related Impulse Response
      // const float subject_015_hrir_left[] and ...right[], from e.g. subject_015.c
      // todo: read from datafile
      static const float hrirPlaceholder[1] = {0.0};
      const float* hrirL = hrirPlaceholder;
      const float* hrirR = hrirPlaceholder;
      // todo: hrirL and hrirR = new float[250000], read from datafile.
      // humansubject ascii datafile format: iAziMax iEleMax iFIRMax, then linearized 3d array of float coeffs.

      // Initial index of HRTF filter for both ears.  Linearized 3D array.
      const unsigned iFIRMic = cEle*iFIRMac*iAzi + iFIRMac*iEle;
      // todo: verify by looping iAzi here, and listening on headphones.

#if 0
      // disabled until datafile reading implemented (see todo's in arSoundClient.cpp)
      for (unsigned m = iFIRMic; m < iFIRMic + iFIRMac; ++m) {
        yL += dspHRTF.rgxFIR[jFIR] * hrirL[m];
        yR += dspHRTF.rgxFIR[jFIR] * hrirR[m];
        --jFIR %= iFIRMax;
      }
#else
      yL = yR = dspHRTF.rgxFIR[jFIR];
#endif

      *bufDst++ = float(yL);
      *bufDst++ = float(yR);
    }
    break;
    }

  case mode_fmod:
    // About 35 fps.
    for (iSamp=0; iSamp<cSamp; ++iSamp) {
      // Average the left and right channel.
      // Incorrectly assumes inchannels is 2, outchannels is 1.
      g->_waveDataPtr[iSamp] = 14000*(bufSrc[inchannels*iSamp] + bufSrc[inchannels*iSamp +1]);
    }
    for (iSamp=0; iSamp<cSamp*inchannels; iSamp++) {
      bufDst[iSamp] = bufSrc[iSamp];
    }
    g->relayWaveform(); // Forward buffer to the next DSP in the chain.
    return FMOD_OK;
  }
  return FMOD_OK;
}
#endif

void ar_soundClientWaveformConnectionTask(void* soundClient) {
  arSoundClient* s = (arSoundClient*) soundClient;
  while (s->_dataServer.acceptConnection()) {
  }
}

bool ar_soundClientConnectionCallback(void*, arTemplateDictionary*) {
  return true;
}

bool ar_soundClientDisconnectCallback(void* client) {
  ar_log_remark() << "disconnected.\n";

  // Delete the bundle path, which is unique to each connection so
  // an app's sound files be other than "on the sound path."
  arSoundClient* c = (arSoundClient*)client;
  c->setDataBundlePath("NULL", "NULL");
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
  _microphoneVolume(0),
  _rolloff(.08)  // .8 is faint, 70 feet away.  .008 is much more gradual.
{
  // Set up the language.
  _waveTemplate.add("data", AR_FLOAT);
  _dspLanguage.add(&_waveTemplate);
  _waveData = new arStructuredData(&_waveTemplate);

  // Allocate enough storage.
  _waveData->setDataDimension("data", 1024);

  // Locally cache the unchanging data ptr.
  _waveDataPtr = (float*) _waveData->getDataPtr("data", AR_FLOAT);
  _cliSync.setConnectionCallback(ar_soundClientConnectionCallback);
  _cliSync.setDisconnectCallback(ar_soundClientDisconnectCallback);
  _cliSync.setConsumptionCallback(ar_soundClientConsumptionCallback);
  _cliSync.setActionCallback(ar_soundClientActionCallback);
  _cliSync.setNullCallback(ar_soundClientNullCallback);
  _cliSync.setPostSyncCallback(ar_soundClientPostSyncCallback);
  _cliSync.setBondedObject(this);

  // For fmod callback.
  __globalSoundClient = this;
}

void arSoundClient::terminateSound() {
#ifdef EnableSound
  if (!_fSilent) {
    FMOD_System_Release( ar_fmod() );
  }
#endif
}

arSoundClient::~arSoundClient() {
  // Don't delete _speakerObject, since we don't own it.
  delete _waveData;
}

bool arSoundClient::configure(arSZGClient* cli) {
  setPath(cli->getAttribute("SZG_SOUND", "path"));

  string renderMode(cli->getAttribute("SZG_SOUND", "render",
    "|fmod|fmod_plugins|vss|mmio|"));
  ar_log_debug() << "mode SZG_SOUND/render '" << renderMode << "'.\n";
  if (renderMode == "fmod_plugins") {
    ar_log_warning() << "ignoring incompletely implemented mode SZG_SOUND/render " <<
      renderMode << ".\n";
    renderMode = "fmod";
  }
  _setMode(
    renderMode == "fmod_plugins" ?
      mode_fmodplugins :
    renderMode == "vss" ?
      mode_vss :
    renderMode == "mmio" ?
      mode_mmio :
      mode_fmod);

  float rolloff;
  if (cli->getAttributeFloats( "SZG_SOUND", "fmod_rolloff", &rolloff )) {
    _rolloff = rolloff;
  }
  
  // Hack in some sound system parameter values
  // (to be replaced eventually by database parameter values).
//  FMOD_System_SetSpeakerPosition( ar_fmod(), FMOD_SPEAKER_FRONT_LEFT, 0., 1.  );
//  FMOD_System_SetSpeakerPosition( ar_fmod(), FMOD_SPEAKER_FRONT_RIGHT, 0., 1.  );
//  FMOD_System_SetSpeakerPosition( ar_fmod(), FMOD_SPEAKER_SIDE_LEFT, 0., 1.  );
//  FMOD_System_SetSpeakerPosition( ar_fmod(), FMOD_SPEAKER_SIDE_RIGHT, 0., 1.  );

  return true;
}

bool ar_soundClientActionCallback(void* client) {
  return ((arSoundClient*)client)->_render();
}

bool ar_soundClientPostSyncCallback(void*) {
  // todo: with a local timer, call it no more than every 20 msec.
  return ar_fmodcheck( FMOD_System_Update( ar_fmod() ) );
}

bool ar_soundClientNullCallback(void*) {
  return true;
}

bool ar_soundClientConsumptionCallback(void* client, ARchar* buffer) {
  arSoundClient* c = (arSoundClient*) client;
  if (!c->_soundDatabase.handleDataQueue(buffer)) {
    ar_log_error() << "failed to consume buffer.\n";
    return false;
  }
  return true;
}

bool arSoundClient::_render() {
  // In this order: update listener's state before sound sources'.
  _soundDatabase.setPlayTransform(_speakerObject);
  return _soundDatabase.render();
}

// Sets the networks on which the arSoundClient will attempt to connect to the
// server, in order of descending preference
void arSoundClient::setNetworks(string networks) {
  _cliSync.setNetworks(networks);
}

bool arSoundClient::start(arSZGClient& client) {
  // Register the service for sending out the waveform, if possible.
  const string serviceName(client.createComplexServiceName("SZG_WAVEFORM"));
  int port = -1;
  if (client.registerService(serviceName, "sound", 1, &port)) {
    _dataServer.setPort(port);
    _dataServer.setInterface("INADDR_ANY");
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // This material is cut-and-pasted from many other locations...
    for (int trials=0; trials < 10; ++trials) {
      if (_dataServer.beginListening(&_dspLanguage)) {
        client.confirmPorts(serviceName, "sound", 1, &port);
        _connectionThread.beginThread(ar_soundClientWaveformConnectionTask, this);
        // We registered the service.
        _dataServerRegistered = true;
        break;
      }
      ar_log_error() << "failed to listen on brokered port.\n";
      client.requestNewPorts(serviceName, "input", 1, &port);
    }
    // If no success, just let this feature go.
  }
  // Start the underlying data transfer from server to client
  _cliSync.setServiceName("SZG_SOUND");
  return _cliSync.init(client) && _cliSync.start();
}

string arSoundClient::processMessage(const string& type,
                                     const string& body) {
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

bool arSoundClient::startDSP() {
  if (_dspStarted)
    return true;
  _dspStarted = true;

  // Always called.  Clone this for HRTF?

#ifndef EnableSound
  return true;
#else

  ar_log_remark() << "DSP started.\n";
  FMOD_SOUND* samp1 = NULL;
  // memory leak: should (void)ar_fmodcheck(_stream->release()) in stopDSP() or ~arSoundClient().

  FMOD_CREATESOUNDEXINFO x = {0};
  x.cbsize = sizeof(x);
  x.numchannels = 1;
  x.format = FMOD_SOUND_FORMAT_PCM16;
  x.defaultfrequency = 44100;
  x.length = 44100 * sizeof(short) * x.numchannels * 5;
  if (!ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), NULL,
          FMOD_3D | FMOD_SOFTWARE | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &x, &samp1 ))) {
//  try FMOD_HARDWARE instead of FMOD_SOFTWARE ?
    return false;
  }
#ifdef Busted_on_zx81
  // Allow playing from the mic.
  // (Mic comes from setRecordDriver and windows' audio control panel.)
  if (!ar_fmodcheck( FMOD_System_RecordStart( ar_fmod(), samp1, true ))) {
    ar_log_error() << "failed to start recording.\n";
    // If you get the error "fmod: A call to a standard soundcard driver failed,"
    // you may need to actually plug in a microphone.
    // Also ensure that your dbatch file enables sound.

    // Don't abort.  Let the DSP start.
  }
#endif

  // Start playing (and recording) paused,
  // to reduce delay between sound and visualizations thereof.
  if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(),
          FMOD_CHANNEL_FREE, samp1, true, &_recordChannel )) ||
      !ar_fmodcheck( FMOD_Channel_SetVolume( _recordChannel, _microphoneVolume )) ||
      !ar_fmodcheck( FMOD_Channel_SetPaused( _recordChannel, true ))) {
    return false;
  }

#ifdef Busted_on_zx81
  // Start the DSP, which taps into the sample stream to e.g. compute its FFT.
  FMOD_DSP_DESCRIPTION d = {0};
  d.read = ar_soundClientDSPCallback;
  return ar_fmodcheck( FMOD_System_CreateDSP( ar_fmod(), &d, &_DSPunit )) &&
         ar_fmodcheck( FMOD_System_AddDSP( ar_fmod(), _DSPunit )) &&
         ar_fmodcheck( FMOD_DSP_SetActive( _DSPunit, true ));
#else
  return true;
#endif
#endif
}

void arSoundClient::relayWaveform() {
  if (_dspTap)
    _dspTap(_waveDataPtr);

  if (_dataServerRegistered) {
    // Post the data to the network.
    _dataServer.sendData(_waveData);
  }
}

void arSoundClient::setDSPTap(void (*callback)(float*)) {
  _dspTap = callback;
}

bool arSoundClient::microphoneVolume(int volume) {
  _microphoneVolume = (volume < 0) ? 0 : (volume > 255) ? 255 : volume;
#ifdef EnableSound
  if (_recordChannel) {
    if (!ar_fmodcheck( FMOD_Channel_SetVolume( _recordChannel, _microphoneVolume / 255. )))
      return false;

    // bug: don't make the same SetPaused call twice in a row.
    // zero-check saves CPU, it's silent anyways.
    if (!ar_fmodcheck( FMOD_Channel_SetPaused( _recordChannel, _microphoneVolume == 0 )))
      return false;
  }
#endif
  return true;
}

bool arSoundClient::_initSound() {
#ifndef EnableSound
  ar_log_error() << "silent, compiled without FMOD (e.g., $(SZGEXTERNAL)/*/fmod-4).\n";
  return false;

#else

  if (!ar_fmodcheck( FMOD_System_SetSoftwareFormat( ar_fmod(),
          44100, FMOD_SOUND_FORMAT_PCM16, 0, 0, FMOD_DSP_RESAMPLER_LINEAR ))) {
    ar_log_error() << "fmod audio failed to init.\n";
    return false;
  }

  const float feetNotMeters = 3.28;
  // JAC 11/18/08 experimenting with higher rolloff. used to be .008
  const int numVirtualVoices = 40;
  return ar_fmodcheck( FMOD_System_Set3DSettings( ar_fmod(), 1.0, feetNotMeters, _rolloff )) &&
         ar_fmodcheck( FMOD_System_Init( ar_fmod(),
             numVirtualVoices, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0 ));

  // ar_fmodcheck(ar_fmod()->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));
  // FMOD_OUTPUTTYPE_ASIO or FMOD_OUTPUTTYPE_DSOUND
  // linux: FMOD_OUTPUTTYPE_ALSA FMOD_OUTPUTTYPE_OSS

#endif
}

string arSoundClient::_processStreamInfo(const string& body) {
  int nodeID = -1;
  if (!ar_stringToIntValid(body, nodeID)) {
    ar_log_error() << "stream_info message had invalid ID.\n";
    return string("SZG_ERROR");
  }

  // getNode() may be unsafe here when database nodes are deleted, or even added.
  arDatabaseNode* node = _soundDatabase.getNode(nodeID);
  if (!node || node->getTypeCode() != AR_S_STREAM_NODE) {
    ar_log_error() << "stream_info message found no node.\n";
    return string("SZG_ERROR");
  }

  // node->getTypeCode() confirms that this cast is valid.
  arStreamNode* tmp = (arStreamNode*)node;
  return tmp->getCurrentTime() + "/" + ar_intToString(tmp->getStreamLength());
}
