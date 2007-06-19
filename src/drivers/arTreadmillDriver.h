//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

/*
Cross-platform Flock-of-Birds driver. Jim Crowell, 10/02.
Further modifications by Lee Hendrickson and Ben Schaeffer.
Thanks to Bill Sherman's FreeVR library for providing important
insight into the Flock's operation.

A Flock can be set up in several ways:
  - Standalone: a single unit hosts a bird and a transmitter via RS232.
  - Flock, 1 RS232: several units, but only one is connected to
    the host via RS232. The "master" unit connects to the host,
    and has flock ID 1. Other units slave to the master via
    the flock's own FBB interface. A transmitter will be connected to
    the flock, but maybe not to the master unit. Also, some units may
    have no birds, e.g. the unit with the transmitter.
  - Flock, multiple RS232. The host communicates to multiple
    units directly via multiple serial connections. Unimplemented.

Within Syzygy, serial port numbers are 1-based:
port 1 in Win32 is COM1, in Linux is /dev/ttys0.
Not true?  InputDevices-Drivers.t2t and drivers/RS232Server.cpp disagree.
*/

#ifndef AR_Treadmill_RS232_DRIVER_H
#define AR_Treadmill_RS232_DRIVER_H

#include "arInputSource.h"
#include "arRS232Port.h"

#include "arDriversCalling.h"

// Driver for Ascension's Flock of Birds magnetic motion tracker.

void ar_TreadmillDriverEventTask(void*);

class arTreadmillDriver: public arInputSource {
  friend void ar_TreadmillDriverEventTask(void*);
 public:
  arTreadmillDriver();
  ~arTreadmillDriver();

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
  char           _charBuf;
  bool            _lastState;
  ar_timeval     _lastEventTime;
};

#endif
