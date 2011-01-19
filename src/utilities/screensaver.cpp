//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include <stdio.h>
#include <stdlib.h>
#include "arMasterSlaveFramework.h"

class ScreensaverFramework: public arMasterSlaveFramework {
  public:
    virtual void onWindowStartGL( arGUIWindowInfo* );
    virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
};


// Method to initialize each window (because now a Syzygy app can
// have more than one).
void ScreensaverFramework::onWindowStartGL( arGUIWindowInfo* ) {
//   OpenGL initialization
  glClearColor(0,0,0,0);
}


void ScreensaverFramework::onDraw( arGraphicsWindow& /*win*/, arViewport& /*vp*/ ) {
}

int main(int argc, char** argv) {

  ScreensaverFramework framework;

  if (!framework.init(argc, argv)) {
    return 1;
  }

  // Never returns unless something goes wrong
  return framework.start() ? 0 : 1;
}
