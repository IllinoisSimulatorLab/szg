/*
Loopback example of getting bytes from, and sending bytes to,
a soundcard via FMOD.

data source:
DSP_ExampleCallback appends stuff to an accumulator-buffer.
[another thread] periodically calls cb = getBytes(char* pb, int cbMax)

data sink:
this guy calls getBytes(), and sends the stuff to an fmod-soundplayer.

TODO:  change printf's to cout and cerr.
*/
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arThread.h"
#include "arSZGClient.h"
#include "fmodStub.h"
#include <math.h>

bool fQuit = false;

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    cli->receiveMessage(&messageType,&messageBody);
    if (messageType=="quit"){
      fQuit = true;
      return;
    }
  }
}

const int mySR = 44100;
const int msecRecord = 5000; // big, so buffer-wrapping is rarer.
const int samplesBuf = mySR*msecRecord/1000;

// We can change msecLatency on the fly if we want!
int msecLatency  = 30; // DSP uses 25-msec buffers, don't go less than that.

int samplesLatency() { return int(msecLatency * mySR / 1000.); }

FSOUND_DSPUNIT* Unit;

char* bufAcc = NULL;
const int samplesAcc = mySR * 2;
const int cbAcc = samplesAcc * 4;
int ibAcc = 0;

void accAppend(const void* pb, int cb) {
  if (cb <= 0)
    return;

  // oversimplistic overflow handling
  if (cb > cbAcc)
    cb = cbAcc;
  if (ibAcc + cb > cbAcc)
    ibAcc = 0;

  memcpy(bufAcc + ibAcc, pb, cb);
  ibAcc += cb;
}

int getBytes(void* pb, int cb) {
  if (cb > ibAcc)
    cb = ibAcc; // reduce request size to what's available
  memcpy(pb, bufAcc, cb);
  // shift things over (simpler than a circular buffer)
  memmove(bufAcc, bufAcc+cb, ibAcc-=cb);
  return cb;
}

const int cbPlayMax = mySR*4*2; // 2 seconds is plenty
char* bufPlay = NULL;
int ibPlay = 0;
arMutex lockPlay;

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

void* SZG_CALLBACK DSP_ExampleCallback(void *bufSrc, void *bufDst, int length, int) {
#if 0
  // Might need this for some soundcards.
  const int mixertype = FSOUND_GetMixer();
  if (mixertype==FSOUND_MIXER_BLENDMODE || mixertype==FSOUND_MIXER_QUALITY_FPU)
    return bufDst;
#endif

  int cb = length*4;      // 4 bytes:  16-bit stereo
  accAppend(bufSrc, cb);

#undef SILENT_OUTPUT
#ifdef SILENT_OUTPUT

  // Do this on the record side, for silent output there.
  // Or just "return bufDst;" to echo input to output.

  memset(bufDst, 0, cb);

#else

  // Do this on the playback side, to inject bytes into the playback buffers.

  ar_mutex_lock(&lockPlay);
    if (ibPlay < cb) {
      // bufPlay needs more bytes than bufDst has, so pad it with zeros.
      memset((char*)bufPlay+cb, 0, cb-ibPlay);
      ibPlay = cb;
    }
    // memset from bufPlay, then shrink bufPlay.
    memcpy(bufDst, bufPlay, cb);
    memmove(bufPlay, bufPlay+cb, ibPlay-=cb);
  ar_mutex_unlock(&lockPlay);

#endif

  return bufDst;
}

void SetupExample() {
  Unit = FSOUND_DSP_Create(&DSP_ExampleCallback,
    FSOUND_DSP_DEFAULTPRIORITY_USER+20, 0);
  FSOUND_DSP_SetActive(Unit, 1);
}

void CloseExample() {
  FSOUND_DSP_Free(Unit);
  Unit = NULL;
}

int cbPlayed = 0;

void consumerTask(void*) {
  while (!fQuit) {
    ar_usleep(10000 * (1 + rand()%4)); // 10 to 40 msec, simulated entropy

    ar_mutex_lock(&lockPlay);
      cbPlayed = getBytes(bufPlay+ibPlay, cbPlayMax-ibPlay);
      ibPlay += cbPlayed;
      if (ibPlay > cbPlayMax)
        ibPlay = 0; // paranoia, this should never happen.
    ar_mutex_unlock(&lockPlay);
  }
}

