//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MOTIONSTAR_DRIVER_H
#define AR_MOTIONSTAR_DRIVER_H

#include "arInputSource.h"
#include "birdnet.h"

#include "arDriversCalling.h"

struct BN_PACKET {
  BN_HEADER header;
  unsigned char data[2048]; // undifferentiated packet data
};

// Driver for Ascension MotionStar motion tracker.

class arMotionstarDriver: public arInputSource {
  friend void ar_motionstarDriverEventTask(void*);
 public:
  arMotionstarDriver();
  ~arMotionstarDriver() {}

  bool init(arSZGClient&);
  bool start();
  bool stop();

 private:
  arThread _eventThread;
  string _birdnetIP;
  float _posScale;
  float _angScale;
  arSocket _commandSocket;
  float _receivedData[7];
  // space in which we can receive a response
  BN_PACKET _response;
  // where we are in the sequence of commands sent to the motionstar
  short _sequence;
  // TCP connection port on the Motionstar
  int _TCP_PORT;

  // Hardware DC filtering parameters
  // two parameters, alpha and Vm.  Values range from 0-32767
  // alpha = 0 implies infinite filtering, 32767 = none
  // Vm is a noise threshold, used to decide whether a change in signal reflects
  // motion or noise. 0 means change always interpreted as motion, 32767 means always
  // interpreted as noise.
  // Alpha represented by two parameter arrays, alphaMin and alphMax.  alphaMin
  // is the steady-state value.  If motion is detected (based on change in signal
  // relative to Vm), alpha is increased to a value not to exceed alphaMax.
  // 7 entries for each parameter represent values for different distance bands from
  // transmitter. With ERC, distance bands (inches) are: 0-55, 55-70, 70-90, 90-110,
  // 110-138, 138-170, 170+
  // Original Cube values were approximately:
  // alphaMin: 655 * 4, 163 * 3
  // alphaMax: 29490 * 7
  // Vm: 2, 4, 8, 32, 64, 256, 512
  unsigned short  _alphaMin[7]; // normal value, when no motion is detected
  unsigned short _alphaMax[7]; // Max value to raise alpha to when motion is detected
  unsigned short _Vm[7];
  bool _setAlphaMin, _setAlphaMax, _setVm;

  // HACK: remap birds in the absence of the general solution
  int _numberRemaps;

  bool _useButton;
  int _lastButtonValue;

  // high-level communication with the motionstar
  bool _sendWakeup();
  bool _getStatusAll(BN_SYSTEM_STATUS**);
  bool _setStatusAll(BN_SYSTEM_STATUS*);
  bool _getStatusBird(int, BN_BIRD_STATUS**);
  bool _setStatusBird(int, BN_BIRD_STATUS*);
  // low-level methods used to communicate with the motionstar
  bool _sendStatus(int, char*, int);
  bool _sendCommand(int, int);
  bool _getResponse(int);
  // low-level routines that parses data from the network
  void _parseData(BN_PACKET*);
  void _generateEvent(int);
  void _generateButtonEvent(int);
};

#endif
