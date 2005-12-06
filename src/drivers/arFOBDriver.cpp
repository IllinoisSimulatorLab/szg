//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#ifdef AR_USE_WIN_32
#include <windows.h>
#include <stdio.h>
#endif

#include "arFOBDriver.h"

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arFOBDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

void ar_FOBDriverEventTask(void* FOBDriver){
  arFOBDriver* driver = (arFOBDriver*) FOBDriver;
  driver->_eventThreadRunning = true;

  while (!driver->_stopped) {
    for (int i=1; i<=driver->_numFlockUnits; i++){
      // Only request information from flock units that hold sensors
      if (driver->_sensorMap[i] > -1){
	if (!driver->_getSendNextFrame(i)){
	  cerr << "arFOBDriver error: get data failed.\n";
	  driver->_stopped = true;
	}
      }
    }
    driver->sendQueue();
  }
  driver->_eventThreadRunning = false;
}

arFOBDriver::arFOBDriver() :
  _timeoutTenths( 10 ),
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
  stringstream& initResponse = SZGClient.initResponse();

  // Syzygy's default frame of reference differs from Ascension's.
  // When the bird's flat surface points down and
  // the cord points back, then the rotation matrix should be
  // the identity vs. the Syzygy coordinate system. In other words, a
  // coordinate frame is attached to the bird with positive z going back along
  // the cord, positive x right, and positive y up.

  // Read in the calibration information. This base transformation corrects
  // for an offset in position and orientation of the transmitter,
  // letting us report a given bird
  // position/orientation as some arbitrary value.
  // Note that szg matrices are in OpenGL order
  // (i.e. down the columns, not along the rows).

  // In practice, you place the bird at an identity orientation
  // (as described above) relative to your physical coordinate system.
  // Furthermore, it will be in a known location. You can then compute
  // the matrix needed to align it.

  // The base transformation is post-multiplied with the supplied data. 

  // This calibration information is only rough.  It can be followed
  // (as we do in ISL) with a more precise grid-based interpolation.

  const string transmitterOffset(
    SZGClient.getAttribute("SZG_FOB","transmitter_offset"));
  if (transmitterOffset != "NULL"){
    ar_parseFloatString(transmitterOffset, _transmitterOffset.v, 16);
    initResponse << "Using transmitter offset =\n"
	         << _transmitterOffset << "\n";
  }

  // The sensor might be rotated in its "neutral" position.
  // For instance, The bird might be taped upside-down to a wand.
  // Then we must pre-multiply a matrix, to transform a "neutral" frame
  // (the physical coordinate frame) into the Syzygy frame that is attached
  // to the bird.

  // This rotation is described by an axis followed by an angle in degrees.

  float temp[4]; 
  for(int j = 0; j < _FOB_MAX_DEVICES; j++) {
    stringstream ss; ss << j;
    const string sensorNum( "sensor" + ss.str() + "_rot" );
    const string sensorRotValue(
      SZGClient.getAttribute("SZG_FOB",sensorNum));
    if (sensorRotValue != "NULL") {
      ar_parseFloatString(sensorRotValue, temp, 4);
      _sensorRot[ j ] =
        ar_rotationMatrix(arVector3(temp), ar_convertToRad(temp[3]));
      initResponse << "Using sensor " << j << " rotation =\n"
  	           << _sensorRot[ j ] <<"\n";
    }
  }
  
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
    cerr << "arFOBDriver warning: unexpected SZG_FOB/baud_rate value "
	 << baudRate << ".\n  Expected one of:";
    for (i=0; i<_nBaudRates; i++){
      cerr << " " << baudRates[i];
    }
    cerr << "\n";
