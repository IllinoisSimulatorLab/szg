/*
Loopback example of getting bytes from, and sending bytes to,
a soundcard via FMOD.

data source:
DSP_ExampleCallback appends stuff to an accumulator-buffer.
[another thread] periodically calls cb = getBytes(char* pb, int cbMax)

data sink:
calls getBytes(), and sends the stuff to an fmod-soundplayer.

TODO:  change printf's to cout and cerr.
*/

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"
#include "fmodStub.h"
#include <math.h>

bool fQuit = false;

void messageTask(void* pClient){
  string messageType, messageBody;
  while (true) {
    ((arSZGClient*)pClient)->receiveMessage(&messageType, &messageBody);
    if (messageType=="quit"){
      fQuit = true;
      return;
    }
  }
}

const int mySR = 44100;
const int msecRecord = 5000; // big, to avoid buffer wraparound
const int samplesBuf = mySR*msecRecord/1000;

// msecLatency can change on the fly if we want.
int msecLatency  = 30; // DSP uses 25-msec buffers, don't go less than that.

int samplesLatency() { return int(msecLatency * mySR / 1000.); }


char* bufRec = NULL;
const int samplesRec = mySR * 2;
const int cbRec = samplesRec * 4;
int ibRec = 0;

void accAppend(const void* pb, int cb) {
  if (cb <= 0)
    return;

  // oversimplistic overflow handling
  if (cb > cbRec)
    cb = cbRec;
  if (ibRec + cb > cbRec)
    ibRec = 0;

  memcpy(bufRec + ibRec, pb, cb);
  ibRec += cb;
}

int getBytes(void* pb, int cb) {
  if (cb > ibRec)
    cb = ibRec; // reduce request size to what's available
  memcpy(pb, bufRec, cb);
  // shift things over (simpler than a circular buffer)
  memmove(bufRec, bufRec+cb, ibRec-=cb);
  return cb;
}

const int cbPlayMax = mySR*4*2; // 2 seconds is plenty
char* bufPlay = NULL;
int ibPlay = 0;
arLock lockPlay;
#ifdef EnableSound
FMOD_DSP* Unit = NULL;
#endif

/*
  'bufSrc'  Pointer to the original mixbuffer, not any buffers passed down
            through the dsp chain.  They are in bufDst.
  'bufDst'      Pointer to buffer passed from previous DSP unit.
  'length'      buffer length in samples.
*/

#ifdef AR_USE_WIN_32
#define SZG_CALLBACK __stdcall
#else
#define SZG_CALLBACK
#endif

// This is an FMOD_DSP_READCALLBACK.
// array of floats, the actual signal (normalized to -1 ... +1)
// number of samples in the array
// number of channels of interleaved data (4 would be quadraphonic, e.g.)
// number of channels of output data
// array of floats to be filled with the filtered input signal.

#ifdef EnableSound
FMOD_RESULT SZG_CALLBACK DSP_ExampleCallback(
    FMOD_DSP_STATE* /*pState*/,
    float *  bufSrc, 
    float *  bufDst, 
    unsigned int  length, 
    int  /*inchannels*/, 
  int  outchannels){

#ifdef UNUSED
  const FMOD::DSP* pdsp = (const FMOD::DSP*)(pState->instance);
#endif
  // pdsp should == Unit.
  // pState->plugindata for something?
  const int cb = length*2*outchannels;      // 2 bytes: 16-bit
  accAppend(bufSrc, cb);

#undef SILENT_OUTPUT
#ifdef SILENT_OUTPUT

  // Do this on the record side, for silent output there.
  // Or just "return bufDst;" to echo input to output.

  memset(bufDst, 0, cb);

#else

  // Do this on the playback side, to inject bytes into the playback buffers.

  lockPlay.lock();
    if (ibPlay < cb) {
      // bufPlay needs more bytes than bufDst has, so pad it with zeros.
      memset((char*)bufPlay+cb, 0, cb-ibPlay);
      ibPlay = cb;
    }
    // memset from bufPlay, then shrink bufPlay.
    memcpy(bufDst, bufPlay, cb);
    memmove(bufPlay, bufPlay+cb, ibPlay-=cb);
  lockPlay.unlock();

#endif

  return FMOD_OK;
}

bool SetupExample() {
  struct FMOD_DSP_DESCRIPTION d = {0};
  strcpy(d.name, "Slartibartfast");
  d.version = 42;
  d.channels = 0; // default (use another value to generate a particular number of channels)
  d.read = DSP_ExampleCallback;
  return ar_fmodcheck( FMOD_System_CreateDSP( ar_fmod(), &d, &Unit )) &&
  	 ar_fmodcheck( FMOD_DSP_SetActive( Unit, true ) ) &&
  	 ar_fmodcheck( FMOD_System_AddDSP( ar_fmod(), Unit ));
}

