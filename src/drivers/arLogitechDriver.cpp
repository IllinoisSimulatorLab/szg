//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// Logitech head-tracker driver
// Below is the PForth code for the arPForthFilter to do the coordinate
// transformation (assuming the frame is mounted directly above head
// with cable on the front)
//
///* Coordinate transformation for Logitech tracker.
//   We want to change the axes such that X->X, Y->Z, and Z->-Y
//   (essentially a 90-deg rotation about X).
//   Except we're applying it to a matrix, so we have to do the
//   t1*M*inv(t1) thing */
//matrix inputMatrix
//matrix rotMat1
//matrix rotMat2
//matrix transMatrix
//matrix outputMatrix
//-90 xaxis rotMat1 rotationMatrix
//90 xaxis rotMat2 rotationMatrix
///* Also raise the head position 4.5 ft, because all the demos assume
//   the head will be at +several feet */
//0 4.5 0 transMatrix translationMatrix
//define filter_matrix_0
//  inputMatrix getCurrentEventMatrix
//  inputMatrix rotMat1 outputMatrix matrixMultiply
//  rotMat2 outputMatrix outputMatrix matrixMultiply
//  transMatrix outputMatrix outputMatrix matrixMultiply
//  outputMatrix setCurrentEventMatrix
//enddef

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLogitechDriver.h"

#include <string>
#include <sstream>

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
  cerr << "arLogitechDriver remark: closing COM port.\n";
  _comPort.ar_close();
}

bool arLogitechDriver::init(arSZGClient& SZGClient) {
  // 0 buttons, 0 axes, 1 matrix
  _setDeviceElements( 0, 0, 1 );
  unsigned int comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_LOGITECH", "com_port"));
  // Go ahead and open the serial port
  if (!_comPort.ar_open( comPortID, 19200, 8, 1, "none" )){
    cerr << "arLogitechDriver error: could not open serial port " << comPortID
	 << ".\n";
    return false;
  }
  cerr << "arLogitechDriver remark: COM port open.\n";

  if (!_reset()) {
    cerr << "arLogitechDriver error: _reset() failed.\n";
    return false;
  }

  if (!_runDiagnostics()) {
    cerr << "arLogitechDriver error: diagnostics failed.\n";
    return false;
  }
  
  // Set read timeout of 1 sec
  if (!_comPort.setReadTimeout(10)){
    cerr << "arLogitechDriver error: could not set timeout for COM port.\n";
    return false;
  }
  
  _woken = true;
  return true;
}

bool arLogitechDriver::start(){
  if (!_woken) {
    cerr << "arLogitechDriver error: start() called with un-inited tracker.\n";
    return false;
  }
  bool stat = _eventThread.beginThread(ar_LogitechDriverEventTask,this);
  if (stat) {
    cerr << "arLogitechDriver started.\n";
  } else {
    cerr << "arLogitechDriver error: failed to start event thread.\n";
  }
  return stat;
}

bool arLogitechDriver::stop(){
  _stopped = true;
  while (_eventThreadRunning) {
    ar_usleep(10000);
//    cerr << "arLogitechDriver remark waiting for event thread.\n";
  }
  if (_woken) {
    if (!_reset()) {
      cerr << "arLogitechDriver error: _reset() failed in stop().\n";
      return false;
    }
    _woken = false;
    cerr << "arLogitechDriver remark: stopped.\n";   
  }
  return true;
}

bool arLogitechDriver::restart(){
  return stop() && start();
}

bool arLogitechDriver::_reset() {
  // Activate 6-D mode
  int stat = _comPort.ar_write( "*R" );
  if (stat < 2) {
    cerr << "arLogitechDriver error: wrote " << stat << " bytes instead of 2 in _reset().\n";
    return false;
  }
  // pause for > 1 second
  for (int i=0; i<12; i++)
    ar_usleep(100000);
  return true;
}

