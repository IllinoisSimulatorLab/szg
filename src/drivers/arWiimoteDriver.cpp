//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arWiimoteDriver.h"

DriverFactory(arWiimoteDriver, "arInputSource")

#ifdef AR_USE_WIN_32
  #define WIIUSE_PATH "wiiuse.dll"
#else
  #include <unistd.h>
  #define WIIUSE_PATH "./wiiuse.so"
#endif

const int WIIMOTE_ID_1    = 1;

const int BUTTONS_WIIMOTE = 11;
const int BUTTONS_NUNCHUK = 2;
const int AXES_WIIMOTE    = 0;
const int AXES_NUNCHUK    = 2;
const int MATRIX_WIIMOTE  = 1;
const int MATRIX_NUNCHUK  = 1;

static arWiimoteDriver* __wiimoteDriver = NULL;
static struct wiimote_t** __wiimotes = NULL;

void ar_wiimoteDriverEventTask( void* wiimoteDriver ) {
  while (true) {
    ((arWiimoteDriver*) wiimoteDriver)->update();
  }
}

arMatrix4 wiiAnglesToRotMatrix( const float yaw, const float pitch, const float roll ) {
//  return ar_rotationMatrix( 'y', -yaw ) *
//         ar_rotationMatrix( 'z', -roll ) *
//         ar_rotationMatrix( 'x', pitch );
  return ar_rotationMatrix( 'y', -yaw ) *
         ar_rotationMatrix( 'x', pitch ) *
         ar_rotationMatrix( 'z', -roll );
}

void freeWiimotes() {
  if (!__wiimotes) {
    return;
  }
  for (int i=0; i<1; ++i) {
    if (__wiimotes[i]) {
      free( __wiimotes[i] );
    }
  }
  free( __wiimotes );
  __wiimotes = NULL;
}

#ifdef EnableWiimote

#include "wiiuse.h"

// button codes
const int __buttonCodes[] = {
  WIIMOTE_BUTTON_TWO,
  WIIMOTE_BUTTON_ONE,
  WIIMOTE_BUTTON_B,
  WIIMOTE_BUTTON_A,
  WIIMOTE_BUTTON_MINUS,
  WIIMOTE_BUTTON_HOME,
  WIIMOTE_BUTTON_LEFT,
  WIIMOTE_BUTTON_RIGHT,
  WIIMOTE_BUTTON_DOWN,
  WIIMOTE_BUTTON_UP,
  WIIMOTE_BUTTON_PLUS,
  NUNCHUK_BUTTON_Z,
  NUNCHUK_BUTTON_C
};

// Pointer mode callbacks (using the Wiimote normally).

// Yuk, #define not function.  Because IS_PRESSED itself is a #define.
#define doButton(theStruct, code, i) \
  if (IS_JUST_PRESSED( theStruct, code )) \
    __wiimoteDriver->queueButton( i, 1 ); \
  else if (IS_RELEASED( theStruct, code )) \
    __wiimoteDriver->queueButton( i, 0 ); \
  if (IS_PRESSED( theStruct, code )) \
    __wiimoteDriver->resetIdleTimer();

static void __pointerHandleEvent( struct wiimote_t* wm ) {
  if (!__wiimoteDriver) {
    ar_log_error() << "Event callback ignoring NULL wiimote driver pointer.\n";
    return;
  }

  for (int i=0; i<BUTTONS_WIIMOTE; ++i) {
    doButton(wm, __buttonCodes[i], i);
  }

  if (WIIUSE_USING_ACC(wm)) {
    const float yaw   = ar_convertToRad( wm->orient.yaw );
    const float pitch = ar_convertToRad( wm->orient.pitch );
    const float roll  = ar_convertToRad( wm->orient.roll );
    __wiimoteDriver->queueAxis( 0, yaw );
    __wiimoteDriver->queueAxis( 1, pitch );
    __wiimoteDriver->queueAxis( 2, roll );
    __wiimoteDriver->queueMatrix( 0, wiiAnglesToRotMatrix( yaw, pitch, roll ));
  }

#if 0
  // Ubuntu gutsy gibbon (isl92) needs this.
  #define expNunchuk EXP_NUNCHUK
#else
  // Slackware 10 needs this.
  #define expNunchuk expansion_t::EXP_NUNCHUK
#endif
  if (wm->exp.type == expNunchuk) {
    struct nunchuk_t* nc = (nunchuk_t*)&wm->exp.nunchuk;
    doButton(nc, NUNCHUK_BUTTON_C, BUTTONS_WIIMOTE);
    doButton(nc, NUNCHUK_BUTTON_Z, BUTTONS_WIIMOTE+1);
    const float yaw   = ar_convertToRad( nc->orient.yaw );
    const float pitch = ar_convertToRad( nc->orient.pitch );
    const float roll  = ar_convertToRad( nc->orient.roll );
    __wiimoteDriver->queueAxis( 5, yaw );
    __wiimoteDriver->queueAxis( 6, pitch );
    __wiimoteDriver->queueAxis( 7, roll );
    __wiimoteDriver->queueMatrix( 1, wiiAnglesToRotMatrix( yaw, pitch, roll ) );

    const float joyAng = ar_convertToRad( nc->js.ang );
    const float joyMag = nc->js.mag;
    __wiimoteDriver->queueAxis( 3, joyMag*sin( joyAng ) );
    __wiimoteDriver->queueAxis( 4, joyMag*cos( joyAng ) );
    if (joyMag > .2) {
      __wiimoteDriver->resetIdleTimer();
    }
  }

  __wiimoteDriver->sendQueue();
}

