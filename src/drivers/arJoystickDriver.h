//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_JOYSTICK_DRIVER_H
#define AR_JOYSTICK_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arInputHeaders.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

// Driver for (USB) joysticks or gamepads.

class SZG_CALL arJoystickDriver: public arInputSource{
  friend void ar_joystickDriverEventTask(void*);
 public:
  arJoystickDriver();
  ~arJoystickDriver();

  bool init(arSZGClient& client);
  bool start();
  bool stop();
 private:
  arThread _eventThread;
  bool _pollingDone;
  bool _shutdown;
  arMutex _shutdownLock;
  arConditionVar _shutdownVar;
#ifdef AR_USE_LINUX
  int _fd;
#endif
  
#ifdef AR_USE_WIN_32
  IDirectInputDevice2* _pStick;
#endif
};

#endif