bool arLogitechDriver::_startStreaming() {
  int stat = _comPort.ar_write("*S");
  if (stat < 2) {
    cerr << "arLogitechDriver error: wrote " << stat << " bytes instead of 2 in _startStreaming().\n";
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
  int stat = _comPort.ar_write("*\05");
  if (stat < 2) {
    cerr << "arLogitechDriver error: wrote " << stat << " bytes instead of 2 in _runDiagnostics().\n";
    return false;
  }
  int bytesRead = _comPort.ar_read( _dataBuffer, 2 );
  if (bytesRead != 2) {
    cerr << "arLogitechDriver error: tracker failed to respond to the 'diagnostic' command\n"
         << "   (# bytes read = " << bytesRead << ").\n";
    return false;
  }
//  printf("%x %x\n",(int)_dataBuffer[0],_dataBuffer[1]);
//  cerr << (int)_dataBuffer[0] << ", " << (int)_dataBuffer[1] << endl;
  unsigned char aByte( ~_dataBuffer[0] );
  unsigned char bByte( ~_dataBuffer[1] );
  if ((aByte & 0xBF)||(bByte & 0x3F)) {
    cerr << "arLogitechDriver diagnostic failure:\n";
    if (aByte & DIAG_MOTHERBOARD) {
      cerr << "  Motherboard failed.\n";
    } 
    if (aByte & DIAG_CPU) {
      cerr << "  CPU failed.\n";
    } 
    if (aByte & DIAG_ROM) {
      cerr << "  ROM failed.\n";
    } 
    if (aByte & DIAG_RAM) {
      cerr << "  RAM failed.\n";
    } 
    if (aByte & DIAG_T_FRAME) {
      cerr << "  T-frame failed.\n";
    } 
    if (aByte & DIAG_MOUSE) {
      cerr << "  Mouse failed.\n";
    } 
    if (bByte & DIAG_COMM_PORT) {
      cerr << "  Comm. port failed.\n";
    } 
    if (bByte & DIAG_EEPROM) {
      cerr << "  EEPROM failed.\n";
    } 
    return false;
  }
  return true;
}

bool arLogitechDriver::_update() {
  int status;

  using namespace arLogitechDriverSpace;

  unsigned int numToRead = ELEMENT_SIZE - _charsInBuffer;
  int numRead = _comPort.ar_read( _dataBuffer+_charsInBuffer, numToRead, MAX_DATA );
  if (numRead < 0) {
    cerr << "arLogitechDriver error: read() failed in _update().\n";
    return false;
  }
  if (numRead == 0) {
    return true;
  }
  _charsInBuffer += numRead;
  unsigned int numElements = static_cast<unsigned int>(floor(_charsInBuffer/(float)ELEMENT_SIZE));
  if (numElements > 0) {
    unsigned int i;
    for (i=0; i<numElements; ++i) {
      _convertSendData( _dataBuffer+i*ELEMENT_SIZE );
    }
    unsigned int numUsed( numElements*ELEMENT_SIZE );
    unsigned int numLeft( _charsInBuffer - numUsed );
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
  const float INCHES_TO_FEET = 1./12.;
  long ax, ay, az;        // integer form of absolute translational data
  short arx, ary, arz;     // integer form of absolute rotational data

  // NOTE: the data-extraction code is lifted from the Logitech sample
  // collect unit's miscellaneous information
  //short buttons = (unsigned char) record[0] & (unsigned char) ~logitech_FLAGBIT;

  // gather the translational information
  // first sign extend if needed and then grab the rest of the information
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
  float x = INCHES_TO_FEET * ((float) ax) / 1000.0;
  float y = INCHES_TO_FEET * ((float) ay) / 1000.0;
  float z = INCHES_TO_FEET * ((float) az) / 1000.0;

  // gather the rotational information
  arx  = (record[10] & 0x7f) << 7;
  arx += (record[11] & 0x7f);
  
  ary  = (record[12] & 0x7f) << 7;
  ary += (record[13] & 0x7f);
  
  arz  = (record[14] & 0x7f) << 7;
  arz += (record[15] & 0x7f);

  // calculate the rotational floating point values
  float xAngle = ar_convertToRad( ((float) arx) / 40.0 );
  float yAngle = ar_convertToRad( ((float) ary) / 40.0 );
  float zAngle = ar_convertToRad( ((float) arz) / 40.0 );

  arMatrix4 matrix( ar_translationMatrix(x,y,z) *
     ar_rotationMatrix('y',yAngle)*ar_rotationMatrix('x',xAngle)*ar_rotationMatrix('z',zAngle)
     );
  sendMatrix( matrix );
}