LDefaultBaudRate:
    baudRate = 115200;
    cerr << "arFOBDriver remark: SZG_FOB/baud_rate defaulting to "
         << baudRate << ".\n";
  }

  // We only support connecting to a flock of birds via a single serial port.
  _comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_FOB", "com_port"));

  // Get the FoB's "description", a string of slash-delimited integers
  // of length equal to the number of FOB units.
  // Each unit's configuration is as follows:
  // 0: bird
  // 1: transmitter
  // 2: transmitter AND bird
  // 3: extended range transmitter
  // 4: extended range transmitter AND bird
  char receivedBuffer[512];
  int birdConfiguration[_FOB_MAX_DEVICES+1];
  const string received(SZGClient.getAttribute("SZG_FOB", "config"));
  if (received == "NULL") {
    cerr << "arFOBDriver error: SZG_FOB/config undefined.\n";
    return false;
  }
  ar_stringToBuffer(received, receivedBuffer, sizeof(receivedBuffer));
  _numFlockUnits = ar_parseIntString(
    receivedBuffer, birdConfiguration+1, _FOB_MAX_DEVICES );
  if (_numFlockUnits < 1) {
    cerr << "arFOBDriver error: SZG_FOB/config needs one entry per bird and ERC.\n";
    return false;
  }

  // We now determine the configuration.
  // Sensor map is an array, indexed by flock ID, whose values
  // are -1 for no attached sensor, and otherwise the appropriate Syzygy
  // ID. So, if SZG_FOB/config = 0/1/0/0 then
  // _sensorMap[0] = 0;
  // _sensorMap[1] = -1;
  // _sensorMap[2] = 1;
  // _sensorMap[3] = 2;
  _numBirds = 0;
  _transmitterID = 0;
  _extendedRange = false;
  // flock addresses start at 1
  for (i=1; i<=_numFlockUnits; i++){
    switch (birdConfiguration[i]) {
    case 1:
    case 2:
    case 3:
    case 4:
      if (_transmitterID>0){
        // The configuration already included a transmitter.
	cerr << "arFOBDriver error: multiple transmitters defined.\n";
	return false;
      }
    }
    switch (birdConfiguration[i]) {
    default:
      cerr << "arFOBDriver error: illegal configuration value.\n";
      return false;
    case 0: // Bird, no transmitter
      _sensorMap[i] = _numBirds++;
      break;
    case 1: // Normal transmitter only
      _sensorMap[i] = -1;
      _transmitterID = i;
      break;
    case 2: // Normal transmitter plus bird
      _transmitterID = i;
      _sensorMap[i] = _numBirds++;
      break;
    case 3: // Extended range transmitter only
      _sensorMap[i] = -1;
      _transmitterID = i;
      _extendedRange = true;
      break;
    case 4: // Extended range transmitter plus bird
      _sensorMap[i] = _numBirds;
      _transmitterID = i;
      _extendedRange = true;
      _numBirds++;
      break;
    }
  }
  if (!_transmitterID){
    cerr << "arFOBDriver error: illegal configuration. No transmitter.\n";
    return false;
  }
  cout << "arFOBDriver remark: " << _numBirds
       << " birds, transmitter at unit " << _transmitterID << ".\n";

  // The number of birds found is the number of matrices we will report.
  _setDeviceElements( 0, 0, _numBirds );

  // Open the serial port.
  if (!_comPort.ar_open(_comPortID,(unsigned long)baudRate,8,1,"none" )){
    cerr << "arFOBDriver error: failed to open serial port " << _comPortID
	 << ".\n";
    return false;
  }

  if (!_comPort.setReadTimeout(_timeoutTenths)){
    cerr << "arFOBDriver error: failed to set timeout for port " << _comPortID
	 << ".\n";
    return false;
  }

  // Configure the FoB.

  // First, set the data mode for all the birds. This is hard-coded to be position-quaternion.
  if (_numBirds > 1){
    for (i=1; i<=_numFlockUnits; i++){
      if (i != _transmitterID){
        if (!_setDataMode(i)){
	  cerr << "arFOBDriver error: failed to set data mode.\n";
	  return false;
	}
      }
    }
  }
  else{
    if (!_setDataMode(0)){
      cerr << "arFOBDriver error: failed to set data mode.\n";
      return false;
    }

    ar_usleep(500000); // Linux needs this, at least half a second.
#if 0
    unsigned char b[2];
    if (_getFOBParam(10, b, 1, 0) != 1)
      cout << "get error code kacked.\n";
    cout << "error code is " << int(b[0]) << ".\n";
    if (_getFOBParam(1, b, 2, 0) != 2)
      cout << "get version number kacked.\n";
    cout << "bird version number is " << int(b[0]) << "." << int(b[1]) << ".\n";
    ar_usleep(100000);
#endif

  }

  // Set the hemisphere for all the birds.
  const string hemisphere(SZGClient.getAttribute("SZG_FOB","hemisphere"));
  if (hemisphere == "NULL") {
    cerr << "arFOBDriver error: SZG_FOB/hemisphere undefined.\n";
    return false;
  }
  if (_numBirds > 1){
    for (i=1; i<=_numFlockUnits; i++){
      if (i != _transmitterID){
        if (!_setHemisphere(hemisphere, i)){
	  cerr << "arFOBDriver error: failed to set hemisphere.\n";
	  return false;
	}
      }
    }
  }
  else{
    if (!_setHemisphere(hemisphere, 0)){
      cerr << "arFOBDriver error: failed to set hemisphere.\n";
      return false;
    }
    // Linux needs these 2 lines.  Some kind of timing thing.
    string foo;
    (void)_getDataMode(foo, 0);
  }

  // Make sure we are using the correct transmitter. If we are not
  // using the default transmitter (flock unit 1), change must occur.
  if (_transmitterID != 1)
    _nextTransmitter(_transmitterID);

  // The extended range transmitter CANNOT be queried in this way.
  // It seems to return 36, which is wrong!
  if (_extendedRange){
    _positionScale = 12/32768.0; // Convert to feet.
    cout << "arFOBDriver remark: using extended range transmitter.\n";
  }
  else{
    bool longRange = false;
    if (!_getPositionScale( longRange ))
      return false;
    cout << "arFOBDriver remark: FOB's scale = "
         << (longRange ? 72 : 36) << " inches.\n";
  }

  // Create a buffer for reading-in data. This must occur AFTER
  // _setDataMode(...).
  // Two bytes/number + a bird address if there's more than one bird.
  // NOTE: THIS HAS NOT BEEN FULLY SET_UP YET! Specifically, this is
  // probably enough space for "group mode", whereby we make one 
  // request for data and get everything back at once. 
  int bytesPerBird = 2*_dataSize + ((_numBirds > 1)?(1):(0));
  _dataBuffer = new unsigned char[bytesPerBird*_numBirds];

  // Finally, iff we are in "flock" mode (more than one unit),
  // issue an auto_config command to get the data moving.
  if (_numBirds > 1){
    if (!_autoConfig()) {
      cerr << "arFOBDriver error: auto-config command failed.\n";
      return false;
    }
  }
  else{
    // The run command seems to do nothing to a flock,
    // but it wakes up a standalone unit.
    if (!_run()){
      cerr << "arFOBDriver error: run failed.\n";
      return false;
    }
    _setDataMode(0); // Linux needs this, one more time.
  }

  // Flock configured. All the FoB's lights should be on solid.
  return true;
}

