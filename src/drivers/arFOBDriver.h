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

#ifndef AR_FOB_RS232_DRIVER_H
#define AR_FOB_RS232_DRIVER_H

#include "arInputSource.h"
#include "arRS232Port.h"

#include "arDriversCalling.h"

// Driver for Ascension's Flock of Birds magnetic motion tracker.

void ar_FOBDriverEventTask(void*);

class arFOBDriver: public arInputSource {
  friend void ar_FOBDriverEventTask(void*);
 public:
  arFOBDriver();
  ~arFOBDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();

 private:
  bool _setHemisphere( const std::string& hemisphere,
                       unsigned char addr = 0);
  bool _getHemisphere( std::string& hemisphere,
                       unsigned char addr = 0);
  bool _setDataMode( unsigned char addr = 0);
  bool _getDataMode( std::string& modeString,
                     unsigned char addr = 0 );
  bool _setPositionScale( bool longRange, unsigned char addr = 0 );
  bool _getPositionScale( bool& longRange, unsigned char addr = 0 );
  bool _autoConfig();
#ifdef UNUSED
  bool _nextTransmitter(const unsigned char addr);
#endif
  bool _sleep();
  bool _run();
  int _getFOBParam( const unsigned char paramNum,
                    unsigned char* buf,
                    const unsigned int numBytes,
                    unsigned char addr = 0 );
  bool _setFOBParam( const unsigned char paramNum,
                     const unsigned char* buf,
                     const unsigned int numBytes,
                     unsigned char addr = 0 );
  bool _sendBirdAddress( const unsigned char );
  bool _sendBirdCommand( const unsigned char* cdata,
                         const unsigned int numBytes );
  bool _sendBirdByte(unsigned char c, bool fSleep = true);
  int _getBirdData( unsigned char* cdata,
                    const unsigned int numBytes );
  bool _getSendNextFrame(const unsigned char addr);
  void _eventloop();
  bool _isBird(const unsigned addr) const
    { return _sensorMap[addr] >= 0; }

  arThread _eventThread;
  const unsigned _timeoutTenths;
  arRS232Port    _comPort;
  unsigned       _comPortID;
  int            _numFlockUnits;
  unsigned short _numBirds;
  const unsigned _dataSize;
  unsigned char* _dataBuffer;
  float          _positionScale;
  const float    _orientScale;
  bool           _eventThreadRunning;
  bool           _stopped;
  bool           _extendedRange;
  bool           _fStandalone; // exactly 1 unit, as opposed to "flock mode", more than 1 unit.

  enum {
    _FOB_MAX_DEVICES = 14,
    _nBaudRates = 7,
    _nHemi = 6
  };
  float _floatData[16];
  int   _sensorMap[_FOB_MAX_DEVICES+1];
};

#endif
