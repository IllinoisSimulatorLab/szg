//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_HEADERS
#define AR_INPUT_HEADERS

#ifdef AR_USE_LINUX
#include <linux/joystick.h>
#include <fcntl.h>
#include <stdio.h> // for perror()
#include <errno.h> // for perror()
#endif

#ifdef AR_USE_WIN_32
#ifndef AR_USE_MINGW
// Force DirectInput 7, so the device type is JOYSTICK_GAMEPAD.
// Later DirectInputs have seperate JOYSTICK and gamepad types, yuck.
#define DIRECTINPUT_VERSION 0x700
#include <dinput.h>
#include <mmsystem.h>
static BOOL CALLBACK DIDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  // Return the FIRST joystick to the arJoystickDriver.
  *(DIDEVICEINSTANCE *)pvRef = *lpddi;

#if 0
  // With wireless intel gamepads and the win2k drivers, the first joystick
  // is set by the base station.  Here's a hack to change it if we need to:
  if (!strcmp(lpddi->tszInstanceName, "Intel(r) Wireless Series Gamepad 2"))
    return DIENUM_STOP;
  else
    return DIENUM_CONTINUE;
#endif

  return (lpddi->dwDevType & DIDEVTYPEJOYSTICK_GAMEPAD) ?
    DIENUM_STOP : DIENUM_CONTINUE;
}
#endif
#endif

#endif