bool arFOBDriver::start() {
  return _eventThread.beginThread(ar_FOBDriverEventTask,this);
}

bool arFOBDriver::stop() {
  _stopped = true;
  while (_eventThreadRunning)
    ar_usleep(20000);
  _sleep();
  _comPort.ar_close();
  return true;
}

bool arFOBDriver::_setHemisphere( const std::string& hemisphere,
                                  unsigned char addr) {
  static unsigned char cdata[] = {'L', 0, 0};
  // Setup the Command string to the Bird...
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
    cerr << "arFOBDriver error: invalid hemisphere #.\n";
    return false;
  }
  if (!_sendBirdAddress(addr))
    return false;
  const bool ok = _sendBirdCommand( cdata, 3 );
  ar_usleep(100000);
  return ok;
}

// NOTE: _getHemisphere might not be supported on older flock hardware!
bool arFOBDriver::_getHemisphere( std::string& hemisphere,
                                  unsigned char addr ) {
  unsigned char outdata[2];
  if (_getFOBParam( 22, outdata, 2, addr ) != 2) {
    cerr << "arFOBDriver error: _getFOBParam(hemisphere) failed.\n";
    return false;
  }
  if ((outdata[0] == 0)&&(outdata[1] == 0)) {
    hemisphere = "front";
  } else if ((outdata[0] == 0)&&(outdata[1] == 1)) {
    hemisphere = "rear";
  } else if ((outdata[0] == 0xc)&&(outdata[1] == 1)) {
    hemisphere = "upper";
  } else if ((outdata[0] == 0xc)&&(outdata[1] == 0)) {
    hemisphere = "lower";
  } else if ((outdata[0] == 6)&&(outdata[1] == 1)) {
    hemisphere = "left";
  } else if ((outdata[0] == 6)&&(outdata[1] == 0)) {
    hemisphere = "right";
  } else {
    hemisphere = "unknown";
    return false;
  }
  return true;
}

