//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Cross-platform Flock-of-Birds driver. Jim Crowell, 10/02.
// Further modifications by Lee Hendrickson and Ben Schaeffer.
// Thanks to Bill Sherman's FreeVR library for providing important
// insight into the Flock's operation.
// 
// A Flock can be set up in several ways:
// - standalone: a single unit hosts a bird and a transmitter. This is
//   connected directly to the host via RS232.
// - flock, 1 RS232: There are several units, but only one is connected to
//   the host via RS232. The "master" unit is connected to the host
//   and has flock ID 1. The other units are slaved to the master via
//   the internal flock FBB interface. A transmitter will be connected to
//   the flock, but maybe not to the master unit. Also, some units may
//   not have birds attached (for instance the unit holding the transmitter).
// - flock, mulitple RS232. In this case, the host communicates to multiple
//   units directly via mulitple serial connections. THIS IS NOT SUPPORTED!
// 
// Serial port numbers are 1-based: under Win32 COM1 is port 1,
// and under Linux /dev/ttys0 is port 1.
// 

#ifndef AR_FOB_RS232_DRIVER_H
#define AR_FOB_RS232_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arRS232Port.h"
#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <string>

/// Driver for Ascension Flock of Birds.

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
  bool _nextTransmitter(unsigned char addr);
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
  bool _sendBirdAddress( unsigned char birdAddess );
  bool _sendBirdCommand( const unsigned char* cdata, 
                         const unsigned int numBytes );
  bool _sendBirdByte(unsigned char c, bool fSleep = true);
  int _getBirdData( unsigned char* cdata, 
                    const unsigned int numBytes );
  bool _getSendNextFrame(unsigned char addr);
  void _eventloop();
  
  arThread _eventThread;
  unsigned int   _timeoutTenths;
  arRS232Port    _comPort;
  unsigned int   _comPortID;
  int            _numFlockUnits;
  int            _transmitterID;
  unsigned short _numBirds;
  unsigned char  _dataSize;
  unsigned char* _dataBuffer;
  float          _positionScale;
  float          _orientScale;
  bool           _eventThreadRunning;
  bool           _stopped;
  bool           _extendedRange;
  enum {
    _FOB_MAX_DEVICES = 14,
    _nBaudRates = 7,
    _nHemi = 6
  };
  int         _sensorMap[_FOB_MAX_DEVICES+1];
  float       _floatData[16];
  
  // calibration parameters
  arMatrix4 _transmitterOffset;
  arMatrix4 _sensorRot[ _FOB_MAX_DEVICES ];
};

#endif

// this is from kam3:/home/szg/src/drivers