static void doBattery(struct wiimote_t* wm, const float battery_level) {
  static int LEDsPrev = WIIMOTE_LED_NONE;
  const int LEDs = (battery_level > .75) ?
      WIIMOTE_LED_1 | WIIMOTE_LED_2 | WIIMOTE_LED_3 | WIIMOTE_LED_4 :
    (battery_level > .5) ?
      WIIMOTE_LED_1 | WIIMOTE_LED_2 | WIIMOTE_LED_3 :
    (battery_level > .25) ?
      WIIMOTE_LED_1 | WIIMOTE_LED_2 :
      WIIMOTE_LED_1;
  if (LEDs != LEDsPrev) {
    wiiuse_set_leds( wm, LEDs );
    LEDsPrev = LEDs;
  }
}

static void __pointerHandleCtrlStatus(
  struct wiimote_t* wm,
  int attachment,
  int /*speaker*/,
  int ir,
  int* /*led*/,
  float battery_level ) {
  if (!__wiimoteDriver) {
    ar_log_error() << "Status callback ignoring NULL wiimote driver pointer.\n";
    return;
  }

  doBattery(wm, battery_level);
  ar_log_debug() << "Nunchuk: " << attachment <<
    "\nIR:      " << ir <<
    "\nBattery: " << battery_level << "\n";

  const int axes_eulerangles = 3;
  int numButtons  = BUTTONS_WIIMOTE;
  int numAxes     = AXES_WIIMOTE + axes_eulerangles;
  int numMatrices = MATRIX_WIIMOTE;
  if (attachment) {
    numButtons  += BUTTONS_NUNCHUK;
    numAxes     += AXES_NUNCHUK + axes_eulerangles;
    numMatrices += MATRIX_NUNCHUK;
  }
  __wiimoteDriver->updateSignature( numButtons, numAxes, numMatrices );
}

static void __pointerHandleDisconnect( struct wiimote_t* /*wm*/ ) {
  if (!__wiimoteDriver) {
    ar_log_error() << "Disconnect callback ignoring NULL wiimote driver pointer.\n";
    return;
  }
  ar_log_remark() << "Disconnected wiimote.\n";
}


// Head-tracking mode callbacks (using the Wiimote camera to track IR LEDs on head,
// see http://johnnylee.net/).

static void __headHandleEvent( struct wiimote_t* /*wm*/ ) {
}
static void __headHandleCtrlStatus(
  struct wiimote_t* /*wm*/,
  int /*attachment*/,
  int /*speaker*/,
  int /*ir*/,
  int* /*led*/,
  float /*battery_level*/ ) {
}
static void __headHandleDisconnect( struct wiimote_t* /*wm*/ ) {
}

// Finger-tracking mode callbacks (using the Wiimote camera to track LEDs or reflectors
// on fingertips a la Minority Report, see http://johnnylee.net).

