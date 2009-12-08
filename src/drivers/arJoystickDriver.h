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

#ifdef AR_USE_WIN_32

#define MAX_JOYSTICKS        16
#define MAX_AXES        6        // per joystick
#define MAX_BUTTONS        32        // ditto

struct JoystickInfo {
        unsigned index;
        int numAxes;
        int axes[MAX_AXES];
  int axisOffsets[MAX_AXES];
  float axisScales[MAX_AXES];
        int numButtons;
        unsigned int buttons[MAX_BUTTONS];
  UINT systemID;
  JOYCAPS capabilities;
};
#endif

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
  arLock _shutdownLock; // with _shutdownVar
  arConditionVar _shutdownVar;
#ifdef AR_USE_LINUX
  int _fd;
#endif

#ifdef AR_USE_WIN_32
  void _printMMError( const string funcName, int errCode );
  int _numJoysticks;
  struct JoystickInfo _joysticks[MAX_JOYSTICKS];
#endif
};

#endif
