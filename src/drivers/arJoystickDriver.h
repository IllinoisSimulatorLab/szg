//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_JOYSTICK_DRIVER_H
#define AR_JOYSTICK_DRIVER_H

#include "arInputSource.h"
#include "arInputHeaders.h"

#include "arDriversCalling.h"

// Driver for (USB) joysticks or gamepads.

class SZG_CALL arJoystickDriver: public arInputSource{
  friend void ar_joystickDriverEventTask(void*);
 public:
  arJoystickDriver();

  bool init(arSZGClient& client);
  bool start();
  bool stop();
 private:
  arThread _eventThread;
  void _eventTask();
  bool _pollingDone;
  bool _shutdown;
  arMutex _shutdownLock; // with _shutdownVar
  arConditionVar _shutdownVar;
#ifdef AR_USE_LINUX
  int _fd;
#endif
  
#ifdef AR_USE_WIN_32
  IDirectInputDevice2* _pStick;
  DIJOYSTATE2 _jsPrev;
  bool _fFirst;
#endif
};

#endif
