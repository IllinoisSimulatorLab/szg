//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FOB_DRIVER_H
#define AR_FOB_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"

#ifdef AR_USE_WIN_32
#include <wtypes.h>
#ifdef EnableBirdWinDriver
// Ascension header file.  We also need to link with Bird.lib
#include "Bird.h"       
#endif
#endif

/// Windows driver for Ascension Flock of Birds.

class arBirdWinDriver: public arInputSource {
  friend void ar_WinBirdDriverEventTask(void*);
 public:
  arBirdWinDriver();
  ~arBirdWinDriver() {}

  bool init(arSZGClient&);
  bool start();
  bool stop();
  bool restart();

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
#ifdef EnableBirdWinDriver
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
