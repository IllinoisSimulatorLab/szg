//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// Use a mouse as an input device.
// An old webpage used as the inspiration for the DirectX calls in this code:
// http://sunlightd.virtualave.net/Windows/DirectX/DirectX7/Input.html

#ifdef AR_USE_LINUX
#include <fcntl.h>
#include <stdio.h> // for perror()
#include <errno.h> // for perror()
#endif

#ifdef AR_USE_WIN_32
// DirectInput
#include <dinput.h>
#include <mmsystem.h>
IDirectInput* pDI = NULL;
IDirectInputDevice2* pMouse = NULL;
#endif

#include "arThread.h"
#include "InputServer.h"

#ifdef AR_USE_WIN_32
static BOOL CALLBACK DIDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  *(DIDEVICEINSTANCE *)pvRef = *lpddi;
  return (lpddi->dwDevType & DIDEVTYPEJOYSTICK_GAMEPAD) ?
    DIENUM_STOP : DIENUM_CONTINUE;
}
#endif

int main(int, char** argv)
{
  arSZGClient szgClient(1, "GamepadIntelServer");
  if (!szgClient)
    return 1;

  // set up the language and start sending data
  arInputServer inputServer(argv[0], szgClient, "SZG_MOUSE", "mouse",
                          "MouseServer", true);
  if (!inputServer)
    return 1;

  arThread dummy(ar_messageTask, &szgClient);

#if defined(AR_USE_LINUX)

  cerr << "linux is NYI.\n";

  while (true) {
    int number = 42;
    int value = 0; // 0 or 1
    inputServer.sendButton(number, value);
    ar_usleep(100000);
    }

#elif defined(AR_USE_WIN_32)

  // Initialize pMouse.
  DirectInputCreate(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &pDI, NULL);
  DIDEVICEINSTANCE dev;

  if (FAILED(pDI->CreateDevice(GUID_SysMouse, (IDirectInputDevice**)&pMouse, NULL)))
    { cerr << "DirectInput failure number 2.\n"; return 1; }
  if (!pMouse)
    return 1;
  if (FAILED(pMouse->SetDataFormat(&c_dfDIMouse)))
    { cerr << "DirectInput failure number 3.\n"; return 1; }
  pMouse->SetCooperativeLevel(NULL/*m_hWnd*/, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
  // Does the NULL in SetCooperativeLevel() work?  I thought it would have
  // needed to be an hwnd.
  if (FAILED(pMouse->Acquire()))
    cerr << "DirectInput failure number 7.\n";

  // If we consider mouse motions to be individual events (right 2 units, up 5 units)
  // then these must be reported as discrete events, i.e. button presses,
  // not axis values:  were they axis values, they might get read at a different rate
  // than they were generated, and errors would accumulate.
  // But button presses are integer (discrete),
  // while axes are intended for continuous data.  So we should report it as axis data.
  // But we can't report the delta (the mouse motion) directly then, as we saw;
  // so we report accumulated values instead.  Values hopefully stay within +-32K,
  // but there can be no guarantees on this.  Pawing a mouse forever to the right
  // will produce arbitrarily large values for the x axis, for instance.

  // Poll pMouse.
  while (true)
    {
    ar_usleep(14000); // 67 Hz, about
    static DIMOUSESTATE mouse = {0};
    static DIMOUSESTATE mousePrev = {0};
    int rc = pMouse->GetDeviceState(sizeof(mouse), &mouse);
    if (FAILED(rc))
      {
      cerr << "DirectInput GetDeviceState failed.\n";
      break;
      }

    if (memcmp(mouse.rgbButtons, mousePrev.rgbButtons, sizeof(mouse.rgbButtons)) == 0 &&
      mouse.lX == 0 && mouse.lY == 0 && mouse.lZ == 0)
      continue;

#if 0
    cout << "xyz " << mouse.lX << ", " << mouse.lY << ", " << mouse.lZ << endl;
    cout << "btns "
         << "-"
         << int(mouse.rgbButtons[0])
         << int(mouse.rgbButtons[1])
         << int(mouse.rgbButtons[2])
         << int(mouse.rgbButtons[3])
         << "-"
         << endl;
#endif

    for (int i=0; i<4; ++i)
      if (mouse.rgbButtons[i] != mousePrev.rgbButtons[i])
        inputServer.queueButton(i, mouse.rgbButtons[i] ? 1 : 0);

    static int axes[3] = {0};
    int axis = mouse.lX; // +-32K?
    if (axis != 0)
      inputServer.queueAxis(0, axes[0] += axis);
    axis = mouse.lY;
    if (axis != 0)
      inputServer.queueAxis(1, axes[1] += axis);
    axis = mouse.lZ;
    if (axis != 0)
      inputServer.queueAxis(2, axes[2] += axis);

    // Send all this accumulated data...
    inputServer.sendQueue();

    mousePrev = mouse;
    }
  // Do we ever clean up from DirectInput?
  // If we get here, it's because GetDeviceState failed.
  return 0;

#else

  cerr << argv[0] << " error: implemented only for linux and win32.\n";
  return 1;

#endif
}
