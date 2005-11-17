//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FARO_DRIVER_H
#define AR_FARO_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arMath.h"
#include "arRS232Port.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

/// Driver for the Faro Arm.

class arFaroDriver: public arInputSource {
  friend void ar_FaroDriverEventTask(void*);
 public:
  arFaroDriver();
  ~arFaroDriver();

  bool init(arSZGClient& SZGClient);
  bool start();
  bool stop();
  
 private:
  bool _getSendData();
  bool _inited;
  arThread _eventThread;
  bool _eventThreadRunning;
  bool _stopped;
  arRS232Port _port;
  arVector3 _probeDimensions;
  arVector3 _defaultProbeDimensions;
  arVector3 _probeDimDiffs;
  char _inbuf[4096];

};

#endif