// Set bird's data mode to position+quaternion
bool arFOBDriver::_setDataMode(unsigned char addr) {
  if (!_sendBirdAddress(addr))
    return false;
  _dataSize = 7;
  return _sendBirdByte(']'); // FoB manual p.92
}

bool arFOBDriver::_getDataMode( std::string& modeString,
                                unsigned char addr ) {
  unsigned char buf[2];
  if (_getFOBParam( 0, buf, 2, addr ) != 2) {
    cerr << "arFOBDriver error: _getFOBParam(datamode) failed.\n";
    return false;
  }

  // FoB manual p.112
  const unsigned dataMode = (buf[0] >> 1) & 0xf;
#if 0
  printf("\t\t\t\t\t2bytes are %x %x\n", (unsigned)buf[0], (unsigned)buf[1]);
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
  cout << (b0 ? "point mode.\n" : "stream mode.\n");
  cout << (b5 ? "sleep !run.\n" : "run !sleep.\n");
  cout << (!b12 ? "sleep !run.\n" : "run !sleep.\n");
  cout << (b6 ? "xoff\n" : "xon\n");
  cout << (b7 ? "factory test enabled\n" : "factory test disabled\n");
  cout << (b8 ? "sync mode disabled\n" : "sync mode enabled\n");
  cout << (b9 ? "crtsync\n" : "!crtsync\n");
  cout << (b10 ? "expanded addr\n" : "!expanded addr\n");
  cout << (b11 ? "host sync\n": "!host sync\n");
  cout << (b13 ? "error\n" : "!error\n");
  cout << (b14 ? "inited/autoconfd\n" : "!inited/autoconfd\n");
  cout << (b15 ? "master\n" : "slave\n");
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
    cerr << "arFOBDriver error: mode is " << dataMode << " aka " << modeString << ".\n";
  return ok;
}

bool arFOBDriver::_setPositionScale( bool longRange,
                                     unsigned char addr ) {
  const unsigned short scale = longRange ? 1 : 0;
  if (!_setFOBParam( 3, (unsigned char *)&scale, 2, addr )) {
    cerr << "arFOBDriver error: _setFOBParam failed.\n";
    return false;
  }
  _positionScale = longRange ? 6./32768. : 3./32768.;
  return true;
}

bool arFOBDriver::_getPositionScale( bool& longRange,
                                     unsigned char addr ) {
  unsigned char outdata[2];
  if (_getFOBParam( 3, outdata, 2, addr ) != 2) {
    cerr << "arFOBDriver error: _getFOBParam(position scale) failed.\n";
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
      cerr << "arFOBDriver error: positionScale data = " << bigScale << endl;
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
    cerr << "arFOBDriver error: failed to send 'get param' command.\n";
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
    cerr << "arFOBDriver error: memory panic.\n";
    return false;
  }
  cdata[0] = 'P';
  cdata[1] = paramNum;
  memcpy( cdata+2, buf, numBytes );
  bool ok = _sendBirdCommand(cdata, 2+numBytes);
  if (!ok)
    cerr << "arFOBDriver error: failed to send 'set param' command.\n";
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
  ar_usleep( 1000000 );
  const unsigned char cdata[] = {'P', 0x32, (unsigned char)_numBirds };
  const bool ok = _sendBirdCommand(cdata, 3);
  ar_usleep( 1000000 );  
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
  if (addr <= 1)
    // 0 means no bird or the unique bird.
    // 1 means the first (and only) bird, when sent to _getSendNextFrame().
    return true;
  if (_numBirds <= 1) {
    cerr << "arFOBDriver warning: expected more than one bird.\n";
    return true;
  }
  const unsigned char cdata = 0xF0 + addr;
  const bool ok = _comPort.ar_write((char*)&cdata, 1) == 1;
  if (!ok)
    cerr << "arFOBDriver error: _sendBirdAddress failed.\n";
  return ok;
}

