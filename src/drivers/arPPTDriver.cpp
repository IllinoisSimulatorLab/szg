//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************


// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#include "arDataUtilities.h"
#include "arPPTDriver.h"

//#define DEBUG

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arPPTDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

const unsigned int BUF_SIZE = 4096;

void ar_PPTDriverEventTask(void* theDriver) {
  cerr << "arPPTDriver remark: started event task.\n";
  arPPTDriver* pptDriver = (arPPTDriver*) theDriver;
  pptDriver->_stopped = false;
  pptDriver->_eventThreadRunning = true;
  while (!pptDriver->_stopped  && pptDriver->_eventThreadRunning) {
    pptDriver->_eventThreadRunning = pptDriver->_processInput();
  }
  pptDriver->_eventThreadRunning = false;
}

arPPTDriver::arPPTDriver() :
  _inited( false ),
  _imAlive( false ),
  _stopped( true ),
  _eventThreadRunning( false ),
  _inbuf(0) {
}

arPPTDriver::~arPPTDriver() {
  _port.ar_close();  // OK even if not open.
  if (_inbuf)
    delete[] _inbuf;
}

bool arPPTDriver::init(arSZGClient& SZGClient) {
  _inbuf = new char[BUF_SIZE];
  if (!_inbuf) {
    cerr << "arPPTDriver error: failed to allocate input buffer.\n";
    return false;
  }
  _portNum = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_PPT", "com_port"));
  _inited = true;
  // 0 buttons, 0 axes, 1 matrix
  _setDeviceElements( 0, 0, 1 );
  cerr << "arPPTDriver remark: initialized.\n";
  return true;
}

bool arPPTDriver::start() {
  if (!_inited) {
    cerr << "arPPTDriver::start() error: Not inited yet.\n";
    return false;
  }
  if (!_port.ar_open( _portNum, 115200, 8, 1, "none" )) {
    cerr << "arPPTDriver error: failed to open serial port #" << _portNum << endl;
    return false;
  }
  if (!_port.setReadTimeout( 2 )) {  // 200 msec
    cerr << "arPPTDriver error: failed to set timeout COM port.\n";
    return false;
  }
  _resetStatusTimer();
  return _eventThread.beginThread(ar_PPTDriverEventTask,this);
}

bool arPPTDriver::stop() {
  cerr << "arPPTDriver remark: stopping.\n";
  _stopped = true;
  while (_eventThreadRunning) {
    ar_usleep(10000);
  }
  cerr << "arPPTDriver remark: event thread exiting.\n";
  _port.ar_close();
  return true;
}

const unsigned int PPT_PACKET_SIZE(27);
const unsigned int PPT_MAX_LIGHTS(4);
const float PPT_RANGE(32768.0); 
const float METERS_PER_FOOT( 12.*.0254 );
const float FEET_PER_METER( 1./METERS_PER_FOOT );

bool arPPTDriver::_processInput() {
  int numRead = _port.ar_read( _inbuf, PPT_PACKET_SIZE, BUF_SIZE );
  if (numRead == 0) {
    if (_statusTimer.done() && _imAlive) {
      cerr << "arPPTDriver remark: lost communication with PPT.\n";
      _imAlive = false;
    }
    return true; 
  }
  if (!_imAlive) {
    cerr << "arPPTDriver remark: established communication with PPT.\n";
    _imAlive = true;
  }
  _resetStatusTimer();

  // Begin code lifted from WorldViz' VizPPTStreamingCode.h
  unsigned char packet[BUF_SIZE];
  memcpy( packet, _inbuf, numRead );
  int lastGoodIndex = -1;

  for(int i = 0; i < (int)numRead; ++i) {
    //If packet begins with 'o' and checksum matches and numlights is between 1 and 4 then assume it is a valid packet
    if(packet[i] == 'o' && i+PPT_PACKET_SIZE <= numRead && packet[i+PPT_PACKET_SIZE-1] == _checksum(&(packet[i]),PPT_PACKET_SIZE-1)
      && (int)packet[i+1] > 0 && (int)packet[i+1] <= PPT_MAX_LIGHTS) {
      lastGoodIndex = i;	
    }
  }

  if(lastGoodIndex != -1) {
    //If we found a fresh packet, unstuff data and return number of lights
    int numlights = (int)packet[1+lastGoodIndex];
    _setDeviceElements( 0, 0, numlights );
    
    for(int i = 0; i < numlights; i++) {
      float x = _unstuffBytes(10.0, &(packet[(i*6+2)+lastGoodIndex]) );
      float y = _unstuffBytes(10.0, &(packet[(i*6+4)+lastGoodIndex]) );
      float z = _unstuffBytes(10.0, &(packet[(i*6+6)+lastGoodIndex]) );

      // End lifted code
#ifdef DEBUG
      cout << x << "\t" << y << "\t" << -z << endl;
#endif
      // change coordinate systems from PPT's sensible left-handed to OpenGL-standard,
      // brainless right-handed, and from PPT's sensible meters to Syzygy's less-sensible
      // feet...
      queueMatrix( i, ar_translationMatrix( x*FEET_PER_METER, y*FEET_PER_METER,-z*FEET_PER_METER ) );
    }

    sendQueue();
  }

  return true;
}

// More code lifted from VizPPTStreamingCode.h
float arPPTDriver::_unstuffBytes(float max, const unsigned char *storage) {
  return (short int)(storage[0] * 256 + storage[1]) / PPT_RANGE * max;
}

unsigned char arPPTDriver::_checksum(const unsigned char* buffer, int length) const {
  unsigned char checksum = 0;
  for(int i = 0; i < length; i++)
    checksum += buffer[i];
  return checksum;
}
// End lifted code

const double PPT_TIMEOUT(5);

void arPPTDriver::_resetStatusTimer() {
  _statusTimer.start( PPT_TIMEOUT*1.e6 );
}
