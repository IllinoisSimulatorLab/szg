//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Logitech head-tracker driver

#include "arPrecompiled.h"
#include "arLogitechDriver.h"

#include <string>

#if 0

Here is PForth code for the arPForthFilter to transform coords
(assuming the frame is mounted directly above head with cable on the front)

  /* Coordinate transformation for Logitech tracker.
     Change the axes such that X->X, Y->Z, and Z->-Y
     (essentially a 90-deg rotation about X).
     Except we're applying it to a matrix, so we have to do t1*M*inv(t1) */
  matrix inputMatrix
  matrix rotMat1
  matrix rotMat2
  matrix transMatrix
  matrix outputMatrix
  -90 xaxis rotMat1 rotationMatrix
  90 xaxis rotMat2 rotationMatrix
  /* Also raise the head position 4.5 ft, because all the demos assume
     the head will be at +several feet */
  0 4.5 0 transMatrix translationMatrix
  define filter_matrix_0
    inputMatrix getCurrentEventMatrix
    inputMatrix rotMat1 outputMatrix matrixMultiply
    rotMat2 outputMatrix outputMatrix matrixMultiply
    transMatrix outputMatrix outputMatrix matrixMultiply
    outputMatrix setCurrentEventMatrix
  enddef

#endif

DriverFactory(arLogitechDriver, "arInputSource")

void ar_LogitechDriverEventTask(void* driver) {
  arLogitechDriver* logDriver = (arLogitechDriver*) driver;
  logDriver->_eventThreadRunning = true;
  logDriver->_startStreaming();
  bool stat(true);
  while ((!logDriver->_stopped)&&(stat)) {
    stat = logDriver->_update();
  }
  logDriver->_eventThreadRunning = false;
}

arLogitechDriver::arLogitechDriver() :
  _woken( false ),
  _stopped( false ),
  _charsInBuffer( 0 ),
  _eventThreadRunning( false )
{}

arLogitechDriver::~arLogitechDriver() {
  if (_woken) {
    stop();
  }
  ar_log_debug() << "arLogitechDriver closing COM port.\n";
  _comPort.ar_close();
}

bool arLogitechDriver::init(arSZGClient& SZGClient) {
  _setDeviceElements( 0, 0, 1 );
  const unsigned comPortID = static_cast<unsigned>
    (SZGClient.getAttributeInt("SZG_LOGITECH", "com_port"));
  if (!_comPort.ar_open( comPortID, 19200, 8, 1, "none" )){
    ar_log_warning() << "arLogitechDriver failed to open serial port " << comPortID << ".\n";
    return false;
  }
  ar_log_debug() << "arLogitechDriver opened serial port.\n";

  if (!_comPort.setReadTimeout(10)){
    ar_log_warning() << "arLogitechDriver failed to set 1-second timeout for COM port.\n";
    return false;
  }
  
  _woken = _reset() && _runDiagnostics();
  return _woken;
}

bool arLogitechDriver::start(){
  if (!_woken) {
    ar_log_warning() << "arLogitechDriver ignoring start() before init().\n";
    return false;
  }

  const bool ok = _eventThread.beginThread(ar_LogitechDriverEventTask,this);
  if (ok) {
    ar_log_debug() << "arLogitechDriver started.\n";
  } else {
    ar_log_warning() << "arLogitechDriver failed to start event thread.\n";
  }
  return ok;
}

bool arLogitechDriver::stop(){
  _stopped = true;
  arSleepBackoff a(10, 20, 1.1);
  while (_eventThreadRunning) {
    a.sleep();
  }

  if (_woken) {
    if (!_reset()) {
      return false;
    }
    _woken = false;
    ar_log_debug() << "arLogitechDriver stopped.\n";   
  }
  return true;
}

bool arLogitechDriver::_reset() {
  // Activate 6D mode
  const int cb = _comPort.ar_write( "*R" );
  if (cb < 2) {
    ar_log_warning() << "arLogitechDriver: _reset wrote only " << cb << " bytes.\n";
    return false;
  }

  ar_usleep(1100000); // hardware needs >1 second
  return true;
}

bool arLogitechDriver::_startStreaming() {
  const int cb = _comPort.ar_write("*S");
  if (cb < 2) {
    ar_log_warning() << "arLogitechDriver: _startStreaming wrote only " << cb << " bytes.\n";
    return false;
  }
  return true;
}

// interpretation of diagnostic bits
// Byte 1
#define DIAG_MOTHERBOARD            0x01
#define DIAG_CPU                    0x02
#define DIAG_ROM                    0x04
#define DIAG_RAM                    0x08
#define DIAG_T_FRAME                0x10
#define DIAG_MOUSE                  0x20

// Byte 2
#define DIAG_COMM_PORT              0x01
#define DIAG_EEPROM                 0x02

