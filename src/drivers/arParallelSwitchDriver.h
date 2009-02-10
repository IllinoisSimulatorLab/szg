//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

/*
Cross-platform serial-port-as-switch driver.
Switch shorts pin 2 to pin 3, making the port a loopback device
(whatever you write to it is what you read).
*/

#ifndef AR_ParallelSwitch_DRIVER_H
#define AR_ParallelSwitch_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

void ar_ParallelSwitchDriverEventTask(void*);

SZG_CALL enum arParallelSwitchEventType {
    AR_NO_PARA_SWITCH_EVENT = 0,
    AR_OPEN_PARA_SWITCH_EVENT = 1,
    AR_CLOSED_PARA_SWITCH_EVENT = 2,
    AR_BOTH_PARA_SWITCH_EVENT = 3
  };

class arParallelSwitchDriver: public arInputSource {
  friend void ar_ParallelSwitchDriverEventTask(void*);
 public:
  arParallelSwitchDriver();
  ~arParallelSwitchDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();

 private:
  bool _poll();
  void _eventloop();
  
  arThread _eventThread;
  unsigned       _comPortID;
  bool           _eventThreadRunning;
  bool           _stopped;
  unsigned char _byteMask;
  unsigned char _lastValue;
  ar_timeval     _lastEventTime;
  arParallelSwitchEventType _eventType;
  ar_timeval     _lastReportTime;
  unsigned       _reportCount;
};

#endif