bool arFOBDriver::_sendBirdCommand( const unsigned char* cdata, 
                                    const unsigned int numBytes ) {
  return _comPort.ar_write( (char*)cdata, numBytes ) ==
    static_cast<int>(numBytes);  
}

int arFOBDriver::_getBirdData( unsigned char* cdata, 
                               const unsigned int numBytes ) {
  return _comPort.ar_read( (char *)cdata, numBytes );   
}

bool arFOBDriver::_getSendNextFrame(unsigned char addr) {
  if (!_sendBirdAddress(addr))
    return false;

  if (!_sendBirdByte('B', false)) {
    cerr << "arFOBDriver error: failed to send 'point' command.\n";
    return false;
  }

  const int bytesPerBird = 2*_dataSize;
  const int bytesRead = _comPort.ar_read(
    (char*)_dataBuffer, static_cast<unsigned int>(bytesPerBird));
  if (bytesRead != bytesPerBird) {
    cerr << "arFOBDriver error: number of bytes read (" << bytesRead
         << ") unequal to number requested (" << bytesPerBird << ")\n";
    return false;
  }
  short* birdData = (short*)_dataBuffer;
  int j;
  for (j=0; j<_dataSize; j++) {
    // Copied & pasted from Ascension code CMDUTIL.C
    *birdData = (short)((((short)(*(unsigned char *) birdData) & 0x7F) |
		(short)(*((unsigned char *) birdData+1)) << 7)) << 2;
    _floatData[j] = (float)*birdData++;
  }
     
  // This matrix maps the coordinate system axes in the szg coordinate system
  // to the coordinate axes in the FOB transmitter coordinate system
  static const arMatrix4 coordSystemTrans = 
    ar_rotationMatrix( 'z', ar_convertToRad(90)) *
    ar_rotationMatrix( 'y', ar_convertToRad(-90)); 
  // The rotation matrix of the bird. PLEASE NOTE: It seems like several
  // Ascension products return the INVERSE (or, equivalently for rotation
  // matrices, the transpose) of the bird's rotation matrix. Certainly,
  // this is also true for the Ascension spacepad!   
  // specific to position+quaternion data mode!!!
  for (j=3; j<7; ++j)
    _floatData[j] *= _orientScale;

  const arMatrix4 rotMatrix(arQuaternion( _floatData + 3));
  // The bird's translation, scaled appropriately.
  const arMatrix4 translationMatrix = 
    ar_translationMatrix( _positionScale * arVector3(_floatData));

  /*
  The calculation of the matrix to be reported to the input device is
  involved. But this reasoning holds true for all 6DOF sensors.  In this
  discussion, the Flock coordinate frame is B2 and the Syzygy coordinate
  frame is B1.  In the context of VR, the Syzygy reference frame is the
  frame of the graphics.  Our final goal is a transformation matrix in B1.

  Suppose R2 is a rotation matrix w.r.t. basis B2.  Let M be a rotation
  taking B1 to B2.  If P1 gives a point's coordinates with respect to
  B1 and P2 those w.r.t. B2 then (! is inverse):

  P2 = (!M)*P1
  P1 = M*P2

  So M maps coordinates in basis B2 to those in B1.  !M does the inverse.
  Now, let R1 be rotation R2, but w.r.t. B1.
  Its matrix must then be:

  M*R2*(!M)

  Furthermore, M*T changes a translation T in B2 to one in B1.  If the
  transmitter's reference frame is as above (in relation to the Syzygy
  reference frame) then the correct matrix is:

  M*T*R2*(!M)

  Now consider sensor rotation. The sensor is attached to a bird.
  It has a NEUTRAL physical orientation relative to the transmitter
  (where the rotation matrix is the identity). Consider the rotation N
  w.r.t. B1 that takes the sensor from its orientation on the bird to
  its neutral position. The bird should report a rotation of !N when in
  its neutral position. So the orientation and position of the bird is:

  M*T*R2*(!M)*(!N)

  Finally, we consider transmitter rotation and translation offset.
  Suppose that the (0,0,0) of the Flock is translated from Syzygy's
  (0,0,0) by T1. Furthermore, suppose that the transmitter is rotated
  by R3 from its neutral position (related to the neutral position of
  the sensor) from the Syzygy reference frame. The orientation of the
  interaction device is:

  T1*R3*M*T*R2*(!M)*(!N)
  
  This does not account for sensor translation offset.  The 180 degree
  rotation about y accounts for the fact that the bird's neutral position
  is flat surface down and tail forward.  (whereas Syzygy assumes flat
  surface down and tail back).
  */

  arMatrix4 finalMatrix 
    = _transmitterOffset*
      (coordSystemTrans)*translationMatrix*!rotMatrix*(!coordSystemTrans)*
      _sensorRot[_sensorMap[addr]]*
      ar_rotationMatrix('y', ar_convertToRad(180));
  // Copy the matrix to the driver queue for sending. The actual
  // sendQueue() command will occur in the event thread.
  queueMatrix(_sensorMap[addr], finalMatrix);
  return true;
}