static void __fingersHandleEvent( struct wiimote_t* wm ) {
  // todo: member of __wiimoteDriver instead of static
  static bool lastButtonState = false;
  static float xAvgStart = -1.;
  static float yAvgStart = -1.;
  static float distanceStart = -1.;
  static float angleStart = -1.;
  if (!__wiimoteDriver) {
    ar_log_error() << "Event callback ignoring NULL wiimote driver pointer.\n";
    return;
  }
  if (!WIIUSE_USING_IR(wm)) {
    ar_log_error() << "arWiimoteDriver in 'fingers' mode with IR disabled.\n";
    return;
  }
  const ir_dot_t* led1 = &(wm->ir.dot[0]);
  const ir_dot_t* led2 = &(wm->ir.dot[1]);
  const bool buttonState = led1->visible && led2->visible;
  if (buttonState != lastButtonState) {
    __wiimoteDriver->queueButton( 0, buttonState );
    if (!buttonState) {
      for (int i=0; i<4; ++i) {
        __wiimoteDriver->queueAxis( i, 0. );
      }
      xAvgStart = -1.;
    }
  }
  if (buttonState) {
    const short x1 = led1->rx;
    const short y1 = led1->ry;
    const short x2 = led2->rx;
    const short y2 = led2->ry;
    const float xAvg = .5 * (x1+x2);
    const float yAvg = .5 * (y1+y2);
    const float xd = x2-x1;
    const float yd = y2-y1;
    const float distance = sqrt(xd*xd+yd*yd);
    const float angle = atan2( y2-y1, x2-x1 );
    if (xAvgStart < 0.) {
      xAvgStart = xAvg;
      yAvgStart = yAvg;
      distanceStart = distance;
      angleStart = angle;
    } else {
      __wiimoteDriver->queueAxis( 0, (xAvg - xAvgStart) / 512. );
      __wiimoteDriver->queueAxis( 1, (yAvg - yAvgStart) / 384. );
      __wiimoteDriver->queueAxis( 2, (distance - distanceStart) / 512. );
      __wiimoteDriver->queueAxis( 3, angle - angleStart );
    }
  }
  lastButtonState = buttonState;
  __wiimoteDriver->sendQueue();
}

static void __fingersHandleCtrlStatus(
  struct wiimote_t* wm,
  int /*attachment*/,
  int /*speaker*/,
  int ir,
  int* /*led*/,
  float battery_level ) {
  if (!__wiimoteDriver) {
    ar_log_error() << "Status callback ignoring NULL wiimote driver pointer.\n";
    return;
  }

  doBattery(wm, battery_level);
  ar_log_debug() << "IR:      " << ir << "\nBattery: " << battery_level << "\n";
}
static void __fingersHandleDisconnect( struct wiimote_t* /*wm*/ ) {
  if (!__wiimoteDriver) {
    ar_log_error() << "Disconnect callback ignoring NULL wiimote driver pointer.\n";
    return;
  }
  ar_log_remark() << "Wiimote disconnected.\n";
}

static wiiuse_event_cb       __handleEvent      = __pointerHandleEvent;
static wiiuse_ctrl_status_cb __handleCtrlStatus = __pointerHandleCtrlStatus;
static wiiuse_dis_cb         __handleDisconnect = __pointerHandleDisconnect;

#endif

static int __wiimote_ids[] = { WIIMOTE_ID_1 };

arWiimoteDriver::arWiimoteDriver() :
  arInputSource(),
  _findTimeoutSecs(5),
  _connected(0),
  _useMotion(false),
  _useIR(false),
  _idleTimer(6.e7),
  _statusTimer(1.e7) {
}

bool arWiimoteDriver::init( arSZGClient& szgClient ) {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
  return false;
#else
  __wiimoteDriver = this;
  const string mode = szgClient.getAttribute( "SZG_WIIMOTE", "mode", "|pointer|head|fingers|" );
  const string useIR = szgClient.getAttribute( "SZG_WIIMOTE", "useIR", "|true|false|" );

  if (mode == "pointer") {
LPointer:
    __handleEvent      = __pointerHandleEvent;
    __handleCtrlStatus = __pointerHandleCtrlStatus;
    __handleDisconnect = __pointerHandleDisconnect;
    _useMotion = true;
    _useIR = (useIR != "false");
    cout << "useIR: " << _useIR << endl;
  } else if (mode == "head") {
    __handleEvent      = __headHandleEvent;
    __handleCtrlStatus = __headHandleCtrlStatus;
    __handleDisconnect = __headHandleDisconnect;
    _useIR = true;
  } else if (mode == "fingers") {
    __handleEvent      = __fingersHandleEvent;
    __handleCtrlStatus = __fingersHandleCtrlStatus;
    __handleDisconnect = __fingersHandleDisconnect;
    _useIR = true;
    updateSignature( 1, 4, 0 );
  } else {
    // shouldn't ever happen.
    ar_log_error() << "Invalid SZG_WIIMOTE/mode '" << mode << ", defaulting to 'pointer'.\n";
    goto LPointer;
  }

  _connect();
  return true;
#endif
}

bool arWiimoteDriver::start() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
  return false;
#else
  return _eventThread.beginThread( ar_wiimoteDriverEventTask, this );
