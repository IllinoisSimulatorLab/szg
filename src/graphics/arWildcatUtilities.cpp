//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arWildcatUtilities.h"
#include "arDataUtilities.h"
#include "arLogStream.h"

namespace arWildcatNamespace {
  arLock __wildcatMutex;
  bool __useWildcatFramelock = false;
  bool __frameLockInitted = false;
#ifdef AR_USE_WIN_32
  // Hack for Wildcat cards' broken glFinish().  Framelock must be on.
  typedef BOOL (APIENTRY *WGLENABLEFRAMELOCKI3DFUNCPTR)(VOID);
  typedef BOOL (APIENTRY *WGLDISABLEFRAMELOCKI3DFUNCPTR)(VOID);
  static BOOL (__stdcall *__wglEnableFrameLockI3D)(void) = 0;
  static BOOL (__stdcall *__wglDisableFrameLockI3D)(void) = 0;
#endif
};

using namespace std;
using namespace arWildcatNamespace;

void ar_useWildcatFramelock( bool isOn ) {
  __wildcatMutex.lock();
  __useWildcatFramelock = isOn;
  __wildcatMutex.unlock();
}

// It is really annoying to have Wildcat-specific code in this file.
// A little bit of a design to-do, I suppose.
void ar_findWildcatFramelock() {
#ifdef AR_USE_WIN_32
  __wildcatMutex.lock();
  if (__useWildcatFramelock) {
    // need to try to find the Wildcat card's frame-locking functions here
      __wglEnableFrameLockI3D 
        = (WGLENABLEFRAMELOCKI3DFUNCPTR) 
            wglGetProcAddress("wglEnableFrameLockI3D");
      if (__wglEnableFrameLockI3D == 0) {
        ar_log_error() << "wglEnableFrameLockI3D not found.\n";
      }
      __wglDisableFrameLockI3D = (WGLDISABLEFRAMELOCKI3DFUNCPTR)
        wglGetProcAddress("wglDisableFrameLockI3D");
      if (__wglDisableFrameLockI3D == 0) {
        ar_log_error() << "wglDisableFrameLockI3D not found.\n";
      }
  }
  __wildcatMutex.unlock();
#endif
}

void ar_activateWildcatFramelock() {
#ifdef AR_USE_WIN_32
  __wildcatMutex.lock();
  if (!__frameLockInitted) {
    __frameLockInitted = true;

    if (__wglEnableFrameLockI3D != 0 && __useWildcatFramelock) {
      if (__wglEnableFrameLockI3D() == FALSE) {
	ar_log_warning() << "wglEnableFrameLockI3D failed.\n";
      }
      else{
	ar_log_debug() << "wglEnableFrameLockI3D.\n";
      }
    }
  }
  __wildcatMutex.unlock();
#endif
}

void ar_deactivateWildcatFramelock() {
#ifdef AR_USE_WIN_32
  __wildcatMutex.lock();
  if (__frameLockInitted){

    if (__wglDisableFrameLockI3D != 0 && __useWildcatFramelock) {
      if (__wglDisableFrameLockI3D() == FALSE) {
        ar_log_warning() << "wglDisableFrameLockI3D failed.\n";
      }
      else{
        ar_log_debug() << "wglDisableFrameLockI3D.\n";
      }

      __frameLockInitted = false;
    }
  }
  __wildcatMutex.unlock();
#endif
}