int main(int argc, char** argv) {
  if (argc > 3) {
    cerr << "usage: " << argv[0] << " [soundcard_number]\n";
    return 1;
  }

  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  if (FSOUND_GetVersion() < FMOD_VERSION) {
    printf("Error: wrong fmod.dll, expected version %.02f\n", FMOD_VERSION);
    return 1;
  }

  FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);

  const int numSoundcards = FSOUND_GetNumDrivers();
  const int numSoundcardsRecord = FSOUND_Record_GetNumDrivers();
  int iSoundcard = argc<2 ? 0 : atoi(argv[1]);
  if (iSoundcard != 0 && numSoundcards == 1) {
    cerr << argv[0] << " warning: ignoring nondefault soundcard "
         << iSoundcard << " on 1-card host.\n";
    iSoundcard = 0;
  }
  if (numSoundcards > 1) {
    int i;
    printf("DirectSound play soundcards found:\n");
    for (i=0; i < numSoundcards; i++)
      printf("%s\t%d: %s\n",
        i==iSoundcard ? "  -->" : "",
        i, FSOUND_GetDriverName(i));

    // Typically, record and play are the same list.
    printf("DirectSound record soundcards found:\n");
    for (i=0; i < numSoundcardsRecord; i++)
      printf("%s\t%d: %s\n",
        i==iSoundcard ? "  -->" : "",
        i, FSOUND_Record_GetDriverName(i));
  }

  // Select sound card and input sound card (0 = default)
  if (!FSOUND_SetDriver(iSoundcard) ||
      !FSOUND_Record_SetDriver(iSoundcard)) {
    printf("Error 1!\n");
    //printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
    return 1;
  }

  (void)FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);
  if (!FSOUND_Init(mySR, 64, 0)) {
    printf("Error 2!\n");
    //printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
    return 1;
  }

  bufAcc = new char[cbAcc];
  bufPlay = new char[cbPlayMax];
#ifdef EnableSound
  FSOUND_SAMPLE *samp1 = FSOUND_Sample_Alloc(FSOUND_UNMANAGED, samplesBuf,
    FSOUND_MONO | FSOUND_16BITS, mySR, 255, 128, 255);

  (void)FSOUND_Sample_SetMode(samp1, FSOUND_LOOP_NORMAL);

  if (!FSOUND_Record_StartSample(samp1, 1)) {
    printf("Error 3!\n");
    //printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
    FSOUND_Close();
    return 1;
  }
#endif

  // Let the record cursor move forward a little bit before we try to play
  // (the position jumps in blocks, so any nonzero value means
  // 1 block has been recorded).
  while (FSOUND_Record_GetPosition() == 0)
    ar_usleep(1000);

  SetupExample();

#ifdef EnableSound
  const int channel = FSOUND_PlaySound(FSOUND_FREE, samp1);
  const int originalfreq = FSOUND_GetFrequency(channel);
#endif

  ar_mutex_init(&lockPlay);
  arThread dummy1(messageTask, &szgClient);
  arThread dummy2(consumerTask);

  int gap = 0;
  int recordposPrev = 0, playposPrev = 0;
#ifdef EnableSound
  int freq = originalfreq;
#endif
  while (!fQuit) {
    const int playpos = FSOUND_GetCurrentPosition(channel);
    const int recordpos = FSOUND_Record_GetPosition();


    // Adjust playback frequency, but not if either cursor just wrapped.
    if (playpos > playposPrev && recordpos > recordposPrev) {
      if ((gap = recordpos - playpos) < 0)
        gap += samplesBuf;

#ifdef EnableSound
      int freqnew = originalfreq;
      if (gap < samplesLatency())
        freqnew -= 1000;
      else if (gap > samplesLatency() * 2)
        freqnew += 1000;
      if (freqnew != freq)
        FSOUND_SetFrequency(channel, freqnew);
      freq = freqnew;
#endif
    }
    playposPrev = playpos;
    recordposPrev = recordpos;

    // print some info and a VU meter
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

    ar_usleep(10000);
  }

  FSOUND_StopSound(channel);
  (void)FSOUND_Record_Stop();
  CloseExample();
  FSOUND_Close();
  delete [] bufAcc;
  delete [] bufPlay;
  return 0;
}
