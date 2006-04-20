#ifndef AR_FMODSTUB_H
#define AR_FMODSTUB_H

#ifdef EnableSound

#include "fmod.hpp"
#include "fmod_errors.h"

#include "arMath.h"
static inline FMOD_VECTOR FmodvectorFromArvector(const arVector3& rhs) {
  // could just use a cast, but this is safer
  FMOD_VECTOR lhs;
  lhs.x = rhs.v[0];
  lhs.y = rhs.v[1];
  lhs.z = rhs.v[2];
  return lhs;
}

extern FMOD::System* ar_fmod();
extern bool ar_fmodcheck3(const FMOD_RESULT, const char*, int);
#define ar_fmodcheck(_) ar_fmodcheck3(_, __FILE__, __LINE__)

#else

#ifdef AR_USE_WIN_32
#define FMOD_VERSION 0x00040305 /* 4.03.05 */
#else
#define FMOD_VERSION 0x00040306 /* 4.03.06 */
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
