//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LOGITECH_DRIVER_H
#define AR_LOGITECH_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arRS232Port.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

namespace arLogitechDriverSpace {
const unsigned int ELEMENT_SIZE = 16;
const unsigned int MAX_ELEMENTS = 5;
const unsigned int MAX_DATA = ELEMENT_SIZE*MAX_ELEMENTS;
};

class arLogitechDriver: public arInputSource {
  friend void ar_LogitechDriverEventTask(void*);
 public:
  arLogitechDriver();
  ~arLogitechDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();
  bool restart();

 private:
  bool _reset();
  bool _startStreaming();
  bool _runDiagnostics();
  bool _update();
  void _convertSendData( char* record );
  bool _woken;
  bool _stopped;
  arRS232Port    _comPort;
  char _dataBuffer[arLogitechDriverSpace::MAX_DATA];
  unsigned int _charsInBuffer;
  arThread _eventThread;
  bool _eventThreadRunning;
};

#endif
