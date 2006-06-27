//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arJoystickDriver.h"

extern "C"{
  SZG_CALL void* factory()
    { return new arJoystickDriver(); } 
  SZG_CALL void baseType(char* buffer, int size) 
    { ar_stringToBuffer("arInputSource", buffer, size); }
}

void ar_joystickDriverEventTask(void* joystickDriver){
  arJoystickDriver* joy = (arJoystickDriver*) joystickDriver;
 
#ifdef AR_USE_LINUX
  js_event js;
  while (!joy->_shutdown) {
    // NOTE: THERE IS A SHUTDOWN BUG IN HERE! Specifically, we are relying
    // on the device producing an event to get us out of this loop!
    // Consequently, the linux arJoystickDriver DOES NOT shut down in an 
    // orderly fashion.
    read(joy->_fd, &js, sizeof(js_event));
    switch (js.type & ~JS_EVENT_INIT){
    case JS_EVENT_BUTTON:
      joy->sendButton(js.number, js.value);
      // js.value is 0 or 1, experimentally determined
      break;
    case JS_EVENT_AXIS:
      joy->sendAxis(js.number, js.value);
      // js.value is -32k to 32k, experimentally determined
      break;
    }
  }
#endif

#ifdef AR_USE_WIN_32
// Poll pStick.
  while (!joy->_shutdown){
    ar_usleep(10000); // 100 Hz
    if (FAILED(joy->_pStick->Poll())){
      cerr << "DirectInput poll failed.\n";
      break;
    }
    static DIJOYSTATE2 jsPrev = {0};
    static bool fFirst = true;
    DIJOYSTATE2	js;
    int rc = joy->_pStick->GetDeviceState(sizeof(js), &js);
    if (FAILED(rc)){
      cerr << "DirectInput GetDeviceState failed.\n";
      break;
    }
    // js.lX js.lY are main axis of joystick.
    // js.lZ is third axis, typically throttle.
    // js.lR[xyz] are rotation axes.
    // js.rglSlider[2] might be rotation axes as well, with some joysticks.
    // js.rgdwPOV[0] is 8-way hat-switch, 0 to 36000 in hundredths of a degree.
    // Implement hatswitch later as a single button, 
    // which has one of nine values...
    // js.rgbButtons[0..9] are the switches, 0==off, 128==on.  
    // (We convert to 0 or 1.)

    if (!fFirst && memcmp(&js, &jsPrev, sizeof(js)) == 0)
      continue;

    if (fFirst || js.lX != jsPrev.lX)
      joy->queueAxis(0, js.lX);
    if (fFirst || js.lY != jsPrev.lY)
      joy->queueAxis(1, js.lY);
    if (fFirst || js.lZ != jsPrev.lZ)
      joy->queueAxis(2, js.lZ);

    if (fFirst || js.lRx != jsPrev.lRx)
      joy->queueAxis(3, js.lRx);
    if (fFirst || js.lRy != jsPrev.lRy)
      joy->queueAxis(4, js.lRy);
    if (fFirst || js.lRz != jsPrev.lRz)
      joy->queueAxis(5, js.lRz);

    if (fFirst || js.rglSlider[0] != jsPrev.rglSlider[0])
      joy->queueAxis(6, js.rglSlider[0]);
    if (fFirst || js.rglSlider[1] != jsPrev.rglSlider[1])
      joy->queueAxis(7, js.rglSlider[1]);

    for (int j=0; j<128; j++)
      if (fFirst || js.rgbButtons[j] != jsPrev.rgbButtons[j])
	joy->queueButton(j, (js.rgbButtons[j] == 0) ? 0 : 1);

    fFirst = false;

    // rgdwPOV[4] is four 8-way hat switches.  What does Linux do with them?

    // Send all this accumulated data...
    joy->sendQueue();

    jsPrev = js;
  }
  // critical to do some clean-up
  joy->_pStick->Unacquire();
#endif

  // we're at the end of the thread... signal stop that we are done
  ar_mutex_lock(&joy->_shutdownLock);
  joy->_pollingDone = true;
  joy->_shutdownVar.signal();
  ar_mutex_unlock(&joy->_shutdownLock);
}

arJoystickDriver::arJoystickDriver(){
  ar_mutex_init(&_shutdownLock);
  _shutdown = false;
  _pollingDone = false;
}

arJoystickDriver::~arJoystickDriver(){
}

