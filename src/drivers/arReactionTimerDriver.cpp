//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************


// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#include "arDataUtilities.h"
#include "arReactionTimerDriver.h"

#define RTDEBUG

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arReactionTimerDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

//const std::string RT_AWAKE = "!";
const char RT_AWAKE = '!';
double RT_TIMEOUT = 10.;
const unsigned int BUF_SIZE = 4096;

void ar_RTDriverEventTask(void* theDriver) {
  cerr << "arReactionTimerDriver remark: started event task.\n";
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
    cerr << "arReactionTimerDriver error: failed to allocate input buffer.\n";
    return false;
  }
  _portNum = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_RT", "com_port"));
  _inited = true;
  // 2 buttons, 2 axes, no matrices.
  _setDeviceElements( 2, 2, 0 );
  cerr << "arReactionTimerDriver remark: initialized.\n";
  return true;
}

bool arReactionTimerDriver::start() {
  if (!_inited) {
    cerr << "arReactionTimerDriver::start() error: Not inited yet.\n";
    return false;
  }
  if (!_port.ar_open( _portNum, 9600, 8, 1, "none" )) {
    cerr << "arReactionTimerDriver error: failed to open serial port #" << _portNum << endl;
    return false;
  }
  if (!_port.setReadTimeout( 2 )) {  // 200 msec
    cerr << "arReactionTimerDriver error: failed to set timeout COM port.\n";
    return false;
  }
  _resetStatusTimer();
  return _eventThread.beginThread(ar_RTDriverEventTask,this);
}

bool arReactionTimerDriver::stop() {
  cerr << "arReactionTimerDriver remark: stopping.\n";
  _stopped = true;
  while (_eventThreadRunning) {
    ar_usleep(10000);
  }
  cerr << "arReactionTimerDriver remark: event thread exiting.\n";
  _port.ar_close();
  return true;
}

bool arReactionTimerDriver::_processInput() {
  const unsigned int numToRead = 3; // Minimum message size ("!\r\n")
  const int numRead = _port.ar_read( _inbuf, numToRead, BUF_SIZE-1 );
  if (numRead == 0) {
    if (_statusTimer.done()) {
      cerr << "arReactionTimerDriver warning: ReactionTimer disconnected.\n";
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
      cout << "arReactionTimerDriver remark: ReactionTimer reconnected.\n";
      _imAlive = true;
    }
    std::string messageString( _bufString.substr( 0, crpos ) );
    _bufString.erase( 0, crpos+2 );
    if (messageString[0] == RT_AWAKE) {
#ifdef RTDEBUG
      if (messageString.size() > 1) {
        cout << "+++++++++++++++++\nDEBUG: " << messageString << "\n++++++++++++++++++\n";
      }
#endif
    } else {
      arDelimitedString inputString( messageString, '|' );
      if (inputString.size() != 3) {
        cerr << "arReactionTimerDriver warning: ill-formed input string:\n"
             << "     " << inputString << endl;
        continue;
      }
#ifdef RTDEBUG
    cout << "---------------------------------------\n";
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
      cout << "Input: " << inputString << ": " << rtDuration << ", " << button0 << ", " << button1 << endl;
//      cerr << "RT: '" << inputString[0] << "', " << rtDuration << endl;
//      cerr << "Button 0: '" << inputString[1] << "', " << button0 << endl;
//      cerr << "Button 1: '" << inputString[2] << "', " << button1 << endl;
#endif
      
      if ((rtDuration > 0.)&&(lastrt < 0.)) {
        _rtTimer.reset();
        _rtTimer.start();
        _rtTimer.setRuntime( rtDuration*1.e3 );
#ifdef RTDEBUG
        cout << "Setting runtime to " << rtDuration*1.e3 << " microsecs.\n";
#endif
      }
      if (button0 != lastb0) {
        if (button0)
          queueAxis( 0, _rtTimer.runningTime()/1.e3 );
        queueButton( 0, button0 );
        lastb0 = button0;
#ifdef RTDEBUG
        cout << "RT 0: " <<_rtTimer.runningTime()/1.e3 << endl;
#endif
      }
      if (button1 != lastb1) {
        if (button1)
          queueAxis( 1, _rtTimer.runningTime()/1.e3 );
        queueButton( 1, button1 );
        lastb1 = button1;
#ifdef RTDEBUG
        cout << "RT 1: " << _rtTimer.runningTime()/1.e3 << endl;
#endif
      }
      sendQueue();
      lastrt = rtDuration;
#ifdef RTDEBUG
    cout << "---------------------------------------\n";
#endif
    }
  } while (crpos != std::string::npos);
  return true;
}

void arReactionTimerDriver::_resetStatusTimer() {
  _statusTimer.start( RT_TIMEOUT*1.e6 );
}
