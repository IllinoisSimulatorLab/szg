/*
todo: in here? or downstream?
  TransmitterOffset 0.06 10.5 -4.8 feet
# left side of crystaleyes glasses:
  HeadSensorRotation 0 0 1 -90 
  HeadSensorOffset 3.5 0 -2.5 inches
  Wand... ditto.
*/

//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFOBDriver.h"

const bool fInTheCave = true;;;;

DriverFactory(arFOBDriver, "arInputSource")

arFOBDriver::arFOBDriver() :
  _timeoutTenths( 30 ),
  _dataSize( 7 ),
  _dataBuffer( NULL ),
  _positionScale( 3./32768.0 ),
  _orientScale( 1./32768.0 ),
  _eventThreadRunning(false),
  _stopped(false)
{}

arFOBDriver::~arFOBDriver() {
  if (_dataBuffer)
    delete[] _dataBuffer;
}

bool arFOBDriver::init(arSZGClient& SZGClient){

  /*
  Syzygy's default frame of reference differs from Ascension's.
  When the bird's flat surface points down and the cable points back,
  then the rotation matrix should be the identity in Syzygy coords.
  So the coordinate frame attached to the bird has
  +z going back along the cable, +x right, and +y up.

  Read in the calibration information. This base transformation corrects
  for an offset in the transmitter's position and orientation, letting
  us report a given bird position/orientation as some arbitrary value.
  Syzygy matrices are in OpenGL order (down the columns, not along the rows).

  In practice, place the bird at a known location and with identity orientation
  (as described above) relative to your physical coordinate system.
  You can then compute the matrix needed to align it.

  The base transformation is post-multiplied with the supplied data. 

  This rough calibration should be followed with a finer one.
  */

  // Bug: this reports WARNING, even from "dex vccave cubevars -szg log=DEBUG".
  // Why doesn't DEBUG propagate to here?
  // cerr << ar_log().logLevel() << " propagation test.\n";
  ar_log().setLogLevel(AR_LOG_DEBUG);

#if 0
  Filter that works in Beckman CAVE:

  <pforth>
  matrix Xin
  matrix Xout
  matrix C1
  matrix C2
  matrix originOffset
  matrix headRotMatrix
  matrix wandRotMatrix
  matrix Ry
  matrix Rz

  1  0  0  0
  0  0 -1  0
  0  1  0  0
  0  0  0  1
  C1 matrixStoreTranspose
  1  0  0  0
  0  0  1  0
  0 -1  0  0
  0  0  0  1
  C2 matrixStoreTranspose

  -90 zaxis Rz rotationMatrix
  -90 yaxis Ry rotationMatrix
  Ry Rz headRotMatrix matrixMultiply
  -90 yaxis wandRotMatrix rotationMatrix

  0.7 10.0 -4.0 originOffset translationMatrix

  define filter_matrix_0
  Xin getCurrentEventMatrix
  originOffset C1 Xin C2 headRotMatrix 5 Xout concatMatrices
  Xout setCurrentEventMatrix
  enddef

  define filter_matrix_1
  Xin getCurrentEventMatrix
  originOffset C1 Xin C2 wandRotMatrix 5 Xout concatMatrices
  Xout setCurrentEventMatrix
  enddef

  define filter_matrix_2
  Xin getCurrentEventMatrix
  originOffset C1 Xin C2 wandRotMatrix 5 Xout concatMatrices
  Xout setCurrentEventMatrix
  enddef
  </pforth>

#endif

  int i = 0;

  // Determine the baud rate.
  const int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
  bool baudRateFound = false;
  int baudRate = SZGClient.getAttributeInt("SZG_FOB", "baud_rate");
  if (baudRate == 0)
    goto LDefaultBaudRate;
  for (i=0; i<_nBaudRates; i++){
    if (baudRate == baudRates[i]) {
      baudRateFound = true;
      break;
    }
  }
  if (!baudRateFound) {
    cerr << "arFOBDriver got unexpected SZG_FOB/baud_rate "
	 << baudRate << ".\n  Expected one of:";
    for (i=0; i<_nBaudRates; i++)
      cerr << " " << baudRates[i];
    cerr << "\n";
LDefaultBaudRate:
    baudRate = 115200;
    cerr << "arFOBDriver SZG_FOB/baud_rate defaulting to "
         << baudRate << ".\n";
  }

  // Multiple serial ports aren't implemented.
  _comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_FOB", "com_port"));
  if (_comPortID <= 0) {
    ar_log_warning() << "arFOBDriver: SZG_FOB/com_port defaulting to 1.\n";
    _comPortID = 1;
  }
  if (!_comPort.ar_open(_comPortID,(unsigned long)baudRate,8,1,"none" )){
    ar_log_warning() << "arFOBDriver failed to open serial port " <<
      _comPortID << ".\n";
    return false;
  }

  if (!_comPort.setReadTimeout(_timeoutTenths)){
    ar_log_warning() << "arFOBDriver failed to set timeout for serial port " <<
      _comPortID << ".\n";
    return false;
  }

  /*
  Get the FoB's "description", a string of slash-delimited integers
  of length equal to the number of FOB units.  Each unit is one of:
    0: bird
    1: transmitter
    2: transmitter AND bird
    3: extended range transmitter
    4: extended range transmitter AND bird
  */
  char receivedBuffer[512];
  int birdConfiguration[_FOB_MAX_DEVICES+1];
  const string received(SZGClient.getAttribute("SZG_FOB", "config"));
  if (received == "NULL") {
    ar_log_warning() << "arFOBDriver: no SZG_FOB/config.\n";
    return false;
  }
  ar_stringToBuffer(received, receivedBuffer, sizeof(receivedBuffer));

  if (fInTheCave) {
    if (received != "0/3/0/0") {
      ar_log_warning() << "SZG_FOB/config should be 0/3/0/0.  Overriding.\n";
      strcpy(receivedBuffer, "0/3/0/0");
      // todo: trust the flock's own status report, and eliminate SZG_FOB/config.
    }
  }

  _numFlockUnits = ar_parseIntString(receivedBuffer, birdConfiguration+1, _FOB_MAX_DEVICES );
  if (_numFlockUnits < 1) {
    ar_log_warning() << "arFOBDriver: SZG_FOB/config needs one entry per bird and ERC.\n";
    return false;
  }

  /*
  Configure.  _sensorMap, indexed by flock ID,
  indicates Syzygy ID, or -1 for no sensor.
  Example: SZG_FOB/config = "0/1/0/0" produces
    _sensorMap[0] = 0;
    _sensorMap[1] = -1;
    _sensorMap[2] = 1;
    _sensorMap[3] = 2;
  */

  _numBirds = 0;
  _transmitterID = 0;
  _extendedRange = false;
  // flock addresses start at 1
  for (int addr=1; addr<=_numFlockUnits; ++addr){
    switch (birdConfiguration[addr]) {
    case 1:
    case 2:
    case 3:
    case 4:
      if (_transmitterID>0){
        // The configuration already included a transmitter.
	ar_log_warning() << "arFOBDriver: more than one transmitter in SZG_FOB/config.\n";
	return false;
      }
    }
    switch (birdConfiguration[addr]) {
    default:
      ar_log_warning() << "arFOBDriver: illegal configuration value.\n";
      return false;
    case 0: // Bird
      _sensorMap[addr] = _numBirds++;
      break;
    case 1: // Transmitter
      _sensorMap[addr] = -1;
      _transmitterID = addr;
      break;
    case 2: // Transmitter and bird
      _sensorMap[addr] = _numBirds++;
      _transmitterID = addr;
      break;
    case 3: // Extended range transmitter
      _sensorMap[addr] = -1;
      _transmitterID = addr;
      _extendedRange = true;
      break;
    case 4: // Extended range transmitter and bird
      _sensorMap[addr] = _numBirds++;
      _transmitterID = addr;
      _extendedRange = true;
      break;
    }
  }
  if (!_transmitterID){
    ar_log_warning() << "arFOBDriver: no transmitter in SZG_FOB/config.\n";
    return false;
  }

  ar_log_remark() << "arFOBDriver expecting " << _numBirds <<
    " birds, " << (_extendedRange ? "ERT" : "transmitter") <<
    " at unit " << _transmitterID << ".\n";

  // Report one matrix per bird.
  _setDeviceElements( 0, 0, _numBirds );

  // Configure the FoB.  (Does it need waking now?)

  {
    unsigned char b[2];
    if (_getFOBParam(1, b, 2, 0) != 2) {
      ar_log_warning() << "arFoBDriver: Flock unresponsive.  Unplugged?  Unpowered?  Wrong SZG_FOB/baud_rate?\n";
      return false;
    }
    ar_log_debug() << "FoB version " << int(b[0]) << "." << int(b[1]) << ".\n";
    ar_usleep(100000);
  }

  if (fInTheCave) {
    if (!_autoConfig()) {
      ar_log_warning() << "arFOBDriver failed to autoconfig.\n";
      return false;
    }

    // 4f 24, FoB manual p.130.
    unsigned char c[14];
    if (_getFOBParam(0x24, c, 14, 0) != 14) {
      ar_log_warning() << "arFOBDriver failed to examine system status (try powercycling the flock).\n";
      return false;
    }

    // SZG_FOB/config should be 0/3/0/0 since this reports e0 d1 e0 a0 0...
    for (int i=0; i<14; ++i) {
      const unsigned t = c[i];
      if (t == 0)
	continue;
      cerr << "  FoB addr " << i+1;
      if (t & 0x80)
	cerr << ", fly";
      if (t & 0x40)
	cerr << ", run";
      if (t & 0x20)
	cerr << ", bird";
      if (t & 0x10)
	cerr << ", extended range";
      if (t & 0x08)
	cerr << ", transmitter #3";
      if (t & 0x04)
	cerr << ", transmitter #2";
      if (t & 0x02)
	cerr << ", transmitter #1";
      if (t & 0x01)
	cerr << ", transmitter #0";
      cerr << ".\n";
    }

    ar_usleep(100000);
  }

  // Set all birds' data mode.
  if (_numBirds > 1){
    for (i=1; i<=_numFlockUnits; i++){
      if (_sensorMap[i] >= 0 && !_setDataMode(i)) {
	ar_log_warning() << "arFOBDriver failed to set data mode of bird on unit " << i << ".\n";
	return false;
      }
    }
  }
  else{
    if (!_setDataMode(0)){
      ar_log_warning() << "arFOBDriver failed to set data mode.\n";
      return false;
    }

#ifdef AR_USE_LINUX
    ar_usleep(500000); // At least half a second.
#endif
  }

#if 0
  {
    unsigned char b[2];
    if (_getFOBParam(10, b, 1, 0) != 1)
      ar_log_warning() << "arFOBDriver failed to get error code.\n";
    else
      ar_log_debug() << "error code is " << int(b[0]) << ".\n";
    ar_usleep(100000);
  }
#endif

  // Set each bird's hemisphere.
  const string hemisphere(SZGClient.getAttribute("SZG_FOB","hemisphere"));
  if (hemisphere == "NULL") {
    ar_log_warning() << "arFOBDriver: SZG_FOB/hemisphere undefined.\n";
    return false;
  }

  if (_numBirds > 1){
    for (i=1; i<=_numFlockUnits; i++){
      if (_sensorMap[i] >= 0 && !_setHemisphere(hemisphere, i)) {
        if (!_setHemisphere(hemisphere, i)){
	  ar_log_warning() << "arFOBDriver failed to set hemisphere.\n";
	  return false;
	}
      }
    }
  }
  else{
    if (!_setHemisphere(hemisphere, 0)){
      ar_log_warning() << "arFOBDriver failed to set hemisphere.\n";
      return false;
    }
  }

#ifdef AR_USE_LINUX
  {
    // Timing tweak.
    string foo;
    (void)_getDataMode(foo, 0);
  }
#endif

  // If transmitter isn't the default (flock unit 1), change to the correct one.
  if (_transmitterID != 1)
    _nextTransmitter(_transmitterID);

  // ERT _getPositionScale() may return 36 which is wrong.  Hardcode it instead.
  if (_extendedRange) {
    _positionScale = 12/32768.0; // Convert to feet.
    ar_log_debug() << "arFOBDriver: FOB's scale forced to 12 feet by expected ERT.\n";
  }
  else {
    bool longRange = false;
    if (!_getPositionScale( longRange ))
      return false;
    ar_log_remark() << "arFOBDriver: FOB's scale is "
         << (longRange ? 72 : 36) << " inches.\n";
  }

  // Create a buffer for reading in data.
  // Two bytes per number.
  // (NYI: plus a bird address if there's more than one bird and in group mode).
  const unsigned bytesPerBird = 2*_dataSize;
  _dataBuffer = new unsigned char[bytesPerBird /* if in group mode: *_numBirds */ ];

  // Iff "in flock mode" (more than one unit),
  // send an auto_config command to get the data moving.
  if (_numBirds > 1){
    if (!_autoConfig()) {
      ar_log_warning() << "arFOBDriver failed to autoconfigure.\n";
      return false;
    }
    ar_log_debug() << "arFOBDriver autoconfigured.\n";
  }
  else{
    // The run command might do nothing to a flock,
    // but it wakes up a standalone unit.
    ar_log_debug() << "arFOBDriver run.\n";
    if (!_run()){
      ar_log_warning() << "arFOBDriver failed to run.\n";
      return false;
    }
#ifdef AR_USE_LINUX
    _setDataMode(1); // Timing tweak.
#endif
  }

  // Flock configured. All the FoB's lights should be on solid.
  return true;
}

