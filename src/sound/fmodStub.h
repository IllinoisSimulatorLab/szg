#ifndef AR_FMODSTUB_H
#define AR_FMODSTUB_H

#ifdef EnableSound
#include "fmod.h"
#else

#define FMOD_VERSION 3.4f

#define FSOUND_16BITS 0
#define FSOUND_FREE 0
#define FSOUND_INIT_USEDEFAULTMIDISYNTH 0
#define FSOUND_LOADMEMORY 0
#define FSOUND_LOOP_NORMAL 0
#define FSOUND_LOOP_OFF 0
#define FSOUND_MIXER_QUALITY_AUTODETECT 0
#define FSOUND_MONO 0
#define FSOUND_SIGNED 0
#define FSOUND_UNMANAGED 0

typedef int FSOUND_DSPUNIT;
typedef int FSOUND_SAMPLE;

#define FSOUND_Record_GetNumDrivers() 0
#define FSOUND_3D_Listener_SetAttributes(_1,_2,_3,_4,_5,_6,_7,_8)
#define FSOUND_3D_SetDistanceFactor(_)
#define FSOUND_3D_Listener_SetDistanceFactor(_)
#define FSOUND_3D_SetRolloffFactor(_)
#define FSOUND_3D_Listener_SetRolloffFactor(_)
#define FSOUND_3D_SetAttributes(_,__,___)
#define FSOUND_Update()
#define FSOUND_3D_Update()
#define FSOUND_Close()
#define FSOUND_DSP_Create(_,__,___) NULL
#define FSOUND_DSP_Free(_)
#define FSOUND_DSP_SetActive(_,__)
#define FSOUND_GetCurrentPosition(_) 0
#define FSOUND_GetCurrentVU(_) 0.0
#define FSOUND_GetDriverName(_) ""
#define FSOUND_Record_GetDriverName(_) ""
#define FSOUND_GetFrequency(_) 0
#define FSOUND_GetNumDrivers() 0
#define FSOUND_GetOutput() 0
#define FSOUND_PlaySound(_,__) 0
#define FSOUND_PlaySoundEx(_1,_2,_3,_4)
#define FSOUND_Record_GetPosition() 0
#define FSOUND_Record_SetDriver(_) 0
#define FSOUND_Record_StartSample(_,__) 0
#define FSOUND_Record_Stop() 0
#define FSOUND_Sample_Alloc(_1,_2,_3,_4,_5,_6,_7) NULL
//#define FSOUND_Sample_Free(_)
#define FSOUND_Sample_Free(_) NULL

#ifdef USE_FMOD_3_6
#define FSOUND_Sample_Load(_1,_2,_3,_4) NULL
#else
#define FSOUND_Sample_Load(_1,_2,_3,_4,_5) NULL
#endif

#define FSOUND_Sample_SetLoopMode(_,__) 0
#define FSOUND_SetDriver(_) 0
#define FSOUND_SetFrequency(_,__) 0
#define FSOUND_SetMixer(_) 0
#define FSOUND_SetOutput(_)
#define FSOUND_SetPaused(_,__)
#define FSOUND_SetSFXMasterVolume(_)
#define FSOUND_SetVolume(_,__)
#define FSOUND_StopSound(_)

using namespace std; // for cerr, in win32
inline int FSOUND_Init(int, int, int)
  { cerr << "syzygy warning: FMOD disabled, compiled with stub.\n"; return 0; }
inline float FSOUND_GetVersion()
  { FSOUND_Init(0,0,0); return -1.; }

#endif

#endif
