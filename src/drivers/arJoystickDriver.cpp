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
  ((arJoystickDriver*)joystickDriver)->_eventTask();
}

void arJoystickDriver::_eventTask() {
#ifdef AR_USE_LINUX
  js_event js;
  while (!_shutdown) {
    // NOTE: THERE IS A SHUTDOWN BUG IN HERE! Specifically, we are relying
    // on the device producing an event to get us out of this loop!
    // Consequently, the linux arJoystickDriver DOES NOT shut down in an 
    // orderly fashion.
    cout << "yoyoyo reading joystick\n";;;;
    read(_fd, &js, sizeof(js_event));
    switch (js.type & ~JS_EVENT_INIT){
    case JS_EVENT_BUTTON:
      cout << "yoyoyo read joystick button\n";;;;
      sendButton(js.number, js.value);
      // js.value is 0 or 1, experimentally determined
      break;
    case JS_EVENT_AXIS:
      cout << "yoyoyo read joystick axis\n";;;;
      sendAxis(js.number, js.value);
      // js.value is +-32k, experimentally determined
      break;
    }
  }
#endif

#ifdef AR_USE_WIN_32
// Poll pStick.
  while (!_shutdown){
    ar_usleep(10000); // 100 Hz
    if (FAILED(_pStick->Poll())){
      cerr << "DirectInput poll failed.\n";
      break;
    }
    static DIJOYSTATE2 jsPrev = {0};
    static bool fFirst = true;
    DIJOYSTATE2	js;
    int rc = _pStick->GetDeviceState(sizeof(js), &js);
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
      queueAxis(0, js.lX);
    if (fFirst || js.lY != jsPrev.lY)
      queueAxis(1, js.lY);
    if (fFirst || js.lZ != jsPrev.lZ)
      queueAxis(2, js.lZ);

    if (fFirst || js.lRx != jsPrev.lRx)
      queueAxis(3, js.lRx);
    if (fFirst || js.lRy != jsPrev.lRy)
      queueAxis(4, js.lRy);
    if (fFirst || js.lRz != jsPrev.lRz)
      queueAxis(5, js.lRz);

    if (fFirst || js.rglSlider[0] != jsPrev.rglSlider[0])
      queueAxis(6, js.rglSlider[0]);
    if (fFirst || js.rglSlider[1] != jsPrev.rglSlider[1])
      queueAxis(7, js.rglSlider[1]);

    for (int j=0; j<128; j++)
      if (fFirst || js.rgbButtons[j] != jsPrev.rgbButtons[j])
	queueButton(j, (js.rgbButtons[j] == 0) ? 0 : 1);

    fFirst = false;

    // rgdwPOV[4] is four 8-way hat switches.  What does Linux do with them?

    // Send all this accumulated data.
    sendQueue();

    jsPrev = js;
  }
  // Clean up.
  _pStick->Unacquire();
#endif

  // Thread ends.  Signal stop.
  ar_mutex_lock(&_shutdownLock);
  _pollingDone = true;
  _shutdownVar.signal();
  ar_mutex_unlock(&_shutdownLock);
}

arJoystickDriver::arJoystickDriver() :
  _pollingDone(false),
  _shutdown(false)
{
  ar_mutex_init(&_shutdownLock);
}

arJoystickDriver::~arJoystickDriver(){
}

