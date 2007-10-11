//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arJoystickDriver.h"

#ifdef AR_USE_MINGW

#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
/* limit axis to 256 possible positions to filter out noise */
#define JOY_AXIS_THRESHOLD      (((AXIS_MAX)-(AXIS_MIN))/256)
#define JOY_BUTTON_FLAG(n)	(1<<n)

#endif


DriverFactory(arJoystickDriver, "arInputSource")

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
    read(_fd, &js, sizeof(js_event));
    switch (js.type & ~JS_EVENT_INIT){
    case JS_EVENT_BUTTON:
      sendButton(js.number, js.value);
      // js.value is 0 or 1, experimentally determined
      break;
    case JS_EVENT_AXIS:
      sendAxis(js.number, js.value);
      // js.value is +-32k, experimentally determined
      break;
    }
  }
#endif

#ifdef AR_USE_WIN_32
#ifdef AR_USE_MINGW
	MMRESULT result;
	int i,j;
	DWORD flags[MAX_AXES] = { JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, 
				  JOY_RETURNR, JOY_RETURNU, JOY_RETURNV };
	DWORD pos[MAX_AXES];
	int value, change;
	JOYINFOEX joyInfo;
  int axisNum;
  int buttonNum;

  while (!_shutdown) {
    ar_usleep(10000); // 100 Hz
    axisNum = 0;
    buttonNum = 0;
    for (j=0; j<_numJoysticks; ++j) {
      struct JoystickInfo *joystick = _joysticks+j;
      joyInfo.dwSize = sizeof(joyInfo);
      joyInfo.dwFlags = JOY_RETURNALL|JOY_RETURNPOVCTS;
      joyInfo.dwFlags &= ~(JOY_RETURNPOV|JOY_RETURNPOVCTS);
      result = joyGetPosEx( joystick->systemID, &joyInfo );
      if (result != JOYERR_NOERROR) {
        _printMMError( "joyGetPosEx", result );
        return;
      }
      pos[0] = joyInfo.dwXpos;
      pos[1] = joyInfo.dwYpos;
      pos[2] = joyInfo.dwZpos;
      pos[3] = joyInfo.dwRpos;
      pos[4] = joyInfo.dwUpos;
      pos[5] = joyInfo.dwVpos;

      for (i=0; i<joystick->numAxes; ++i) {
        if (joyInfo.dwFlags & flags[i]) {
          value = (int)(((float)pos[i] + joystick->axisOffsets[i]) * joystick->axisScales[i]);
          change = (value - joystick->axes[i]);
          if ( (change < -JOY_AXIS_THRESHOLD) || (change > JOY_AXIS_THRESHOLD) ) {
            joystick->axes[i] = value;
            queueAxis( axisNum, value );
          }
        }
        ++axisNum;
      }

      /* joystick button events */
      if (joyInfo.dwFlags & JOY_RETURNBUTTONS) {
        for (i=0; i<joystick->numButtons; ++i) {
          if (joyInfo.dwButtons & JOY_BUTTON_FLAG(i)) {
            if (!joystick->buttons[i]) {
              joystick->buttons[i] = 1;
              queueButton( buttonNum, 1 );
              ar_log_debug() << "Button " << buttonNum << " pressed.\n";
            }
          } else {
            if (joystick->buttons[i]) {
              joystick->buttons[i] = 0;
              queueButton( buttonNum, 0 );
              ar_log_debug() << "Button " << buttonNum << " released.\n";
            }
          }
          ++buttonNum;
        }
      }

      sendQueue();
    }
  }
  
#else
// Poll pStick.
  memset(&_jsPrev, 0, sizeof(_jsPrev));
  while (!_shutdown){
    ar_usleep(10000); // 100 Hz
    if (FAILED(_pStick->Poll())){
      cerr << "DirectInput poll failed.\n";
      break;
    }
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

    if (!_fFirst && memcmp(&js, &_jsPrev, sizeof(js)) == 0)
      continue;

    if (_fFirst || js.lX != _jsPrev.lX)
      queueAxis(0, js.lX);
    if (_fFirst || js.lY != _jsPrev.lY)
      queueAxis(1, js.lY);
    if (_fFirst || js.lZ != _jsPrev.lZ)
      queueAxis(2, js.lZ);

    if (_fFirst || js.lRx != _jsPrev.lRx)
      queueAxis(3, js.lRx);
    if (_fFirst || js.lRy != _jsPrev.lRy)
      queueAxis(4, js.lRy);
    if (_fFirst || js.lRz != _jsPrev.lRz)
      queueAxis(5, js.lRz);

    if (_fFirst || js.rglSlider[0] != _jsPrev.rglSlider[0])
      queueAxis(6, js.rglSlider[0]);
    if (_fFirst || js.rglSlider[1] != _jsPrev.rglSlider[1])
      queueAxis(7, js.rglSlider[1]);

    for (int j=0; j<128; j++) {
      if (_fFirst || js.rgbButtons[j] != _jsPrev.rgbButtons[j]) {
        queueButton(j, (js.rgbButtons[j] == 0) ? 0 : 1);
      }
    }

    _fFirst = false;

    // rgdwPOV[4] is four 8-way hat switches.  What does Linux do with them?

    // Send all this accumulated data.
    sendQueue();

    _jsPrev = js;
  }
  // Clean up.
  _pStick->Unacquire();
