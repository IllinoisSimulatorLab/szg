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
// DirectInput... note that here we are forcing Direct Input 7.
// Why? In Direct Input 7, the device type is JOYSTICK_GAMEPAD,
// while in later Direct Inputs there are seperate JOYSTICK and
// gamepad types.
#define DIRECTINPUT_VERSION 0x700
#include <dinput.h>
#include <mmsystem.h>
static BOOL CALLBACK DIDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  *(DIDEVICEINSTANCE *)pvRef = *lpddi;
  // PLEASE NOTE THE FOLLOWING IMPORTANT FACT, this code will return the
  // first joystick to the arJoystickDriver. Unfortunately, in the case
  // of wireless intel joysticks and the win2k drivers, the first joystick
  // is basically fixed for all time by the base station... what if we want
  // to change it? these first few commented lines show how to do so in a
  // hack-ish way
  //if (!strcmp(lpddi->tszInstanceName,
  //            "Intel(r) Wireless Series Gamepad 2")){
  //  return DIENUM_STOP;
  //}
  //else{
  //  return DIENUM_CONTINUE;
  //}
  return (lpddi->dwDevType & DIDEVTYPEJOYSTICK_GAMEPAD) ?
    DIENUM_STOP : DIENUM_CONTINUE;
}
#endif
#endif

#endif
