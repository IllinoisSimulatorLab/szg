//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFramelockUtilities.h"
#include "arLogStream.h"

namespace arFramelockNamespace {
  arLock lock;
  bool fUseFramelock = false;
  bool fActive = false;
#ifdef AR_USE_WIN_32
  // Hack for  cards' broken glFinish().  Framelock must be on.
  typedef BOOL (APIENTRY *pfn)(void);
  static BOOL (__stdcall *__wglEnableFrameLockI3D)(void) = 0;
  static BOOL (__stdcall *__wglDisableFrameLockI3D)(void) = 0;
#endif
};

void ar_useFramelock( bool f ) {
  arGuard _(arFramelockNamespace::lock, "ar_useFramelock");
  arFramelockNamespace::fUseFramelock = f;
}

#ifdef AR_USE_WIN_32

// -specific code.

void ar_findFramelock() {
  arGuard _(arFramelockNamespace::lock, "ar_findFramelock");
  if (!arFramelockNamespace::fUseFramelock)
    return;

  arFramelockNamespace::__wglEnableFrameLockI3D = (arFramelockNamespace::pfn) 
    wglGetProcAddress("wglEnableFrameLockI3D");
  if (!arFramelockNamespace::__wglEnableFrameLockI3D) {
    ar_log_error() << "No wglEnableFrameLockI3D.\n";
  }
  arFramelockNamespace::__wglDisableFrameLockI3D = (arFramelockNamespace::pfn)
    wglGetProcAddress("wglDisableFrameLockI3D");
  if (!arFramelockNamespace::__wglDisableFrameLockI3D) {
    ar_log_error() << "No wglDisableFrameLockI3D.\n";
  }
}

void ar_activateFramelock() {
  arGuard _(arFramelockNamespace::lock, "ar_activateFramelock");
  if (arFramelockNamespace::fActive) {
    ar_log_error() << "Ignoring duplicate ar_activateFramelock.\n";
    return;
  }

  arFramelockNamespace::fActive = true;
  if (!arFramelockNamespace::__wglEnableFrameLockI3D || !arFramelockNamespace::fUseFramelock)
    return;
  
  if (arFramelockNamespace::__wglEnableFrameLockI3D() == FALSE) {
    ar_log_error() << "wglEnableFrameLockI3D failed.\n";
  }
  else{
    ar_log_debug() << "wglEnableFrameLockI3D ok.\n";
  }
}

void ar_deactivateFramelock() {
  arGuard _(arFramelockNamespace::lock, "ar_deactivateFramelock");
  if (!arFramelockNamespace::fActive) {
    ar_log_error() << "Ignoring duplicate ar_deactivateFramelock.\n";
    return;
  }

  arFramelockNamespace::fActive = false;
  if (!arFramelockNamespace::__wglDisableFrameLockI3D || !arFramelockNamespace::fUseFramelock)
    return;

  if (arFramelockNamespace::__wglDisableFrameLockI3D() == FALSE) {
    ar_log_error() << "wglDisableFrameLockI3D failed.\n";
  }
  else{
    ar_log_debug() << "wglDisableFrameLockI3D ok.\n";
  }
}

#else

void ar_findFramelock() {}
void ar_activateFramelock() {}
void ar_deactivateFramelock() {}

#endif