// KEEP THE BELOW AROUND UNTIL WE FULLY HANDLE GROUP MODE, SINCE THIS CONTAINS
// EXAMPLE CODE.
/*bool arFOBDriver::_getSendNextFrame() {
  if (!_configured) {
    cerr << "arFOBDriver error: attempt to get data before init().\n";
    return false;
  }
  const int bytesPerBird = 2*_dataSize + (_numBirds>1 ? 1 : 0);
  int numBytes = bytesPerBird;
  int bytesRead = -1;
  int i = 0;
  int j = 0;
  if (_numComports == 1) {
    numBytes *= _numBirds;
    bytesRead = _comPorts[0].ar_read( (char*)_dataBuffer, numBytes );
    if (bytesRead != numBytes) {
      cerr << "arFOBDriver error: # bytes read (" << bytesRead
           << ") <> # requested (" << numBytes << ")\n";
      stop();
      return false;
    }
  } else {
    for (i=0; i<_numComports; i++) {
      bytesRead = _comPorts[i].ar_read( (char*)_dataBuffer+i*numBytes, numBytes );
      if (bytesRead != numBytes) {
        cerr << "arFOBDriver error: # bytes read from Bird #" << i << endl
             << " (" << bytesRead << ") <> # requested (" << numBytes << ")\n";
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
    const int birdAddress = (_numBirds == 1) ? 0 :
      // addresses go from 1-_numBirds (or else).  Subtract 1 to get matrix #
      *(_dataBuffer + (i+1)*bytesPerBird - 1) - 1;
    if ((birdAddress < 0)||(birdAddress >= _numBirds)) {
      cerr << "arFOBDriver error: Bird address (" << birdAddress
           << ") out of range.\n";
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
    arMatrix4 transMatrix = invSwitchMatrix * 
                            ar_translationMatrix(  _positionScale * arVector3(  _floatData[ 0 ], 
                                                                               -_floatData[ 2 ], 
                                                                                _floatData[ 1 ] ) );
    
    arMatrix4 rotMatrix = arMatrix4( arQuaternion( _floatData + 3 ) );
    
    arMatrix4 finalMatrix = _transmitterOffset * 
                            transMatrix * 
                            invSwitchMatrix * 
                            !rotMatrix * 
                            switchMatrix * 
                            coordMatrix * 
                            _sensorRot[ birdAddress ];
          
    // queue the calibrated translation/rotation matrix 
    queueMatrix( birdAddress, finalMatrix );
  }
  sendQueue();
  return true;
  }*/