bool CloseExample() {
  const bool ok = ar_fmodcheck( FMOD_DSP_Remove(Unit)) && ar_fmodcheck( FMOD_DSP_Release(Unit));
  Unit = NULL;
  return ok;
}
#endif

int cbPlayed = 0;

void consumerTask(void*) {
  while (!fQuit) {
    ar_usleep(10000 * (1 + rand()%4)); // 10 to 40 msec, simulated entropy

    lockPlay.lock();
      cbPlayed = getBytes(bufPlay+ibPlay, cbPlayMax-ibPlay);
      ibPlay += cbPlayed;
      if (ibPlay > cbPlayMax)
        ibPlay = 0; // paranoia, this should never happen.
    lockPlay.unlock();
  }
}

int main(int argc, char** argv) {

  arSZGClient szgClient;
  (void)szgClient.init(argc, argv); // standalone is ok

  if (argc > 2) {
    cerr << "usage: " << argv[0] << " [soundcard_number]\n";
    return 1;
  }

  {
    unsigned t;
    if (!ar_fmodcheck( FMOD_System_GetVersion( ar_fmod(), &t )))
      return 1;
    if (t < FMOD_VERSION) {
      printf("Error: fmod dll has version %x, expected %x.\n", t, FMOD_VERSION);
      return 1;
    }
  }

  int cDriverPlay;
  int cDriverRec;
  if (!ar_fmodcheck( FMOD_System_GetNumDrivers( ar_fmod(), &cDriverPlay )) ||
      !ar_fmodcheck( FMOD_System_GetRecordNumDrivers( ar_fmod(), &cDriverRec )))
    return 1;

  int iDriver = argc<2 ? 0 : atoi(argv[1]);
  if (iDriver != 0 && cDriverPlay == 1) {
    cerr << argv[0] << " warning: host has only one sound card.\n";
    iDriver = 0;
  }
  if (iDriver > cDriverPlay || iDriver > cDriverRec) {
    cerr << argv[0] << " warning: host doesn't have that many sound cards.\n";
    iDriver = 0;
  }

  if (cDriverPlay > 1) {
    int i = 0;
    printf("Soundcards:\n");
    for (i=0; i < cDriverPlay; i++) {
      char sz[200];
      if (!ar_fmodcheck( FMOD_System_GetDriverName( ar_fmod(), i, sz, sizeof(sz)-1) ))
        return 1;
      printf("%s%d: %s\n",
        i==iDriver ? "* " : "  ",
        i, sz);
    }
  }

  if (cDriverRec > 1) {
    int i = 0;
    printf("Recording soundcards:\n");
    for (i=0; i < cDriverRec; i++) {
      char sz[200];
      if (!ar_fmodcheck( FMOD_System_GetRecordDriverName( ar_fmod(),
              i, sz, sizeof(sz)-1 )))
        return 1;
      printf("%c%d: %s\n",
        i==iDriver ? '*' : ' ',
        i, sz);
    }
  }

  // Select sound cards (0 = default)
  if (!ar_fmodcheck( FMOD_System_SetDriver( ar_fmod(), iDriver ))) {
    cerr << "Failed to set driver.\n";
    return 1;
  }
  if (!ar_fmodcheck( FMOD_System_SetRecordDriver( ar_fmod(), iDriver ))) {
    cerr << "Failed to set record driver.\n";
    return 1;
  }

  if (!ar_fmodcheck( FMOD_System_SetSoftwareFormat( ar_fmod(), 
           int(mySR), FMOD_SOUND_FORMAT_PCM16, 0, 0, FMOD_DSP_RESAMPLER_LINEAR)) ||
      !ar_fmodcheck( FMOD_System_Init( ar_fmod(), 
	   50/*numVirtualVoices*/, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0))) {
    cerr << "Failed to init fmod.\n";
    return 1;
  }

	//;;;; FMOD_3D_HEADRELATIVE is possible.

  bufRec = new char[cbRec*3]; // *3 is chicken factor
  bufPlay = new char[cbPlayMax];
#ifdef EnableSound
  struct FMOD_CREATESOUNDEXINFO x = {0};
  x.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  x.numchannels = 1;
  x.format = FMOD_SOUND_FORMAT_PCM16;
  x.decodebuffersize = cbRec;
  x.defaultfrequency = mySR;
  x.length = x.defaultfrequency * sizeof(short) * x.numchannels * 5; // What's the 5 for?
  FMOD_SOUND *samp1 = NULL;
  if (!ar_fmodcheck( FMOD_System_CreateSound( ar_fmod(), 
    NULL/*bufRec*/,
    FMOD_2D | FMOD_SOFTWARE | FMOD_OPENUSER | FMOD_LOOP_NORMAL,
    // FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_OPENMEMORY | FMOD_LOOP_NORMAL,
    // FMOD_CREATESTREAM | FMOD_OPENMEMORY | FMOD_LOOP_NORMAL,
    &x,
    &samp1 ))) {
    ar_log_error() << "FMOD failed to create record stream.\n";
LAbort:
    (void)ar_fmodcheck( FMOD_System_Release( ar_fmod() ));
    return 1;
  }

  if (!ar_fmodcheck( FMOD_System_RecordStart( ar_fmod(), samp1, true ))) {
    ar_log_error() << "FMOD failed to record.\n";
    goto LAbort;
  }

  // Before playing, wait until the record cursor has advanced
  // (the position jumps in blocks, so any nonzero value means
  // 1 block has been recorded).
  {
    unsigned t = 0;
    do {
      ar_usleep(1000);
      if (!ar_fmodcheck( FMOD_System_GetRecordPosition( ar_fmod(), &t )))
        goto LAbort;
    }
    while (t == 0);
  }

  if (!SetupExample())
    return 1;

  FMOD_CHANNEL* channel = NULL;
  if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, samp1, false, &channel )))
    return 1;
  float originalFreq;
  if (!ar_fmodcheck( FMOD_Channel_GetFrequency( channel, &originalFreq )))
    return 1;
