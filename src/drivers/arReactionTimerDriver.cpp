//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arReactionTimerDriver.h"

#include <string>

#define RTDEBUG

DriverFactory(arReactionTimerDriver, "arInputSource")

//const std::string RT_AWAKE = "!";
const char RT_AWAKE = '!';
double RT_TIMEOUT = 10.;
const unsigned int BUF_SIZE = 4096;

void ar_RTDriverEventTask(void* theDriver) {
  ar_log_warning() << "arReactionTimerDriver remark: started event task.\n";
  arReactionTimerDriver* rtDriver = (arReactionTimerDriver*) theDriver;
  rtDriver->_stopped = false;
  rtDriver->_eventThreadRunning = true;
  while (!rtDriver->_stopped  && rtDriver->_eventThreadRunning) {
    rtDriver->_eventThreadRunning = rtDriver->_processInput();
  }
  rtDriver->_eventThreadRunning = false;
}

arReactionTimerDriver::arReactionTimerDriver() :
  _inited( false ),
  _imAlive( true ),
  _stopped( true ),
  _eventThreadRunning( false ),
  _inbuf(0) {
}

arReactionTimerDriver::~arReactionTimerDriver() {
  _port.ar_close();  // OK even if not open.
  if (_inbuf)
    delete[] _inbuf;
}

bool arReactionTimerDriver::init(arSZGClient& SZGClient) {
  _inbuf = new char[BUF_SIZE];
  if (!_inbuf) {
    ar_log_warning() << "arReactionTimerDriver error: failed to allocate input buffer.\n";
    return false;
  }
  _portNum = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_RT", "com_port"));
  _inited = true;
  // 2 buttons, 2 axes, no matrices.
  _setDeviceElements( 2, 2, 0 );
  ar_log_warning() << "arReactionTimerDriver remark: initialized.\n";
  return true;
}

bool arReactionTimerDriver::start() {
  if (!_inited) {
    ar_log_warning() << "arReactionTimerDriver::start() error: Not inited yet.\n";
    return false;
  }
  if (!_port.ar_open( _portNum, 9600, 8, 1, "none" )) {
    ar_log_warning() << "arReactionTimerDriver error: failed to open serial port #" << _portNum << ".\n";
    return false;
  }
  if (!_port.setReadTimeout( 2 )) {  // 200 msec
    ar_log_warning() << "arReactionTimerDriver error: failed to set timeout COM port.\n";
    return false;
  }
  _resetStatusTimer();
  return _eventThread.beginThread(ar_RTDriverEventTask,this);
}

bool arReactionTimerDriver::stop() {
  ar_log_warning() << "arReactionTimerDriver remark: stopping.\n";
  _stopped = true;
  arSleepBackoff a(5, 20, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  ar_log_warning() << "arReactionTimerDriver remark: event thread exiting.\n";
  _port.ar_close();
  return true;
}

bool arReactionTimerDriver::_processInput() {
  const unsigned int numToRead = 3; // Minimum message size ("!\r\n")
  const int numRead = _port.ar_read( _inbuf, numToRead, BUF_SIZE-1 );
  if (numRead == 0) {
    if (_statusTimer.done()) {
      ar_log_warning() << "arReactionTimerDriver warning: ReactionTimer disconnected.\n";
      _imAlive = false;
    }
    return true; 
  }
  _resetStatusTimer();
  // Make sure it's 0-terminated (the arRS232Port won't)
  _inbuf[numRead] = '\0';
  _bufString += std::string( _inbuf );
  std::string::size_type crpos;
  do {
    crpos = _bufString.find("\r\n");
    if (crpos == std::string::npos) {
      return true;
    }
    if (!_imAlive) {
      ar_log_remark() << "arReactionTimerDriver remark: ReactionTimer reconnected.\n";
      _imAlive = true;
    }
    std::string messageString( _bufString.substr( 0, crpos ) );
    _bufString.erase( 0, crpos+2 );
    if (messageString[0] == RT_AWAKE) {
#ifdef RTDEBUG
      if (messageString.size() > 1) {
        ar_log_remark() << "+++++++++++++++++\nDEBUG: " << messageString << "\n++++++++++++++++++\n";
      }
#endif
    } else {
      arDelimitedString inputString( messageString, '|' );
      if (inputString.size() != 3) {
        ar_log_warning() << "arReactionTimerDriver warning: ill-formed input string:\n"
             << "     " << inputString << ".\n";
        continue;
      }
#ifdef RTDEBUG
    ar_log_remark() << "---------------------------------------\n";
#endif

      // NOTE: rtDuration is currently encoded as a whole number of msecs.
      int rtDuration = 0;
      int button0 = 0;
      int button1 = 0;
      static int lastrt = -2;
      static int lastb0 = 0;
      static int lastb1 = 0;
//      istringstream numStream( inputString );
//      numStream >> rtDuration;
//      numStream >> button0;
//      numStream >> button1;
      istringstream rtStream( inputString[0] );
      rtStream >> rtDuration;
      istringstream b0Stream( inputString[1] );
      b0Stream >> button0;
      istringstream b1Stream( inputString[2] );
      b1Stream >> button1;

#ifdef RTDEBUG
      ar_log_remark() << "Input: " << inputString << ": " << rtDuration << ", " << button0 << ", " << button1 << ".\n";
//      ar_log_warning() << "RT: '" << inputString[0] << "', " << rtDuration << ".\n";
//      ar_log_warning() << "Button 0: '" << inputString[1] << "', " << button0 << ".\n";
//      ar_log_warning() << "Button 1: '" << inputString[2] << "', " << button1 << ".\n";
#endif
      
      if ((rtDuration > 0.)&&(lastrt < 0.)) {
        _rtTimer.reset();
        _rtTimer.start();
        _rtTimer.setRuntime( rtDuration*1.e3 );
#ifdef RTDEBUG
        ar_log_remark() << "Setting runtime to " << rtDuration*1.e3 << " microsecs.\n";
#endif
      }
      if (button0 != lastb0) {
        if (button0)
          queueAxis( 0, _rtTimer.runningTime()/1.e3 );
        queueButton( 0, button0 );
        lastb0 = button0;
#ifdef RTDEBUG
        ar_log_remark() << "RT 0: " <<_rtTimer.runningTime()/1.e3 << ".\n";
#endif
      }
      if (button1 != lastb1) {
        if (button1)
          queueAxis( 1, _rtTimer.runningTime()/1.e3 );
        queueButton( 1, button1 );
        lastb1 = button1;
#ifdef RTDEBUG
        ar_log_remark() << "RT 1: " << _rtTimer.runningTime()/1.e3 << ".\n";
#endif
      }
      sendQueue();
      lastrt = rtDuration;
#ifdef RTDEBUG
    ar_log_remark() << "---------------------------------------\n";
#endif
    }
  } while (crpos != std::string::npos);
  return true;
}

void arReactionTimerDriver::_resetStatusTimer() {
  _statusTimer.start( RT_TIMEOUT*1.e6 );
}