bool arFOBDriver::start() {
  return _eventThread.beginThread(ar_FOBDriverEventTask,this);
}

void ar_FOBDriverEventTask(void* FOBDriver){
  ((arFOBDriver*)FOBDriver)->_eventloop();
}

void arFOBDriver::_eventloop(){
  _eventThreadRunning = true;
  for (;;) {
    // Reverse loop, so arInputEventQueue's signature
    // is updated all at once, not one matrix at a time.
    for (int i=_numFlockUnits; i>=1; --i){
      if (_sensorMap[i] < 0)
        continue;
      // This flock unit has a bird.  Query it.
      if (!_getSendNextFrame(i)){
	ar_log_warning() << "arFOBDriver got no data from Flock.\n";
	_stopped = true;
      }
      if (_stopped) {
	_eventThreadRunning = false;
	stop();
	return;
      }
    }
    sendQueue();
  }
}

bool arFOBDriver::stop() {
  _stopped = true;
  arSleepBackoff a(10, 30, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  (void)_sleep();
  _comPort.ar_close();
  ar_log_debug() << "arFOBDriver stopped.\n";
  return true;
}

bool arFOBDriver::_setHemisphere( const std::string& hemisphere,
                                  unsigned char addr) {
  unsigned char cdata[] = {'L', 0, 0};
  // 2 data bytes are HEMI_AXIS and then HEMI_SIGN
  if (hemisphere == "front") {
    cdata[1] = 0;
    cdata[2] = 0;
  } else if (hemisphere == "rear") {
    cdata[1] = 0;
    cdata[2] = 1;
  } else if (hemisphere == "upper") {
    cdata[1] = 0xc;
    cdata[2] = 1;
  } else if (hemisphere == "lower") {
    cdata[1] = 0xc;
    cdata[2] = 0;
  } else if (hemisphere == "left") {
    cdata[1] = 6;
    cdata[2] = 1;
  } else if (hemisphere == "right") {
    cdata[1] = 6;
    cdata[2] = 0;
  } else {  
    ar_log_warning() << "arFOBDriver: invalid hemisphere '" << hemisphere << "'.\n";
    return false;
  }
  if (!_sendBirdAddress(addr))
    return false;
  const bool ok = _sendBirdCommand( cdata, 3 );
  ar_usleep(100000);
  return ok;
}

// Older FoBs might not support _getHemisphere.
bool arFOBDriver::_getHemisphere( std::string& hemisphere, unsigned char addr ) {
  unsigned char rgb[2];
  if (_getFOBParam( 22, rgb, 2, addr ) != 2) {
    ar_log_warning() << "arFOBDriver failed to _getFOBParam(hemisphere).\n";
    return false;
  }
  if (rgb[0] == 0 && rgb[1] == 0) {
    hemisphere = "front";
  } else if (rgb[0] == 0 && rgb[1] == 1) {
    hemisphere = "rear";
  } else if (rgb[0] == 0xc && rgb[1] == 1) {
    hemisphere = "upper";
  } else if (rgb[0] == 0xc && rgb[1] == 0) {
    hemisphere = "lower";
  } else if (rgb[0] == 6 && rgb[1] == 1) {
    hemisphere = "left";
  } else if (rgb[0] == 6 && rgb[1] == 0) {
    hemisphere = "right";
  } else {
    hemisphere = "unknown";
    return false;
  }
  return true;
}

// Set bird's data mode to position+quaternion
bool arFOBDriver::_setDataMode(unsigned char addr) {
  return _sendBirdAddress(addr) && _sendBirdByte(']'); // FoB manual p.92
}

bool arFOBDriver::_getDataMode( std::string& modeString, unsigned char addr ) {
  unsigned char buf[2];
  if (_getFOBParam( 0, buf, 2, addr ) != 2) {
    ar_log_warning() << "arFOBDriver failed to _getFOBParam(datamode).\n";
    return false;
  }

  // FoB manual p.112
  const unsigned dataMode = (buf[0] >> 1) & 0xf;

#if 0
  printf("\taddr=%d, 2bytes = %x %x\n\t", int(addr), (unsigned)buf[0], (unsigned)buf[1]);
  const unsigned b0  = (buf[0]     ) & 1;
  const unsigned b5  = (buf[0] >> 5) & 1;
  const unsigned b6  = (buf[0] >> 6) & 1;
  const unsigned b7  = (buf[0] >> 7) & 1;
  const unsigned b8  = (buf[1]     ) & 1;
  const unsigned b9  = (buf[1] >> 1) & 1;
  const unsigned b10 = (buf[1] >> 2) & 1;
  const unsigned b11 = (buf[1] >> 3) & 1;
  const unsigned b12 = (buf[1] >> 4) & 1;
  const unsigned b13 = (buf[1] >> 5) & 1;
  const unsigned b14 = (buf[1] >> 6) & 1;
  const unsigned b15 = (buf[1] >> 7) & 1;
  cout << (b15 ? "master. " : "slave. ")
    << (b0 ? "point mode. " : "stream mode. ")
    << (b5 ? "sleep !run. " : "")
    << (b12 ? "" : "sleep !run. ")
    << (b6 ? "xoff. " : "")
    << (b7 ? "factory test. " : "")
    << (b8 ? "" : "sync mode. ")
    << (b9 ? "crtsync. " : "")
    << (b10 ? "expandedAddr. " : "")
    << (b11 ? "host sync. ": "")
    << (b13 ? "error. " : "")
    << (b14 ? "" : "!inited/autoconfd. ")
    << "\n";
#endif

  bool ok = true;
  switch (dataMode) {
    case 1:
      modeString = "position";
      break;
    case 2:
      modeString = "angle";
      break;
    case 3:
      modeString = "matrix";
      break;
    case 4:
      modeString = "position+angle";
      break;
    case 5:
      modeString = "position+matrix";
      break;
    case 6:
      modeString = "factory use only";
      ok = false;
      break;
    case 7:
      modeString = "quaternion";
      break;
    case 8:
      modeString = "position+quaternion";
      break;
    default:
      modeString = "invalid";
      ok = false;
      break;
  }
  if (!ok)
    ar_log_warning() << "arFOBDriver mode is " << dataMode << " aka '" << modeString << "'.\n";
  return ok;
}

bool arFOBDriver::_setPositionScale( bool longRange,
                                     unsigned char addr ) {
  const unsigned short scale = longRange ? 1 : 0;
  if (!_setFOBParam( 3, (unsigned char *)&scale, 2, addr )) {
    ar_log_warning() << "arFOBDriver failed to _setFOBParam.\n";
    return false;
  }
  _positionScale = longRange ? 6./32768. : 3./32768.;
  return true;
}

bool arFOBDriver::_getPositionScale( bool& longRange,
                                     unsigned char addr ) {
  unsigned char outdata[2];
  if (_getFOBParam( 3, outdata, 2, addr ) != 2) {
    ar_log_warning() << "arFOBDriver failed to _getFOBParam(position scale).\n";
    return false;
  }
  const unsigned short bigScale = *((unsigned short*)outdata);
  switch (bigScale) {
    case 0:
      _positionScale = 3./32768.0; // convert to feet.
      longRange = false;
      break;
    case 1:
      _positionScale = 6./32768.0; // convert to feet.
      longRange = true;
      break;
    default:
      _positionScale = 3./32768.0; // convert to feet.
      longRange = false;
      ar_log_warning() << "arFOBDriver: positionScale data = " << bigScale << ".\n";
      return false;
  }
  return true;
}

int arFOBDriver::_getFOBParam( const unsigned char paramNum,
                               unsigned char* buf, 
                               unsigned int numBytes,
                               unsigned char addr ) {

  if (!_sendBirdAddress(addr))
    return 0;
  const unsigned char cdata[2] = {'O', paramNum};
  if (!_sendBirdCommand( cdata, 2 )) {
    ar_log_warning() << "arFOBDriver failed to send 'get param'.\n";
    return 0;
  }
  ar_usleep( 10000 );
  return _getBirdData( buf, numBytes );
}

bool arFOBDriver::_setFOBParam( const unsigned char paramNum,
                                const unsigned char* buf,
                                const unsigned int numBytes,
                                unsigned char addr ) {
  if (!_sendBirdAddress(addr))
    return false;
  unsigned char* cdata = new unsigned char[2+numBytes];
  if (!cdata) {
    ar_log_warning() << "arFOBDriver out of memory.\n";
    return false;
  }
  cdata[0] = 'P'; // FoB manual pp.73, 110
  cdata[1] = paramNum;
  memcpy( cdata+2, buf, numBytes );
  bool ok = _sendBirdCommand(cdata, 2+numBytes);
  if (!ok)
    ar_log_warning() << "arFOBDriver failed to send 'set param'.\n";
  delete [] cdata;
  return ok;
}

bool arFOBDriver::_sendBirdByte(unsigned char c, bool fSleep){
  const bool ok = _sendBirdCommand(&c, 1);
  if (fSleep)
    ar_usleep(1000000);
  return ok;
}

bool arFOBDriver::_sleep(){
  return _sendBirdByte('G');
}

bool arFOBDriver::_run(){
  return _sendBirdByte('F');
}

bool arFOBDriver::_autoConfig() {
  // FoB manual p.132: wait 600 msec before and after this command.
  ar_log_debug() << "arFOBDriver::_autoConfig " << _numFlockUnits << " units.\n";
  ar_usleep( 700000 );
  const unsigned char cdata[] = {'P', 0x32, (unsigned char)_numFlockUnits };
  const bool ok = _sendBirdCommand(cdata, 3);
  ar_usleep( 700000 );  
  return ok;
}

bool arFOBDriver::_nextTransmitter(unsigned char addr){
  // The address of the transmitter unit goes in the most significant
  // half of the byte. The transmitter number goes in the lower
  // half. We are assuming that this is 0.
  const unsigned char cdata[] = {'0', (addr << 4) };
  const bool ok = _sendBirdCommand(cdata, 2);
  ar_usleep( 1000000 );  
  return ok;
}

bool arFOBDriver::_sendBirdAddress( unsigned char addr ) {
  if (addr <= 1) {
    // 0 means no bird or the unique bird.
    // 1 means the first (and only) bird, when sent to _getSendNextFrame().
    return true;
  }
  if (_stopped)
      return false;
  if (_numBirds <= 1) {
    ar_log_warning() << "arFOBDriver expected more than one bird.\n";
    return true;
  }
  const unsigned char cdata = 0xF0 | addr;
  const bool ok = _comPort.ar_write((char*)&cdata, 1) == 1;
  if (!ok)
    ar_log_warning() << "arFOBDriver failed to _sendBirdAddress.\n";
  return ok;
}

bool arFOBDriver::_sendBirdCommand( const unsigned char* cdata, 
                                    const unsigned int numBytes ) {
  if (_stopped)
    return false;

  // printf("_sendBirdCommand (%c)   ", *cdata);
  // for (unsigned i=0; i<numBytes; ++i)
  //   printf("%02x ", cdata[i]);
  // printf("\n");

  return _comPort.ar_write( (char*)cdata, numBytes ) ==
    static_cast<int>(numBytes);
}

int arFOBDriver::_getBirdData( unsigned char* cdata, 
                               const unsigned int numBytes ) {
  if (_stopped)
      return false;
  return _comPort.ar_read((char *)cdata, numBytes);
}

bool arFOBDriver::_getSendNextFrame(const unsigned char addr) {
  if (!_sendBirdAddress(addr))
    return false;

  if (!_sendBirdByte('B', false)) {
    ar_log_warning() << "arFOBDriver failed to send 'point' command.\n";
    return false;
  }

  const unsigned bytesPerBird = 2*_dataSize;
  if (_stopped)
    return false;
  const unsigned bytesRead = _comPort.ar_read((char*)_dataBuffer, bytesPerBird);
  if (bytesRead != bytesPerBird) {
    ar_log_warning() << "arFOBDriver read only " << bytesRead <<
      " of " << bytesPerBird << " bytes.\n";
    return false;
  }

// FoB manual p.35: FoBs 14-byte data record, the response to a Point command.
//   Position/orientation data.

  static const unsigned char zeros[80] = {0};
  if (!memcmp(_dataBuffer, zeros, _dataSize*2)) {
    cerr << "arFOBDriver ignoring zeros from bird " << int(addr) << " (not running?).\n";
    return true;
  }

  unsigned j;

  unsigned char* pb = (unsigned char*)_dataBuffer;
  if (*pb & 0x80 == 0)
    cerr << "FoB phasing error at start of frame.\n";
  *pb &= 0x7f; // clear phasing bit

#if 0
  if (addr == 1)
    printf("\t\t\t____ xyz %02x%02x %02x%02x %02x%02x    Quat %02x%02x %02x%02x %02x%02x %02x%02x\n",
	pb[1], pb[0], pb[3], pb[2], pb[5], pb[4], pb[7], pb[6], pb[9], pb[8], pb[11], pb[10], pb[13], pb[12]);
#endif

  for (j=0; j<_dataSize*2; j+=2) {
    const unsigned lsb = pb[j];
    const unsigned msb = pb[j+1];
    // printf("lsb msb:  %02x %02x\n", lsb, msb);
    if ((lsb & 0x80) != 0) {
      cerr << "FoB phasing error during byte " << j << ".\n";
      return true;
    }
    if ((msb & 0x80) != 0) {
      cerr << "FoB phasing error during byte " << j+1 << ".\n";
      return true;
    }
    // convert to signed
    int s = ((msb << 7) | lsb) << 2;
    if (s >= 32768)
      s -= 65536;
    _floatData[j/2] = float(s);
  }
  //printf("____\n");

  // FoB manual p.89: scale xyz (signed int * S) / 32768,
  //     where S is 36 72 or 144, output is in inches.
  // FoB manual p.93: scale quaternions +.99996 = 0x7fff, 0=0, -1.0=0x8000.

  // The rotation matrix of the bird. SpacePad, and maybe other Ascension
  // products, returns the INVERSE (= transpose) of the bird's rotation matrix.
  //
  // Specific to position+quaternion.
  for (j=3; j<_dataSize; ++j)
    _floatData[j] *= _orientScale;

  const arMatrix4 rotMatrix(arQuaternion( _floatData + 3));

  // The bird's translation, scaled appropriately.
  const arMatrix4 translationMatrix = 
    ar_translationMatrix( _positionScale * arVector3(_floatData));

  /*
  How to compute the matrix reported to the input device.
  Preferably in pforth, not C++.

  Call Syzygy's coordinate frame B1, the Flock's B2.
  In the context of VR, B1 is the frame of the graphics.
  We need a transformation matrix in B1.

  Let M be the rotation taking B1 to B2.
  If a point has coordinates P1 w.r.t B1, and P2 w.r.t. B2,
  then (! is inverse):
    P1 =  M * P2
    P2 = !M * P1

  M maps B2-coords to B1-coords, !M the opposite.
  Let R2 be a rotation in B2.  Then R2 in B1-coords is:
    M * R2 * !M

  M*T changes a translation T in B2 to one in B1.
  If the transmitter's reference frame is as above (w.r.t. B1),
  then the matrix (of M*T w.r.t B1?) is:
    M * T * R2 * !M

  Consider sensor rotation. The sensor is attached to a bird.
  The bird has a NEUTRAL physical orientation relative to the transmitter
  (the orientation whose rotation matrix is the identity).
  Let N be the rotation in B1 that takes the sensor from its orientation
  on the bird to the sensor's neutral position.
  Then the bird should report a rotation of !N when in its neutral position.
  Thus, the orientation and position of the bird is:
    (M * T * R2 * !M) * !N

  Consider transmitter rotation and translation offset.
  Let T1 translate Syzygy's origin to the flock's origin.
  Let R3 be the transmitter's rotation from its neutral position
  (relative to the sensor's neutral position) in B1.
  The orientation of the sensor (the interaction device) is:
    T1 * R3 * (M * T * R2 * !M * !N)
  
  This does not account for sensor translation offset.

  The 180 degree rotation about y accounts for the fact that
  the bird's neutral position is flat surface down and tail forward,
  while Syzygy assumes flat surface down and tail back.
  */

  queueMatrix(_sensorMap[addr],
    translationMatrix *
    !rotMatrix 
//  ar_rotationMatrix('y', M_PI)
    );

  return true;
}

#if 0
// KEEP THIS EXAMPLE UNTIL WE FULLY HANDLE GROUP MODE.
bool arFOBDriver::_getSendNextFrame() {
  if (!_configured) {
    ar_log_warning() << "arFOBDriver tried to get data before init.\n";
    return false;
  }
  const unsigned bytesPerBird = 2*_dataSize + (_numBirds>1 ? 1 : 0);
  int numBytes = bytesPerBird;
  int bytesRead = -1;
  int i = 0;
  int j = 0;
  if (_stopped)
    return false;
  if (_numComports == 1) {
    numBytes *= _numBirds;
    bytesRead = _comPorts[0].ar_read( (char*)_dataBuffer, numBytes );
    if (bytesRead != numBytes) {
      ar_log_warning() << "arFOBDriver read " << bytesRead <<
        " of " << numBytes << " bytes.\n";
      stop();
      return false;
    }
  } else {
    for (i=0; i<_numComports; i++) {
      bytesRead = _comPorts[i].ar_read( (char*)_dataBuffer+i*numBytes, numBytes );
      if (bytesRead != numBytes) {
	ar_log_warning() << "arFOBDriver read " << bytesRead <<
	  " of " << numBytes << " bytes from bird # " << i << ".\n";
        stop();
        return false;
      }
    }
  }
  short* birdData = NULL;
  // Convert & queue Bird data.
  for (i=0; i<_numBirds; i++) {
    birdData = (short*) _dataBuffer + i*bytesPerBird;
    for (j=0; j<_dataSize; j++) {
      // Copied & pasted from Ascension code CMDUTIL.C
  	  *birdData = (short)((((short)(*(unsigned char *) birdData) & 0x7F) |
				(short)(*((unsigned char *) birdData+1)) << 7)) << 2;
      _floatData[j] = (float)*birdData++;
    }
    // specific to position+quaternion data mode!!!
    for (j=3; j<7; j++)
      _floatData[j] *= _orientScale;
    // Addresses go from 1 to _numBirds; matrices are one less.
    const int birdAddress = (_numBirds == 1) ? 0 :
      *(_dataBuffer + (i+1)*bytesPerBird - 1) - 1;
    if ((birdAddress < 0)||(birdAddress >= _numBirds)) {
      ar_log_warning() << "arFOBDriver: out-of-range bird address " <<
        birdAddress << ".\n";
      stop();
      return false;
    }
 
    // FOB uses a different reference frame than syzygy, so we use the switch
    // matrix and coord matrix to convert between frames, see pg 68 of the FOB
    // manual for a diagram
    // TODO: see if we could use the FOB REFERENCE FRAME1/FRAME2 commands to 
    // achieve the same affect (pg 94)
    static const arMatrix4 switchMatrix( 1,  0, 0, 0,
                                         0,  0, 1, 0,
                                         0, -1, 0, 0,
                                         0,  0, 0, 1 );
                                         
    static const arMatrix4 invSwitchMatrix = !switchMatrix;
    static const arMatrix4 coordMatrix  = ar_rotationMatrix( 'y', ar_convertToRad( -90 ) );
    
    // translations and rotations need a slightly different switchMatrix,
    // compensate by manually remapping the order of the x,y,z translations
    const arMatrix4 transMatrix = invSwitchMatrix * ar_translationMatrix(
      _positionScale * arVector3(_floatData[0], -_floatData[2], _floatData[1]));
    const arMatrix4 rotMatrix = arMatrix4( arQuaternion( _floatData + 3 ) );
    const arMatrix4 finalMatrix =
      transMatrix * 
      invSwitchMatrix * 
      !rotMatrix * 
      switchMatrix * 
      coordMatrix;

    // queue the calibrated translation/rotation matrix 
    queueMatrix( birdAddress, finalMatrix );
  }
  sendQueue();
  return true;
}
#endif
