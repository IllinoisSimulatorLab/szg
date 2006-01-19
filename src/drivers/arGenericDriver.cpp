//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGenericDriver.h"

arGenericDriver::arGenericDriver(){
  // does nothing yet!
}

arGenericDriver::~arGenericDriver(){
  // does nothing yet!
}

void arGenericDriver::setSignature(int buttons, int axes, int matrices){
  _setDeviceElements(buttons,axes,matrices);
}