#endif

  arThread dummy1(messageTask, &szgClient);
  arThread dummy2(consumerTask);

#ifdef EnableSound
  unsigned recordposPrev = 0;
  unsigned playposPrev = 0;
  float freq = originalFreq;
  while (!fQuit) {
    unsigned playpos;
    unsigned recordpos;
    if (!ar_fmodcheck( FMOD_Channel_GetPosition( channel, &playpos, FMOD_TIMEUNIT_PCM )) ||
        !ar_fmodcheck( FMOD_System_GetRecordPosition( ar_fmod(), &recordpos ))) { // in PCM units
      fQuit = true;
      goto LAbort;
    }

    // Adjust playback frequency, but not if either cursor just wrapped.
    if (playpos > playposPrev && recordpos > recordposPrev) {
      int gap = recordpos - playpos;
      if (gap < 0)
        gap += samplesBuf;

      float freqnew = originalFreq;
      if (gap < samplesLatency())
        freqnew -= 1000;
      else if (gap > samplesLatency() * 2)
        freqnew += 1000;
      if (freqnew != freq) {
        FMOD_Channel_SetFrequency( channel, freqnew );
	freq = freqnew;
      }
    }
    playposPrev = playpos;
    recordposPrev = recordpos;

#ifdef not_yet_ported
    // fmod 4 has no VU meter AFAIK.  "levels" are input levels set through API.
    // We'd need to scan the buffers ourselves to compute the current loudness.
    // Look at fmod4api/examples, and vss, for code.

    // print info and a VU meter
    {
      static float smoothedvu = 0.;
      const int vuLen = 50;
      // sqrt is a hack, should use dB properly.
      // the fmod FSOUND_GetCurrentVU call changed... no point in fixing
      const float vuval = 0;
      //const float vuval = sqrt(FSOUND_GetCurrentVU(channel)) * (vuLen+.99);
      if (vuval > smoothedvu)
        smoothedvu = vuval;

      char vu[vuLen];
      memset(vu, ' ', vuLen-1);
      memset(vu, '=', int(smoothedvu));
      vu[vuLen-1] = '\0';

      printf("Level: |%s|\r", vu);

      const float VUSPEED = 1.f; // bigger is faster decay
      if ((smoothedvu -= VUSPEED) < 0.)
        smoothedvu = 0.;
    }
#endif

    ar_usleep(10000);
  }
#endif

#ifdef EnableSound
  (void)ar_fmodcheck( FMOD_Sound_Release( samp1 ));
  (void)ar_fmodcheck( FMOD_System_RecordStop( ar_fmod() ));
  (void)CloseExample();
  (void)ar_fmodcheck( FMOD_System_Release( ar_fmod() ));
#endif
  delete [] bufRec;
  delete [] bufPlay;
  return 0;
}
