//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTreadmillDriver.h"

const char SIGNAL(0x53);

DriverFactory(arTreadmillDriver, "arInputSource")

arTreadmillDriver::arTreadmillDriver() :
  _timeoutTenths( 30 ),
  _eventThreadRunning(false),
  _stopped(false),
  _charBuf(SIGNAL),
  _lastState(false),
  _lastEventTime(0,0)
{}

arTreadmillDriver::~arTreadmillDriver() {
}

bool arTreadmillDriver::init(arSZGClient& SZGClient){
  int i = 0;

  // Determine the baud rate.
  const int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
  const int NUM_BAUD_RATES(7);
  bool baudRateFound = false;
  int baudRate = SZGClient.getAttributeInt("SZG_TREADMILL", "baud_rate");
  if (baudRate == 0)
    goto LDefaultBaudRate;
  for (i=0; i<NUM_BAUD_RATES; ++i){
    if (baudRate == baudRates[i]) {
      baudRateFound = true;
      break;
    }
  }
  if (!baudRateFound) {
    cerr << "arTreadmillDriver got unexpected SZG_TREADMILL/baud_rate "
	 << baudRate << ".\n  Expected one of:";
    for (i=0; i<NUM_BAUD_RATES; ++i)
      cerr << " " << baudRates[i];
    cerr << "\n";
LDefaultBaudRate:
    baudRate = 115200;
    cerr << "arTreadmillDriver SZG_TREADMILL/baud_rate defaulting to "
         << baudRate << ".\n";
  }

  // Multiple serial ports aren't implemented.
  _comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_TREADMILL", "com_port"));
  if (_comPortID <= 0) {
    ar_log_warning() << "arTreadmillDriver: SZG_TREADMILL/com_port defaulting to 1.\n";
    _comPortID = 1;
  }
  if (!_comPort.ar_open(_comPortID,(unsigned long)baudRate,8,1,"none" )){
    ar_log_warning() << "arTreadmillDriver failed to open serial port " <<
      _comPortID << ".\n";
    return false;
  }

  if (!_comPort.setReadTimeout(_timeoutTenths)){
    ar_log_warning() << "arTreadmillDriver failed to set timeout for serial port " <<
      _comPortID << ".\n";
    return false;
  }

  // Report one axis, the time since the last event occurred.
  _setDeviceElements( 0, 1, 0 );

  cerr << "arTreadmillDriver inited().\n";
  return true;
}

bool arTreadmillDriver::start() {
  return _eventThread.beginThread(ar_TreadmillDriverEventTask,this);
}

void ar_TreadmillDriverEventTask(void* driver){
  ((arTreadmillDriver*)driver)->_eventloop();
}

void arTreadmillDriver::_eventloop(){
  _eventThreadRunning = true;
  for (;;) {
    if (!_poll()) {
      cerr << "_poll() failed.\n";
      _stopped = true;
    }
    if (_stopped) {
      _eventThreadRunning = false;
      stop();
      return;
    }
  }
}

bool arTreadmillDriver::stop() {
  _stopped = true;
  arSleepBackoff a(10, 30, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  _comPort.ar_close();
  ar_log_debug() << "arTreadmillDriver stopped.\n";
  return true;
}

bool arTreadmillDriver::_poll( void ) {
  if (_stopped)
    return false;
  if (_comPort.ar_write( &SIGNAL, 1 ) != 1) {
    cerr << "ar_write failed.\n";
    return false;
  }
  if (_comPort.ar_read( &_charBuf, 1) != 1) {
    cerr << "ar_read failed.\n";
    return false;
  }
  bool state = _charBuf == SIGNAL;
  if (state != _lastState) {
    if (state) {
      if (_lastEventTime.zero()) {
        _lastEventTime = ar_time();
      } else {
        ar_timeval currTime = ar_time();
        double diffTime = ar_difftime( currTime, _lastEventTime );
        sendAxis( 0, (float)(diffTime/1.e6) );
//        cerr << diffTime << endl;
        _lastEventTime = currTime;
      }
    }
    _lastState = state;
  }
  return true;
}

