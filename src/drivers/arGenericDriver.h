//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GENERIC_DRIVER_H
#define AR_GENERIC_DRIVER_H

#include "arInputSource.h"

/// Example to copy-paste from, for writing drivers for input devices.

class SZG_CALL arGenericDriver: public arInputSource{
 public:
  arGenericDriver();
  ~arGenericDriver();
  
  void setSignature(int,int,int);
};

#endif
