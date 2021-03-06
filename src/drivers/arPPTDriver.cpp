//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPPTDriver.h"

#include <string>

DriverFactory(arPPTDriver, "arInputSource")

const unsigned int BUF_SIZE = 4096;

void ar_PPTDriverEventTask(void* theDriver) {
  ar_log_remark() << "arPPTDriver started event task.\n";
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
  _inbuf(0),
  _packetBuf(0),
  _packetOffset(0),
  _lastNumLights(0) {
}

arPPTDriver::~arPPTDriver() {
  _port.ar_close();  // OK even if not open.
  if (_inbuf)
    delete[] _inbuf;
  if (_packetBuf)
    delete[] _packetBuf;
}

bool arPPTDriver::init(arSZGClient& SZGClient) {
  _inbuf = new char[BUF_SIZE];
  if (!_inbuf) {
    ar_log_error() << "arPPTDriver failed to allocate input buffer.\n";
    return false;
  }
  _packetBuf = new unsigned char[BUF_SIZE];
  if (!_packetBuf) {
    ar_log_error() << "arPPTDriver failed to allocate packet buffer.\n";
    return false;
  }
  _portNum = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_PPT", "com_port"));
  _inited = true;
  _setDeviceElements( 0, 0, 1 );
  ar_log_remark() << "arPPTDriver inited.\n";
  return true;
}

bool arPPTDriver::start() {
  if (!_inited) {
    ar_log_error() << "arPPTDriver can't start before init.\n";
    return false;
  }
  if (!_port.ar_open( _portNum, 115200, 8, 1, "none" )) {
    ar_log_error() << "arPPTDriver failed to open serial port #" << _portNum << ".\n";
    return false;
  }
  if (!_port.setReadTimeout( 2 )) {  // 200 msec
    ar_log_error() << "arPPTDriver failed to set timeout on COM port.\n";
    return false;
  }
  _resetStatusTimer();
  return _eventThread.beginThread(ar_PPTDriverEventTask, this);
}

bool arPPTDriver::stop() {
  ar_log_debug() << "arPPTDriver stopping.\n";
  _stopped = true;
  arSleepBackoff a(5, 20, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  ar_log_debug() << "arPPTDriver event thread done.\n";
  _port.ar_close();
  return true;
}

const unsigned PPT_PACKET_SIZE = 27;
const int PPT_MAX_LIGHTS = 4;
const float PPT_RANGE = 32768.0;
const float METERS_PER_FOOT = 12.0 * 0.0254;
const float FEET_PER_METER = 1./METERS_PER_FOOT;

bool arPPTDriver::_processInput() {
  unsigned int numRead = _port.ar_read( _inbuf, PPT_PACKET_SIZE, BUF_SIZE );
  if (numRead == 0) {
    if (_statusTimer.done() && _imAlive) {
      ar_log_error() << "arPPTDriver lost PPT.\n";
      _imAlive = false;
    }
    return true; 
  }
  if (!_imAlive) {
    ar_log_remark() << "arPPTDriver found PPT.\n";
    _imAlive = true;
  }
  _resetStatusTimer();

  // Begin code lifted from WorldViz' VizPPTStreamingCode.h
  memcpy( _packetBuf+_packetOffset, _inbuf, numRead );
  _packetOffset += numRead;
  int lastGoodIndex = -1;
  unsigned int i;
  while (_packetOffset >= PPT_PACKET_SIZE) {
    for(i=0; i<_packetOffset; ++i) {
      //If packet begins with 'o' and checksum matches and numlights is between 1 and 4 then assume it is a valid packet
      if (_packetBuf[i] == 'o' && i+PPT_PACKET_SIZE <= _packetOffset && _packetBuf[i+PPT_PACKET_SIZE-1] == _checksum(&(_packetBuf[i]), PPT_PACKET_SIZE-1)
        && (int)_packetBuf[i+1] > 0 && (int)_packetBuf[i+1] <= PPT_MAX_LIGHTS) {
        lastGoodIndex = i;        
      }
    }

    if (lastGoodIndex == -1) {
      ar_log_error() << "Bad PPT packet.\n";
    } else {
      // Found a fresh packet.  Unstuff data and return number of lights.
      const int unsigned numlights = _packetBuf[1+lastGoodIndex];
      if (numlights != _lastNumLights) {
        _setDeviceElements( 0, 0, numlights );
        _lastNumLights = numlights;
      }
      
      for(i=0; i<numlights; ++i) {
        const float x = _unstuffBytes(10.0, &(_packetBuf[(i*6+2)+lastGoodIndex]) );
        const float y = _unstuffBytes(10.0, &(_packetBuf[(i*6+4)+lastGoodIndex]) );
        const float z = _unstuffBytes(10.0, &(_packetBuf[(i*6+6)+lastGoodIndex]) );
        // End lifted code

        // change coordinate systems from PPT's left-handed to OpenGL's
        // right-handed, and from PPT's meters to Syzygy's feet.
        queueMatrix( i, ar_translationMatrix( x*FEET_PER_METER, y*FEET_PER_METER, -z*FEET_PER_METER ) );
      }

      for (i=lastGoodIndex+PPT_PACKET_SIZE; i<_packetOffset; ++i) {
        _packetBuf[i-(lastGoodIndex+PPT_PACKET_SIZE)] = _packetBuf[i];
      }
      _packetOffset -= (lastGoodIndex+PPT_PACKET_SIZE);
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

void arPPTDriver::_resetStatusTimer() {
  const double PPT_TIMEOUT = 5.;
  _statusTimer.start( PPT_TIMEOUT*1.e6 );
}
