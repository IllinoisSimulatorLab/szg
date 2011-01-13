#ifndef AR_FMODSTUB_H
#define AR_FMODSTUB_H

#ifdef EnableSound

#include "fmod.h"
#include "fmod_errors.h"

#include "arMath.h"
static inline FMOD_VECTOR FmodvectorFromArvector(const arVector3& rhs) {
  // safer than a cast
  FMOD_VECTOR lhs;
  lhs.x = rhs.v[0];
  lhs.y = rhs.v[1];
  lhs.z = rhs.v[2];
  return lhs;
}

extern SZG_CALL FMOD_SYSTEM* ar_fmod();
extern SZG_CALL bool ar_fmodcheck3(const FMOD_RESULT, const char*, int);
#define ar_fmodcheck(_) ar_fmodcheck3(_, __FILE__, __LINE__)

#else

#ifdef AR_USE_WIN_32
#define FMOD_VERSION 0x00040305 /* 4.03.05 */
#else
#define FMOD_VERSION 0x00040306 /* 4.03.06 (incorrect for 64-bit fmod 4.30.02, but it still runs) */
#endif

namespace FMOD
{
typedef int DSP;
typedef int Sound;
}

// inline int FSOUND_Init(int, int, int)
// { cerr << "syzygy warning: FMOD disabled, compiled with stub.\n"; return 0; }

#define FmodvectorFromArvector(_) 0
#define ar_fmod() NULL
#define ar_fmodcheck(_) false

#endif
#endif

#if FMOD_VERSION == 0x00043002
  #undef Busted_on_zx81
#else
  #define Busted_on_zx81
#endif

