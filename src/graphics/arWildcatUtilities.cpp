//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "arWildcatUtilities.h"
#include "arDataUtilities.h"
#include "arThread.h"

namespace arWildcatNamespace {
  bool __inited = false;
  arMutex __wildcatMutex;
  bool __useWildcatFramelock = false;
  bool __frameLockInitted = false;
#ifdef AR_USE_WIN_32
  // AARGH! a hack here for the Wildcat cards! note that glFinish does
  // not work with them, apparently. Framelock needs to be on!
  typedef BOOL (APIENTRY *WGLENABLEFRAMELOCKI3DFUNCPTR)(VOID);
  typedef BOOL (APIENTRY *WGLDISABLEFRAMELOCKI3DFUNCPTR)(VOID);
  static BOOL (__stdcall *__wglEnableFrameLockI3D)(void) = 0;
  static BOOL (__stdcall *__wglDisableFrameLockI3D)(void) = 0;
#endif
};

using namespace std;
using namespace arWildcatNamespace;

void ar_useWildcatFramelock( bool isOn ) {
  if (__inited) {
    ar_mutex_lock( &__wildcatMutex );
    __useWildcatFramelock = isOn;
    ar_mutex_unlock( &__wildcatMutex );
  } else {
    ar_mutex_init( &__wildcatMutex );
    ar_mutex_lock( &__wildcatMutex );
    __inited = true;
    __useWildcatFramelock = isOn;
    ar_mutex_unlock( &__wildcatMutex );
  }
}

/// It is really annoying to have Wildcat-specific code in this file.
/// A little bit of a design to-do, I suppose.
void ar_findWildcatFramelock() {
#ifdef AR_USE_WIN_32
  if (!__inited){
    cerr << "arWildcatUtilities warning: calling ar_findWildcatFramelock "
	 << "\n  before calling ar_useWildcatFramelock(...). Disabling.\n";
    ar_useWildcatFramelock( false );
  }
  ar_mutex_lock( &__wildcatMutex );
  if (__useWildcatFramelock) {
    // need to try to find the Wildcat card's frame-locking functions here
      __wglEnableFrameLockI3D 
        = (WGLENABLEFRAMELOCKI3DFUNCPTR) 
            wglGetProcAddress("wglEnableFrameLockI3D");
      if (__wglEnableFrameLockI3D == 0) {
        cout << "wglEnableFrameLockI3D not found.\n";
      }
      __wglDisableFrameLockI3D = (WGLDISABLEFRAMELOCKI3DFUNCPTR)
        wglGetProcAddress("wglDisableFrameLockI3D");
      if (__wglDisableFrameLockI3D == 0) {
        cout << "wglDisableFrameLockI3D not found.\n";
      }
  }
  ar_mutex_unlock( &__wildcatMutex );
#endif
}

void ar_activateWildcatFramelock() {
#ifdef AR_USE_WIN_32
  if (!__inited){
    cerr << "arWildcatUtilities warning: calling ar_activateWildcatFramelock "
	 << "\n  before calling ar_useWildcatFramelock(...). Disabling.\n";
    ar_useWildcatFramelock( false );
  }
  ar_mutex_lock( &__wildcatMutex );
  if (!__frameLockInitted) {
    if (__wglEnableFrameLockI3D != 0 && __useWildcatFramelock) {
      if (__wglEnableFrameLockI3D() == FALSE) {
	cout << "\nwglEnableFrameLockI3D failed.\n";
      }
      else{
	cout << "\nwglEnableFrameLockI3D succeeded.\n";
      }
      // the next line is actually necessary for the wildcats...
      // frame-locking needs to be working when next we try to swap frames
      // there's a little bit of a lesson in here... syzygy is trying to
      // maintain frame-by-frame precision, while the wildcat cards seem to
      // be built to tolerate (or even require) some slop from frame to frame
      //ar_usleep(900000);
    }
    __frameLockInitted = true;
  }
  ar_mutex_unlock( &__wildcatMutex );
#endif
}

void ar_deactivateWildcatFramelock() {
#ifdef AR_USE_WIN_32
  if (!__inited){
    cerr << "arWildcatUtilities warning: calling "
	 << "ar_deactivateWildcatFramelock "
	 << "\n  before calling ar_useWildcatFramelock(...). Disabling.\n";
    ar_useWildcatFramelock( false );
  }
  ar_mutex_lock( &__wildcatMutex );
  if (__frameLockInitted){
    if (__wglDisableFrameLockI3D != 0 && __useWildcatFramelock) {
      if (__wglDisableFrameLockI3D() == FALSE) {
        cout << "\nwglDisableFrameLockI3D() failed.\n";
      }
      else{
        cout << "\nwglDisableFrameLockI3D() succeeded.\n";
      }
      // make sure this really took...
      //ar_usleep(900000);
      __frameLockInitted = false;
    }
  }
  ar_mutex_unlock( &__wildcatMutex );
#endif
}