#endif
}

bool arWiimoteDriver::stop() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
  return false;
#else
  if (_connected) {
    _disconnect();
  }
  ar_log_remark() << "Wiimote driver stopped.\n";
  return true;
#endif
}

void arWiimoteDriver::update() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
#else
  if (!_connected && !_connect()) {
    return;
  }
  wiiuse_poll( __wiimotes, 1 );
  if (_statusTimer.done()) {
    updateStatus();
  }
//  if (_idleTimer.done()) {
//    _disconnect();
//  }
#endif
}

bool arWiimoteDriver::_connect() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
  return false;
#else
  //  Load the wiiuse library
  _version = wiiuse_startup( WIIUSE_PATH );
  if (!_version) {
    ar_log_error() << "Failed to load wiiuse library " << WIIUSE_PATH << ".\n";
    return false;
  }
  ar_log_remark() << "Wiiuse library version " << _version << ar_endl;

  // Initialize an array of wiimote objects.
  //
  // handle_event gets called when a generic event occurs (button press, motion sensing, etc)
  // handle_ctrl_status gets called when a response to a status request arrives (battery power, etc)
  // handle_disconnect gets called when the wiimote disconnect (holding power button)
  __wiimotes = wiiuse_init( 1, __wiimote_ids, __handleEvent, __handleCtrlStatus, __handleDisconnect );

  ar_log_critical() << "Connecting to wiimote. Press wiimote's buttons '1' and '2'.\n";
  // Find wiimotes (only one?).
  const int found = wiiuse_find( __wiimotes, 1, _findTimeoutSecs );
  if (found == 0) {
    ar_log_error() << "Found no wiimote; timed out after " << _findTimeoutSecs << " seconds.\n";
    goto LAbort;
  }
  ar_log_remark() << "Found " << found << " wiimote(s).\n"; // that were in discovery mode

  // Found some wiimotes. Connect to them.
  // Give the function the wiimote array and the number of wiimote devices found.
  // Return how many established connections to the found wiimotes.
  _connected = wiiuse_connect( __wiimotes, 1 );
  if (!_connected) {
    ar_log_error() << "Connected to no wiimote.\n";
    goto LAbort;
  }
  if (_connected < found) {
    ar_log_error() << "Connected to only " << _connected << " of " << found << " wiimotes.\n";
    goto LAbort;
  }
  if (_connected > found) {
    ar_log_error() << "Internal error: connected to " << _connected << " of (only!) " <<
      found << " wiimotes.\n";
LAbort:
  wiiuse_shutdown();
  freeWiimotes();
  _connected = 0;
  return false;
  }

  ar_log_remark() << "Connected to " << _connected << " wiimotes.\n";

  // Blink and rumble to show which wiimotes connected, just like the wii.
  wiiuse_set_leds( __wiimotes[0], WIIMOTE_LED_1 | WIIMOTE_LED_4 );
  wiiuse_rumble( __wiimotes[0], 1 );
  ar_usleep( 200000 );
  wiiuse_rumble( __wiimotes[0], 0 );
  wiiuse_set_leds( __wiimotes[0], WIIMOTE_LED_NONE );

  wiiuse_motion_sensing( __wiimotes[0], _useMotion );
  wiiuse_set_ir( __wiimotes[0], _useIR );
  updateStatus();
  resetIdleTimer();
  return true;
#endif
}

void arWiimoteDriver::updateStatus() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
#else
  // Invoke the __handleCtrlStatus() callback.
  wiiuse_status( __wiimotes[0] );
  _statusTimer.start( 3.e7 );
#endif
}

void arWiimoteDriver::updateSignature( const int numButtons, const int numAxes, const int numMatrices ) {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
#else
  _setDeviceElements( numButtons, numAxes, numMatrices );
#endif
}

void arWiimoteDriver::_disconnect() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
#else
  wiiuse_rumble( __wiimotes[0], 1 );
  ar_usleep( 100000 );
  wiiuse_rumble( __wiimotes[0], 0 );
  wiiuse_set_leds( __wiimotes[0], WIIMOTE_LED_NONE );
  wiiuse_disconnect( __wiimotes[0] );
  _connected = 0;
  ar_log_debug() << "Wiimote disconnected.\n";
  // Unload the wiiuse library
  wiiuse_shutdown();
  freeWiimotes();
#endif
}

void arWiimoteDriver::resetIdleTimer() {
#ifndef EnableWiimote
  ar_log_error() << "Wiimote support not compiled.\n";
#else
  //_idleTimer.start( 6.e7 );
#endif
}
