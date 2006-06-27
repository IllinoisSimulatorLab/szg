//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arIntelGamepadDriver.h"

extern "C"{
  SZG_CALL void* factory()
    { return new arIntelGamepadDriver(); } 
  SZG_CALL void baseType(char* buffer, int size)
    { ar_stringToBuffer("arInputSource", buffer, size); }
}

void ar_intelGamepadDriverEventTask(void* gamepadDriver){
#ifdef AR_USE_WIN_32
  arIntelGamepadDriver* g = (arIntelGamepadDriver*) gamepadDriver;

  // Poll pKeyboard.
  while (true){
    ar_usleep(14000); // 67 Hz, about
    static BYTE jsPrev[256] = {0};
    static BYTE js[256] = {0};
    int rc = g->_pKeyboard->GetDeviceState(sizeof(js), &js);
    if (FAILED(rc)){
      cerr << "DirectInput GetDeviceState failed.\n";
      break;
    }

    // Special D-pad behavior:
    // Ramp up the multiplier from 0 to 32k, over about half a second (60 iterations).
    // And ramp it back down, more slowly, when D-pad is released.
    static int m = 0;
    if (js[DIK_LEFT] || js[DIK_RIGHT] || js[DIK_UP] || js[DIK_DOWN]){
      m += 32767/60;
      if (m > 32767)
        m = 32767;
    }
    else{
      m -= 32767/120;
      if (m < 0)
        m = 0;
    }

    if (memcmp(&js, &jsPrev, sizeof(js)) == 0 &&
       !(js[DIK_LEFT] || js[DIK_RIGHT] || js[DIK_UP] || js[DIK_DOWN]))
       // special case for time-variant behavior while the D-pad is held down
      continue;

#if 0
    for (int i=0; i<256; ++i){
      if (js[i] != 0)
        cout << "[" << i << "] = " << int(js[i]) << endl;
    }
    cout << endl;
#endif

    const int keys[] = {
      // unshifted buttons
      DIK_A, DIK_B, DIK_C, DIK_X, DIK_Y, DIK_Z, DIK_L, DIK_R, 
      // shifted buttons
      DIK_D, DIK_E, DIK_F, DIK_U, DIK_V, DIK_W, DIK_G, DIK_H, 
      // "start" button, shifted and unshifted
      DIK_HOME, DIK_END
    };

    for (int iKey=0; iKey < sizeof(keys) / sizeof(int); ++iKey){
      const int key = keys[iKey];
      if (js[key] != jsPrev[key]){
        g->queueButton(iKey, js[key] ? 1 : 0);
      }
    }

    // D-pad
    static int axis0Prev = -1000000;
    static int axis1Prev = -1000000;
    int axis = m * (js[DIK_LEFT] ? -1 : js[DIK_RIGHT] ? 1 : 0);
    if (axis != axis0Prev){
      // KLUGE! Want to get values between -1 and 1!
      // this stuff really needs to be pushed into a filter of some sort!
      g->queueAxis(0, axis/32768.0);
    }
    axis0Prev = axis;
    axis = m * (js[DIK_UP] ? -1 : js[DIK_DOWN] ? 1 : 0);
    if (axis != axis1Prev){
      // KLUGE! Want to get values between -1 and 1! 
      // this stuff really needs to be pushed into a filter of some sort!
      g->queueAxis(1, -axis/32768.0);
    }
    axis1Prev = axis;

    // Send all this accumulated data...
    g->sendQueue();

    memcpy(jsPrev, js, sizeof(js));
  }
#endif
}

bool arIntelGamepadDriver::init(arSZGClient& client){
  int sig[3] = {8,2,0};
  if (!client.getAttributeInts("SZG_JOYSTICK","signature",sig,3)) {
    cerr << "arIntelGamepadDriver remark: SZG_JOYSTICK/signature not set, "
         << "defaulting to ( " << sig[0] << ", " << sig[1] << ", " << sig[2]
         << " ).\n";
    _setDeviceElements(8,2,0);
  } else {
    cerr << "arIntelGamepadDriver remark: SZG_JOYSTICK/signature set to "
         << " ( " << sig[0] << ", " << sig[1] << ", " << sig[2] << " ).\n";
    _setDeviceElements(sig[0],sig[1],sig[2]);
  }

#ifdef AR_USE_WIN_32
  IDirectInput* pDI = NULL;
  _pKeyboard = NULL; 
  DirectInputCreate(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &pDI, NULL);
  DIDEVICEINSTANCE dev;

  if (FAILED(pDI->CreateDevice(GUID_SysKeyboard, 
             (IDirectInputDevice**)&_pKeyboard, NULL))){ 
    cerr << "DirectInput failure number 2.\n"; 
    return false; 
  }
  if (!_pKeyboard){
    return false;
  }
  if (FAILED(_pKeyboard->SetDataFormat(&c_dfDIKeyboard))){ 
    cerr << "DirectInput failure number 3.\n"; 
    return false; 
  }
  // Does the NULL in SetCooperativeLevel() work?  I thought it would have
  // needed to be an hwnd.
  _pKeyboard->SetCooperativeLevel(NULL/*m_hWnd*/, 
                                  DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
  if (FAILED(_pKeyboard->Acquire())){
    cerr << "DirectInput failure number 7.\n";
    return false;
  }
#endif

  return true;
}

bool arIntelGamepadDriver::start(){
  return _eventThread.beginThread(ar_intelGamepadDriverEventTask,this);
}