#endif
#endif

  // Thread ends.  Signal stop.
  arGuard dummy(_shutdownLock);
  _pollingDone = true;
  _shutdownVar.signal();
}

// Don't inline, lest factory break.
arJoystickDriver::arJoystickDriver() :
  _pollingDone(false),
#ifdef AR_USE_WIN_32
#ifdef AR_USE_MINGW
  _shutdown(false)
#else
  _shutdown(false),
  _fFirst(true)
#endif
#else
  _shutdown(false)
#endif
{
}


bool arJoystickDriver::init(arSZGClient& szgClient){
  // Many gamepads have 6 axes and 10 buttons in 2007.  Default generously.
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
  _setDeviceElements(sig);

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

#ifdef AR_USE_WIN_32
#ifdef AR_USE_MINGW
	int	i,j;
	int maxDevices;
	int numDevices;
	JOYINFOEX joyInfo;
	JOYCAPS	joyCaps;
	MMRESULT result;
	int fCapabilities[MAX_AXES-2] =
		{ JOYCAPS_HASZ, JOYCAPS_HASR, JOYCAPS_HASU, JOYCAPS_HASV };
	int axisMins[MAX_AXES], axisMaxes[MAX_AXES];


	for (i=0; i<MAX_JOYSTICKS; ++i) {
    _joysticks[i].systemID = 0;
	}

	/* Loop over all potential joystick devices */
	numDevices = 0;
	maxDevices = joyGetNumDevs();
	for (i=JOYSTICKID1; i<maxDevices && numDevices<MAX_JOYSTICKS; ++i) {
		joyInfo.dwSize = sizeof(joyInfo);
		joyInfo.dwFlags = JOY_RETURNALL;
		result = joyGetPosEx( _joysticks[i].systemID, &joyInfo );
		if (result == JOYERR_NOERROR) {
			result = joyGetDevCaps(i, &joyCaps, sizeof(joyCaps));
			if (result == JOYERR_NOERROR) {
        _joysticks[numDevices].systemID = i;
        _joysticks[numDevices].capabilities = joyCaps;
				numDevices++;
			}
		}
	}
  _numJoysticks = numDevices;
  ar_log_critical() << "Found " << _numJoysticks << " joysticks.\n";
  
  for (j=0; j<_numJoysticks; ++j) {
    struct JoystickInfo* joystick = _joysticks+j;
    joystick->index = j;
    JOYCAPS *capabilities = &(joystick->capabilities);
    axisMins[0] = capabilities->wXmin;
    axisMaxes[0] = capabilities->wXmax;
    axisMins[1] = capabilities->wYmin;
    axisMaxes[1] = capabilities->wYmax;
    axisMins[2] = capabilities->wZmin;
    axisMaxes[2] = capabilities->wZmax;
    axisMins[3] = capabilities->wRmin;
    axisMaxes[3] = capabilities->wRmax;
    axisMins[4] = capabilities->wUmin;
    axisMaxes[4] = capabilities->wUmax;
    axisMins[5] = capabilities->wVmin;
    axisMaxes[5] = capabilities->wVmax;

    for (i=0; i<MAX_AXES; ++i) {
      if ((i<2) || (capabilities->wCaps & fCapabilities[i-2])) {
        joystick->axisOffsets[i] = AXIS_MIN-axisMins[i];
        joystick->axisScales[i] = (float)(AXIS_MAX-AXIS_MIN)/(axisMaxes[i]-axisMins[i]);
      } else {
        joystick->axisOffsets[i] = 0;
        joystick->axisScales[i] = 1.;
      }
    }

    /* fill nbuttons, naxes, and nhats fields */
    joystick->numButtons = capabilities->wNumButtons;
    joystick->numAxes = capabilities->wNumAxes;

    ar_log_critical() << "Joystick #" << j << " has " << joystick->numButtons
                      << " buttons and " << joystick->numAxes << " axes.\n";
  }
  
#else
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
#endif
  ar_log_debug() << "arJoystickDriver inited.\n";
  return true;
}

bool arJoystickDriver::start(){
  return _eventThread.beginThread(ar_joystickDriverEventTask,this);
}

bool arJoystickDriver::stop(){
  // Windows probably needs a clean shutdown
  _shutdown = true;
  arGuard dummy(_shutdownLock);
  // Wait for thread to end
  while (!_pollingDone){
    _shutdownVar.wait(_shutdownLock);
  }
  ar_log_debug() << "arJoystickDriver polling thread done.\n";
  return true;
}

#ifdef AR_USE_MINGW
void arJoystickDriver::_printMMError( const string funcName, int errCode ) {
	string error;
	switch (errCode) {
		case MMSYSERR_NODRIVER:
			error = "Joystick driver not present";
      break;
		case MMSYSERR_INVALPARAM:
		case JOYERR_PARMS:
			error = "Invalid parameter(s)";
      break;
		case MMSYSERR_BADDEVICEID:
			error = "Bad device ID";
      break;
		case JOYERR_UNPLUGGED:
			error = "Joystick not attached";
      break;
		case JOYERR_NOCANDO:
			error = "Can't capture joystick input";
      break;
		default:
      ar_log_error() << funcName << ": Unknown Multimedia system error: 0x" << errCode << ar_endl;
      return;
	}
  ar_log_error() << funcName << ": " << error << ar_endl;
}
#endif
