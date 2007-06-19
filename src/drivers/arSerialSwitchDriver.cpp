//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSerialSwitchDriver.h"

const char DEFAULT_SIGNAL(0x53);

DriverFactory(arSerialSwitchDriver, "arInputSource")

arSerialSwitchDriver::arSerialSwitchDriver() :
  _timeoutTenths( 30 ),
  _eventThreadRunning(false),
  _stopped(false),
  _lastState(AR_NO_SWITCH_EVENT),
  _lastEventTime(0,0)
{}

arSerialSwitchDriver::~arSerialSwitchDriver() {
}

bool arSerialSwitchDriver::init(arSZGClient& SZGClient){
  int i = 0;

  // Determine the baud rate.
  const int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
  const int NUM_BAUD_RATES(7);
  bool baudRateFound = false;
  int baudRate = SZGClient.getAttributeInt("SZG_SERIALSWITCH", "baud_rate");
  if (baudRate == 0)
    goto LDefaultBaudRate;
  for (i=0; i<NUM_BAUD_RATES; ++i){
    if (baudRate == baudRates[i]) {
      baudRateFound = true;
      break;
    }
  }
  if (!baudRateFound) {
    cerr << "arSerialSwitchDriver got unexpected SZG_SERIALSWITCH/baud_rate "
	 << baudRate << ".\n  Expected one of:";
    for (i=0; i<NUM_BAUD_RATES; ++i)
      cerr << " " << baudRates[i];
    cerr << "\n";
LDefaultBaudRate:
    baudRate = 115200;
    cerr << "arSerialSwitchDriver SZG_SERIALSWITCH/baud_rate defaulting to "
         << baudRate << ".\n";
  }

  // Multiple serial ports aren't implemented.
  _comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_SERIALSWITCH", "com_port"));
  if (_comPortID <= 0) {
    ar_log_warning() << "arSerialSwitchDriver: SZG_SERIALSWITCH/com_port defaulting to 1.\n";
    _comPortID = 1;
  }
  if (!_comPort.ar_open(_comPortID,(unsigned long)baudRate,8,1,"none" )){
    ar_log_warning() << "arSerialSwitchDriver failed to open serial port " <<
      _comPortID << ".\n";
    return false;
  }

  if (!_comPort.setReadTimeout(_timeoutTenths)){
    ar_log_warning() << "arSerialSwitchDriver failed to set timeout for serial port " <<
      _comPortID << ".\n";
    return false;
  }

  string eventType = SZGClient.getAttribute( "SZG_SERIALSWITCH", "event_type", "|both|open|closed|" );
  if (eventType == "closed") {
    _eventType = AR_CLOSED_SWITCH_EVENT;
  } else if (eventType == "open") {
    _eventType = AR_OPEN_SWITCH_EVENT;
  } else {
    _eventType = AR_BOTH_SWITCH_EVENT;
  }

  int signalByte = SZGClient.getAttributeInt( "SZG_SERIALSWITCH", "signal_byte" );
  if (signalByte == 0) {
    ar_log_warning() << "SZG_SERIALSWITCH/signal_byte not set or set to 0, using default ("
                     << (int)DEFAULT_SIGNAL << ").\n";
    signalByte = DEFAULT_SIGNAL;
  } else if (signalByte > 255) {
    ar_log_error() << "SZG_SERIALSWITCH/signal_byte must be < 256, using default ("
                     << (int)DEFAULT_SIGNAL << ") instead.\n";
    signalByte = DEFAULT_SIGNAL;
  }
  _outChar = (char)signalByte;

  // Report one axis, the time since the last event occurred.
  _setDeviceElements( 0, 1, 0 );

  cerr << "arSerialSwitchDriver inited().\n";
  return true;
}

bool arSerialSwitchDriver::start() {
  return _eventThread.beginThread(ar_SerialSwitchDriverEventTask,this);
}

void ar_SerialSwitchDriverEventTask(void* driver){
  ((arSerialSwitchDriver*)driver)->_eventloop();
}

void arSerialSwitchDriver::_eventloop(){
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

bool arSerialSwitchDriver::stop() {
  _stopped = true;
  arSleepBackoff a(10, 30, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  _comPort.ar_close();
  ar_log_debug() << "arSerialSwitchDriver stopped.\n";
  return true;
}

bool arSerialSwitchDriver::_poll( void ) {
  if (_stopped)
    return false;
  if (_comPort.ar_write( &_outChar, 1 ) != 1) {
    cerr << "ar_write failed.\n";
    return false;
  }
  if (_comPort.ar_read( &_inChar, 1) != 1) {
    cerr << "ar_read failed.\n";
    return false;
  }
  arSerialSwitchEventType switchState = (arSerialSwitchEventType)((_inChar == _outChar)+1);
  if (switchState != _lastState) {
    if (switchState & _eventType) {
      if (_lastEventTime.zero()) {
        _lastEventTime = ar_time();
      } else {
        ar_timeval currTime = ar_time();
        double diffTime = ar_difftime( currTime, _lastEventTime );
        if (switchState == AR_OPEN_SWITCH_EVENT) {
          diffTime = -diffTime;
        }
        sendAxis( 0, (float)(diffTime/1.e6) );
//        cerr << diffTime << endl;
        _lastEventTime = currTime;
      }
    }
    _lastState = switchState;
  }
  return true;
}