bool arLogitechDriver::_runDiagnostics() {
  const int stat = _comPort.ar_write("*\05");
  if (stat < 2) {
    cerr << "arLogitechDriver error: wrote " << stat << " bytes instead of 2 in _runDiagnostics().\n";
    return false;
  }
  const int bytesRead = _comPort.ar_read( _dataBuffer, 2 );
  if (bytesRead != 2) {
    cerr << "arLogitechDriver error: tracker failed to respond to the 'diagnostic' command\n"
         << "   (# bytes read = " << bytesRead << ").\n";
    return false;
  }
//  printf("%x %x\n",(int)_dataBuffer[0],_dataBuffer[1]);
//  cerr << (int)_dataBuffer[0] << ", " << (int)_dataBuffer[1] << "\n";
  const unsigned char aByte( ~_dataBuffer[0] );
  const unsigned char bByte( ~_dataBuffer[1] );
  if (!((aByte & 0xBF)||(bByte & 0x3F)))
    return true;

  cerr << "arLogitechDriver diagnostic failure:\n";
  if (aByte & DIAG_MOTHERBOARD)
    cerr << "  Motherboard failed.\n";
  if (aByte & DIAG_CPU)
    cerr << "  CPU failed.\n";
  if (aByte & DIAG_ROM)
    cerr << "  ROM failed.\n";
  if (aByte & DIAG_RAM)
    cerr << "  RAM failed.\n";
  if (aByte & DIAG_T_FRAME)
    cerr << "  T-frame failed.\n";
  if (aByte & DIAG_MOUSE)
    cerr << "  Mouse failed.\n";
  if (bByte & DIAG_COMM_PORT)
    cerr << "  Comm. port failed.\n";
  if (bByte & DIAG_EEPROM)
    cerr << "  EEPROM failed.\n";
  return false;
}

bool arLogitechDriver::_update() {
  using namespace arLogitechDriverSpace;
  const unsigned int numToRead = ELEMENT_SIZE - _charsInBuffer;
  const int numRead = _comPort.ar_read( _dataBuffer+_charsInBuffer, numToRead, MAX_DATA );
  if (numRead < 0) {
    cerr << "arLogitechDriver error: read() failed in _update().\n";
    return false;
  }
  if (numRead == 0) {
    return true;
  }
  _charsInBuffer += numRead;
  const unsigned int numElements = static_cast<unsigned int>(floor(_charsInBuffer/(float)ELEMENT_SIZE));
  if (numElements > 0) {
    unsigned int i;
    for (i=0; i<numElements; ++i) {
      _convertSendData( _dataBuffer+i*ELEMENT_SIZE );
    }
    const unsigned int numUsed = numElements*ELEMENT_SIZE;
    const unsigned int numLeft = _charsInBuffer - numUsed;
    for (i=0; i<numLeft; ++i) {
      _dataBuffer[i] = _dataBuffer[i+numUsed]; 
    }
    _charsInBuffer = numLeft;
  }
  return true;
}

// interpretations of misc bits - buttons on input devices
#define logitech_FLAGBIT           0x80
#define logitech_FRINGEBIT         0x40
#define logitech_OUTOFRANGEBIT     0x20
#define logitech_RESERVED          0x10
#define logitech_SUSPENDBUTTON     0x08
#define logitech_LEFTBUTTON        0x04
#define logitech_MIDDLEBUTTON      0x02
#define logitech_RIGHTBUTTON       0x01

void arLogitechDriver::_convertSendData( char* record ) {

  // collect unit's miscellaneous information
  //const short buttons = (unsigned char)record[0] & (unsigned char)~logitech_FLAGBIT;

  long ax=0, ay=0, az=0;
  // absolute translational data
  // Sign extend if needed.
  ax = (record[1] & 0x40) ? 0xFFE00000 : 0;
  ax |= (long)(record[1] & 0x7f) << 14;
  ax |= (long)(record[2] & 0x7f) << 7;
  ax |= (record[3] & 0x7f);

  ay = (record[4] & 0x40) ? 0xFFE00000 : 0;
  ay |= (long)(record[4] & 0x7f) << 14;
  ay |= (long)(record[5] & 0x7f) << 7;
  ay |= (record[6] & 0x7f);

  az = (record[7] & 0x40) ? 0xFFE00000 : 0;
  az |= (long)(record[7] & 0x7f) << 14;
  az |= (long)(record[8] & 0x7f) << 7;
  az |= (record[9] & 0x7f);

  // calculate the positional floating point values
  const float INCHES_TO_FEET = 1./12.;
  const float x = INCHES_TO_FEET * ((float) ax) / 1000.0;
  const float y = INCHES_TO_FEET * ((float) ay) / 1000.0;
  const float z = INCHES_TO_FEET * ((float) az) / 1000.0;

  // absolute rotational data
  const short arx = ((record[10] & 0x7f) << 7) + (record[11] & 0x7f);
  const short ary = ((record[12] & 0x7f) << 7) + (record[13] & 0x7f);
  const short arz = ((record[14] & 0x7f) << 7) + (record[15] & 0x7f);

  // calculate the rotational floating point values
  const float xAngle = ar_convertToRad( ((float) arx) / 40.0 );
  const float yAngle = ar_convertToRad( ((float) ary) / 40.0 );
  const float zAngle = ar_convertToRad( ((float) arz) / 40.0 );

  sendMatrix( ar_translationMatrix(x,y,z) *
     ar_rotationMatrix('y',yAngle)*ar_rotationMatrix('x',xAngle)*ar_rotationMatrix('z',zAngle));
}
