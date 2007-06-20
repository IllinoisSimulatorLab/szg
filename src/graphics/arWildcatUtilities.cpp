//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arWildcatUtilities.h"
#include "arLogStream.h"

namespace arWildcatNamespace {
  arLock lock;
  bool fUseFramelock = false;
  bool fActive = false;
#ifdef AR_USE_WIN_32
  // Hack for Wildcat cards' broken glFinish().  Framelock must be on.
  typedef BOOL (APIENTRY *pfn)(void);
  static BOOL (__stdcall *__wglEnableFrameLockI3D)(void) = 0;
  static BOOL (__stdcall *__wglDisableFrameLockI3D)(void) = 0;
#endif
};

void ar_useWildcatFramelock( bool f ) {
  arGuard dummy(arWildcatNamespace::lock);
  arWildcatNamespace::fUseFramelock = f;
}

#ifdef AR_USE_WIN_32

// Wildcat-specific code.

void ar_findWildcatFramelock() {
  arGuard dummy(arWildcatNamespace::lock);
  if (!arWildcatNamespace::fUseFramelock)
    return;

  arWildcatNamespace::__wglEnableFrameLockI3D = (arWildcatNamespace::pfn) 
    wglGetProcAddress("wglEnableFrameLockI3D");
  if (!arWildcatNamespace::__wglEnableFrameLockI3D) {
    ar_log_warning() << "No wglEnableFrameLockI3D.\n";
  }
  arWildcatNamespace::__wglDisableFrameLockI3D = (arWildcatNamespace::pfn)
    wglGetProcAddress("wglDisableFrameLockI3D");
  if (!arWildcatNamespace::__wglDisableFrameLockI3D) {
    ar_log_warning() << "No wglDisableFrameLockI3D.\n";
  }
}

void ar_activateWildcatFramelock() {
  arGuard dummy(arWildcatNamespace::lock);
  if (arWildcatNamespace::fActive) {
    ar_log_warning() << "Ignoring duplicate ar_activateWildcatFramelock.\n";
    return;
  }

  arWildcatNamespace::fActive = true;
  if (!arWildcatNamespace::__wglEnableFrameLockI3D || !arWildcatNamespace::fUseFramelock)
    return;
  
  if (arWildcatNamespace::__wglEnableFrameLockI3D() == FALSE) {
    ar_log_warning() << "wglEnableFrameLockI3D failed.\n";
  }
  else{
    ar_log_debug() << "wglEnableFrameLockI3D ok.\n";
  }
}

void ar_deactivateWildcatFramelock() {
  arGuard dummy(arWildcatNamespace::lock);
  if (!arWildcatNamespace::fActive){
    ar_log_warning() << "Ignoring duplicate ar_deactivateWildcatFramelock.\n";
    return;
  }

  arWildcatNamespace::fActive = false;
  if (!arWildcatNamespace::__wglDisableFrameLockI3D || !arWildcatNamespace::fUseFramelock)
    return;

  if (arWildcatNamespace::__wglDisableFrameLockI3D() == FALSE) {
    ar_log_warning() << "wglDisableFrameLockI3D failed.\n";
  }
  else{
    ar_log_debug() << "wglDisableFrameLockI3D ok.\n";
  }
}

#else

void ar_findWildcatFramelock() {}
void ar_activateWildcatFramelock() {}
void ar_deactivateWildcatFramelock() {}

#endif
