//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGenericDriver.h"

arGenericDriver::arGenericDriver() {
}

arGenericDriver::~arGenericDriver() {
}

void arGenericDriver::setSignature(unsigned buttons, unsigned axes, unsigned matrices) {
  _setDeviceElements(buttons, axes, matrices);
}
