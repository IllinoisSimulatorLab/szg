//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_5DT_GLOVE_DRIVER_H
#define AR_5DT_GLOVE_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

class ar5DTGloveDriver: public arInputSource {
 public:
  bool init(arSZGClient& client);
  bool start();
  bool stop();
  int _numSensors;
  int _numGestures;
  int _lastGesture;
 private:
  arThread _eventThread;
};

#endif
