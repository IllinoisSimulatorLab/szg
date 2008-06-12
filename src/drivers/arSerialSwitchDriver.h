//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

/*
Cross-platform serial-port-as-switch driver.
Switch shorts pin 2 to pin 3, making the port a loopback device
(whatever you write to it is what you read).
*/

#ifndef AR_SerialSwitch_RS232_DRIVER_H
#define AR_SerialSwitch_RS232_DRIVER_H

#include "arInputSource.h"
#include "arRS232Port.h"

#include "arDriversCalling.h"

void ar_SerialSwitchDriverEventTask(void*);

SZG_CALL enum arSerialSwitchEventType {
    AR_NO_SWITCH_EVENT = 0,
    AR_OPEN_SWITCH_EVENT = 1,
    AR_CLOSED_SWITCH_EVENT = 2,
    AR_BOTH_SWITCH_EVENT = 3
  };

class arSerialSwitchDriver: public arInputSource {
  friend void ar_SerialSwitchDriverEventTask(void*);
 public:
  arSerialSwitchDriver();
  ~arSerialSwitchDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();

 private:
  bool _poll();
  void _eventloop();

  arThread _eventThread;
  const unsigned _timeoutTenths;
  arRS232Port    _comPort;
  unsigned       _comPortID;
  bool           _eventThreadRunning;
  bool           _stopped;
  char           _outChar;
  char           _inChar;
  arSerialSwitchEventType _lastState;
  ar_timeval     _lastEventTime;
  arSerialSwitchEventType _eventType;
};

#endif