bool arJoystickDriver::init(arSZGClient& client){
  stringstream& initResponse = client.initResponse();

  // Most gamepads have 6 axes and 10 buttons in 2006.  Default generously.
  int sig[3] = {10,6,0};
  if (!client.getAttributeInts("SZG_JOYSTICK","signature",sig,3)) {
    initResponse << "arJoystickDriver remark: SZG_JOYSTICK/signature not set, "
                 << "defaulting to ( " << sig[0] << ", " 
                 << sig[1] << ", " << sig[2] << " ).\n";
    _setDeviceElements(10,6,0);
  } else {
    initResponse << "arJoystickDriver remark: SZG_JOYSTICK/signature set to "
                 << " ( " << sig[0] << ", " << sig[1] << ", " 
                 << sig[2] << " ).\n";
    _setDeviceElements(sig[0],sig[1],sig[2]);
  }

#ifdef AR_USE_LINUX
  _fd = open("/dev/js0", O_RDONLY);
  if (_fd < 0){
    initResponse << "arJoystickDriver error: failed to open /dev/js0\n";
    return false;
  }
#endif

#ifdef AR_USE_WIN_32
// Initialize pStick.
  IDirectInput* pDI = NULL;
  _pStick = NULL;
  DirectInputCreate(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &pDI, NULL);
  DIDEVICEINSTANCE dev;
  if (FAILED(pDI->EnumDevices(DIDEVTYPE_JOYSTICK, DIDevCallback, &dev,  
      DIEDFL_ATTACHEDONLY)))
    { initResponse << "DirectInput failure number 1.\n"; return false; }

  // LEAVE THESE COMMENTS IN SINCE THEY SHOW HOW TO USE DIRECT INPUT
  //cerr << "Instance is <" << dev.tszInstanceName << ">.\n";
  //cerr << "Product is <" << dev.tszProductName << ">.\n";
  // "Joystick 2" and "Microsoft SideWinder Freestyle Pro (USB)" respectively, for that device.

  if (FAILED(pDI->CreateDevice(dev.guidInstance, 
                               (IDirectInputDevice**)&_pStick, NULL))){ 
    initResponse << "DirectInput failure number 2.\n"; 
    return false; 
  }
  if (!_pStick)
    return false;
  if (FAILED(_pStick->SetDataFormat(&c_dfDIJoystick2))){ 
    initResponse << "DirectInput failure number 3.\n"; 
    return false; 
  }
  _pStick->SetCooperativeLevel(NULL/*m_hWnd*/, 
                              DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
  // The NULL in SetCooperativeLevel() works!  I thought it would have
  // needed to be an hwnd.  Phew.
  // set range of axes to +-32k
  DIPROPRANGE	range;
  range.diph.dwSize = sizeof(DIPROPRANGE);
  range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  range.diph.dwHow = DIPH_BYOFFSET;
  range.diph.dwObj = DIJOFS_X;
  range.lMax = 32767;
  range.lMin = -32768;
  if (FAILED(_pStick->SetProperty(DIPROP_RANGE, &range.diph))){
    initResponse << "DirectInput failure number 4.\n";
    return false;
  }
  range.diph.dwObj = DIJOFS_Y;
  if (FAILED(_pStick->SetProperty(DIPROP_RANGE, &range.diph))){
    initResponse << "DirectInput failure number 5.\n";
    return false;
  }
  DIPROPDWORD	dw;
  dw.dwData = DIPROPAXISMODE_ABS;
  dw.diph.dwSize = sizeof(DIPROPDWORD);
  dw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dw.diph.dwHow = DIPH_DEVICE;
  dw.diph.dwObj = 0;
  if (FAILED(_pStick->SetProperty(DIPROP_AXISMODE, &dw.diph))){
    initResponse << "DirectInput failure number 6.\n";
    return false;
  }
  if (FAILED(_pStick->Acquire())){
    initResponse << "DirectInput failure number 7.\n";
    return false;
  }
#endif
  initResponse << "arJoystickDriver remark: initialized.\n";
  return true;
}

bool arJoystickDriver::start(){
  return _eventThread.beginThread(ar_joystickDriverEventTask,this);
}

bool arJoystickDriver::stop(){
  // on Windows, we probably need a reasonably clean shut-down
  _shutdown = true;
  ar_mutex_lock(&_shutdownLock);
  // waiting for the thread to end
  while (!_pollingDone){
    _shutdownVar.wait(&_shutdownLock);
  }
  ar_mutex_unlock(&_shutdownLock);
  // cout << "arJoystickDriver remark: polling thread done.\n";
  return true;
}
