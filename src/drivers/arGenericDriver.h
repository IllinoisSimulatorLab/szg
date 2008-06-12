//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GENERIC_DRIVER_H
#define AR_GENERIC_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

// Example to copy-paste from, for writing drivers for input devices.

// Used, too, by framework/arInputSimulator.cpp.

class SZG_CALL arGenericDriver: public arInputSource {
 public:
  arGenericDriver();
  virtual ~arGenericDriver();
  void setSignature(unsigned numButtons, unsigned numAxes, unsigned numMatrices);
};

#endif