bool arJoystickDriver::init(arSZGClient& szgClient){
  // Many gamepads have 6 axes and 10 buttons in 2006.  Default generously.
  int sig[3] = {10,6,0};
  if (!szgClient.getAttributeInts("SZG_JOYSTICK","signature",sig,3)) {
    sig[0] = 10;
    sig[1] = 6;
    sig[2] = 0;
    ar_log_warning() << "arJoystickDriver: "
      << szgClient.getComputerName() << "/SZG_JOYSTICK/signature defaulting to ("
      << sig[0] << ", " << sig[1] << ", " << sig[2] << ").\n";
  } else {
    ar_log_remark() << "arJoystickDriver: "
      << szgClient.getComputerName() << "/SZG_JOYSTICK/signature is ("
      << sig[0] << ", " << sig[1] << ", " << sig[2] << ").\n";
  }
  _setDeviceElements(sig[0],sig[1],sig[2]);

#ifdef AR_USE_LINUX
  _fd = open("/dev/js0", O_RDONLY);
  if (_fd < 0){
    _fd = open("/dev/input/js0", O_RDONLY);
    if (_fd < 0){
      ar_log_warning() << "arJoystickDriver failed to open /dev/js0 or /dev/input/js0\n";
      // bug: should keep retrying js1... like kam3:wandcassatt/cavewand.c does.
      return false;
    }
  }
#endif
  cout << "yoyoyo camilleg kam3 arJoystickDriver 2\n";;;;

#ifdef AR_USE_WIN_32
  // Initialize pStick.
  IDirectInput* pDI = NULL;
  _pStick = NULL;
  DirectInputCreate(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &pDI, NULL);
  DIDEVICEINSTANCE dev;
  if (FAILED(pDI->EnumDevices(DIDEVTYPE_JOYSTICK, DIDevCallback, &dev,  
      DIEDFL_ATTACHEDONLY))) {
    ar_log_warning() << "arJoystickDriver DirectInput failure number 1.\n";
    return false;
  }

  ar_log_debug() << "arJoystickDriver DirectInput: instance '"
    << dev.tszInstanceName << "', product '" << dev.tszProductName << "'.\n";
  // e.g. "Joystick 2" and "Microsoft SideWinder Freestyle Pro (USB)" respectively.

  if (FAILED(pDI->CreateDevice(dev.guidInstance, 
                               (IDirectInputDevice**)&_pStick, NULL))){ 
    ar_log_warning() << "arJoystickDriver DirectInput failure number 2.\n";
    return false; 
  }
  if (!_pStick)
    return false;
  if (FAILED(_pStick->SetDataFormat(&c_dfDIJoystick2))){ 
    ar_log_warning() << "arJoystickDriver DirectInput failure number 3.\n";
    return false; 
  }
  _pStick->SetCooperativeLevel(NULL, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);

  // set range of axes to +-32k
  DIPROPRANGE	range;
  range.diph.dwSize = sizeof(DIPROPRANGE);
  range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  range.diph.dwHow = DIPH_BYOFFSET;
  range.diph.dwObj = DIJOFS_X;
  range.lMax = 32767;
  range.lMin = -32768;
  if (FAILED(_pStick->SetProperty(DIPROP_RANGE, &range.diph))){
    ar_log_warning() << "arJoystickDriver DirectInput failure number 4.\n";
    return false;
  }
  range.diph.dwObj = DIJOFS_Y;
  if (FAILED(_pStick->SetProperty(DIPROP_RANGE, &range.diph))){
    ar_log_warning() << "arJoystickDriver DirectInput failure number 5.\n";
    return false;
  }
  DIPROPDWORD	dw;
  dw.dwData = DIPROPAXISMODE_ABS;
  dw.diph.dwSize = sizeof(DIPROPDWORD);
  dw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dw.diph.dwHow = DIPH_DEVICE;
  dw.diph.dwObj = 0;
  if (FAILED(_pStick->SetProperty(DIPROP_AXISMODE, &dw.diph))){
    ar_log_warning() << "arJoystickDriver DirectInput failure number 6.\n";
    return false;
  }
  if (FAILED(_pStick->Acquire())){
    ar_log_warning() << "arJoystickDriver DirectInput failure number 7.\n";
    return false;
  }
#endif
  cout << "yoyoyo camilleg kam3 arJoystickDriver 3\n";;;;
  ar_log_debug() << "arJoystickDriver initialized.\n";
  return true;
}

bool arJoystickDriver::start(){
  cout << "yoyoyo camilleg kam3 arJoystickDriver 4\n";;;;
  return _eventThread.beginThread(ar_joystickDriverEventTask,this);
}

bool arJoystickDriver::stop(){
  // Windows probably needs a clean shutdown
  _shutdown = true;
  ar_mutex_lock(&_shutdownLock);
  // Wait for thread to end
  while (!_pollingDone){
    _shutdownVar.wait(&_shutdownLock);
  }
  ar_mutex_unlock(&_shutdownLock);
  ar_log_debug() << "arJoystickDriver polling thread done.\n";
  return true;
}
