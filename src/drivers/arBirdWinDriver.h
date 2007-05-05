//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FOB_DRIVER_H
#define AR_FOB_DRIVER_H

#include "arInputSource.h"

#ifdef AR_USE_WIN_32
  #include <wtypes.h>
  #ifdef EnableBirdWinDriver
    #include "Bird.h" // Ascension's header file.  Link with Ascension's Bird.lib.
  #endif
#endif

#include "arDriversCalling.h"

// Windows driver for Ascension Flock of Birds.

class arBirdWinDriver: public arInputSource {
 public:
  arBirdWinDriver();
  ~arBirdWinDriver() {}

  bool init(arSZGClient&);
  bool start();
  bool stop();

#ifdef EnableBirdWinDriver
 private:
  arThread _eventThread;
  int _numDevices;
  bool _flockWoken;
  bool _streamingStarted;
  enum {
    _FOB_MAX_DEVICES = 31,
    _nBaudRates = 7,
    _nHemi = 6
  };

  friend void ar_WinBirdDriverEventTask(void*);
  int _groupID;
  BOOL _standAlone;
  WORD _comPorts[_FOB_MAX_DEVICES];
  DWORD _baudRate;
  DWORD _readTimeout;
  DWORD _writeTimeout;
  BIRDSYSTEMCONFIG _sysConfig;
  BIRDDEVICECONFIG _devConfig[_FOB_MAX_DEVICES];
  BYTE _hemisphereNum;
#endif
};

#endif
