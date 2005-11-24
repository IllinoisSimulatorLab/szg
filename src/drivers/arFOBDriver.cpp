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
  _timeoutTenths( 50 ),
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

  // Syzygy Reference Frame of the Bird:
  // Syzygy has implicit assumptions about the "neutral" position of a
  // bird. Specifically, when the bird's flat surface is straight down and
  // the "tail" is straight back, then the rotation matrix should be
  // the identity vs. the Syzygy coordinate system. In other words, a
  // coordinate frame is attached to the bird with positive z going back along
  // the tail, positive x going to the right, and positive y going up.
  // This is different from the frame used by Ascension.
  
  stringstream& initResponse = SZGClient.initResponse();
  
  // Read in the calibration information. This base transformation corrects
  // for an offset in position and orientation of the transmitter,
  // essentially allowing this driver to report a given bird
  // position/orientation as some arbitrary value. PLEASE NOTE 
  // that szg matrices are written in OpenGL order (i.e. going down the
  // columns instead of along the rows).

  // In practice, you will place the bird at an identity orientation
  // (as described above) relative to your physical coordinate system.
  // Furthermore, it will be in a known location. You can then compute
  // the matrix needed to move it into perfect alignment (locally at least).

  // The base transformation is POST-MULIPLIED onto the supplied data. 

  // NOTE: this calibration information is really the rough, first-level
  // calibration. It can be followed (as we do in the Cube) by a 
  // more precise grid-base interpolation scheme.
  string transmitterOffset 
    = SZGClient.getAttribute("SZG_FOB","transmitter_offset");
  if (transmitterOffset != "NULL"){
    ar_parseFloatString(transmitterOffset, _transmitterOffset.v, 16);
    initResponse << "Using transmitter offset =\n"
	         << _transmitterOffset << "\n";
  }
  float temp[4]; 
  // The sensor might be rotated somewhat in its "neutral" position.
  // For instance, The bird might be attached to the bottom of a 
  // manipulation device, and consequently would be upside down. In this
  // case, we must PRE-MULIPLY a matrix transform, whose function is
  // transform a "neutral" frame (i.e. a frame that is the same as
  // the physical coordinate frame) into the Syzygy frame that is
  // attached to the bird).
  // NOTE: this rotation is described by axis followed by angle in degrees.
  for( int j = 0; j < _FOB_MAX_DEVICES; j++ ){
    stringstream ss; ss << j;
    string sensorNum( "sensor" + ss.str() + "_rot" );
    string sensorRotValue
      = SZGClient.getAttribute("SZG_FOB",sensorNum);
    if (sensorRotValue != "NULL"){
      ar_parseFloatString(sensorRotValue, temp, 4);
      _sensorRot[ j ]
        = ar_rotationMatrix(arVector3(temp),ar_convertToRad(temp[3]));
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
    cerr << "arFOBDriver error: SZG_FOB/config must contain an "
         << "entry for each bird and ERC.\n";
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
        if (!_setHemisphere(hemisphere,i)){
	  cerr << "arFOBDriver error: failed to set hemisphere.\n";
	  return false;
	}
      }
    }
  }
  else{
    if (!_setHemisphere(hemisphere,0)){
      cerr << "arFOBDriver error: failed to set hemisphere.\n";
      return false;
    }
  }

  // Next, make sure we are using the correct transmitter. If we are not
  // using the default transmitter (flock unit 1), change must occur.
  if (_transmitterID != 1){
    _nextTransmitter(_transmitterID);
  }

  // The extended range transmitter CANNOT be queried in this way. It 
  // seems to return 36, which is wrong!
  if (_extendedRange){
    // Our units are feet!
    _positionScale = 12/32768.0;
    cout << "arFOBDriver remark: using extended range transmitter.\n";
  }
  else{
    bool longRange = false;
    if (!_getPositionScale( longRange )) {
      cerr << "arFOBDriver error: _getPositionScale() failed.\n";
      return false;
    }
    cout << "arFOBDriver remark: FOB reports position scale = "
         << (longRange ? 72 : 36)
         << " inches.\n";
  }

  // Create a buffer for reading-in data. This must occur AFTER
  // _setDataMode(...).
  // Two bytes/number + a bird address if there's more than one bird.
  // NOTE: THIS HAS NOT BEEN FULLY SET_UP YET! Specifically, this is
  // probably enough space for "group mode", whereby we make one 
  // request for data and get everything back at once. 
  // We do not do this yet, though we will eventually do so.
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
  }

  // Success. All the FoB's lights should be on solidly.
  cout << "arFOBDriver remark: flock configured.\n";
  return true;
}

bool arFOBDriver::start() {
  return _eventThread.beginThread(ar_FOBDriverEventTask,this);
}

bool arFOBDriver::stop() {
  _stopped = true;
  while (_eventThreadRunning){
    ar_usleep(10000);
  }
  _sleep();
  _comPort.ar_close();
  return true;
}

bool arFOBDriver::_setHemisphere( const std::string& hemisphere,
                                  unsigned char addr) {
  static unsigned char cdata[] = {'L', 0, 0};
  // Setup the Command string to the Bird...
  // 2 data bytes must be set for HEMI_AXIS and HEMI_SIGN
  if (hemisphere == "front") {
    cdata[1] = 0;       /* set HEMI_AXIS */
    cdata[2] = 0;       /* set HEMI_SIGN */
  } else if (hemisphere == "rear") {
    cdata[1] = 0;       /* set HEMI_AXIS */
    cdata[2] = 1;       /* set HEMI_SIGN */
  } else if (hemisphere == "upper") {
    cdata[1] = 0xc;     /* set HEMI_AXIS */
    cdata[2] = 1;       /* set HEMI_SIGN */
  } else if (hemisphere == "lower") {
    cdata[1] = 0xc;     /* set HEMI_AXIS */
    cdata[2] = 0;       /* set HEMI_SIGN */
  } else if (hemisphere == "left") {
    cdata[1] = 6;       /* set HEMI_AXIS */
    cdata[2] = 1;       /* set HEMI_SIGN */
  } else if (hemisphere == "right") {
    cdata[1] = 6;       /* set HEMI_AXIS */
    cdata[2] = 0;       /* set HEMI_SIGN */
  } else {  
    cerr << "arFOBDriver error: invalid hemisphere #.\n";
    return false;
  }
  if (addr > 0 && !_sendBirdAddress(addr))
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
    cerr << "arFOBDriver error: _getFOBParam() failed.\n";
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

// Sets position+quaternion data mode
bool arFOBDriver::_setDataMode(unsigned char addr) {
  const unsigned char posorientcmd = ']';
  _dataSize = 7;
  if (addr > 0 && !_sendBirdAddress(addr))
    return false;
  return _sendBirdCommand( &posorientcmd, 1 );
}

bool arFOBDriver::_getDataMode( std::string& modeString,
                                unsigned char addr ) {
  unsigned char buf[2];
  if (_getFOBParam( 0, buf, 2, addr ) != 2) {
    cerr << "arFOBDriver error: _getFOBParam() failed.\n";
    return false;
  }
  bool ok = true;
  unsigned short dataMode = ((*((unsigned short*)buf)) >> 1) & 8;
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
    cerr << "arFOBDriver error: " << dataMode << " " << modeString << endl;
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
    cerr << "arFOBDriver error: _getFOBParam() failed.\n";
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
  static unsigned char cdata[] = {'O', (unsigned char)paramNum };
  if (addr > 0 && !_sendBirdAddress(addr))
    return 0;
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
  unsigned char* cdata = new unsigned char[2+numBytes];
  if (!cdata) {
    cerr << "arFOBDriver error: memory panic.\n";
    return false;
  }
  cdata[0] = 'P';
  cdata[1] = paramNum;
  memcpy( cdata+2, buf, numBytes );
  bool stat = true;
  if (addr > 0 && !_sendBirdAddress(addr)) {
    delete [] cdata;
    return false;
  }
  if (!_sendBirdCommand( cdata, 2+numBytes )) {
    cerr << "arFOBDriver error: failed to send 'set param' command.\n";
    stat = false;
  }
  delete [] cdata;
  return stat;
}

bool arFOBDriver::_autoConfig() {
  static unsigned char cdata[] = {'P',0x32,0 };
  cdata[2] = (unsigned char)_numBirds;   
  ar_usleep( 1000000 );
  int numWrit = _comPort.ar_write( (char*)cdata, 3 );
  ar_usleep( 1000000 );  
  return numWrit==3;
}

bool arFOBDriver::_nextTransmitter(unsigned char addr){
  static unsigned char cdata[] = {'0',0 };
  // The address of the transmitter unit goes in the most significant
  // half of the byte. The transmitter number goes in the lower
  // half. We are assuming that this is 0.
  cdata[1] = (addr << 4);    
  int numWrit = _comPort.ar_write( (char*)cdata, 2 );
  ar_usleep( 1000000 );  
  return numWrit==2;
}

bool arFOBDriver::_sleep(){
  static unsigned char cdata[] = {'G'};
  int numWrit = _comPort.ar_write( (char*)cdata, 1);
  ar_usleep(1000000);
  return numWrit==1;
}

bool arFOBDriver::_run(){
  static unsigned char cdata[] = {'F'};
  int numWrit = _comPort.ar_write( (char*)cdata, 1);
  ar_usleep(1000000);
  return numWrit==1;
}

bool arFOBDriver::_sendBirdAddress( unsigned char birdAddress ) {
  unsigned char cdata[] = { 0xF0 };
  *cdata += birdAddress;
  return _comPort.ar_write( (char*) cdata, 1 )==1;
}

bool arFOBDriver::_sendBirdCommand( const unsigned char* cdata, 
                                    const unsigned int numBytes ) {
  return _comPort.ar_write( (char*)cdata, numBytes )==static_cast<int>(numBytes);  
}

int arFOBDriver::_getBirdData( unsigned char* cdata, 
                               const unsigned int numBytes ) {
  return _comPort.ar_read( (char *)cdata, numBytes );   
}

bool arFOBDriver::_getSendNextFrame(unsigned char addr) {
  // DO NOT SEND THE ADDRESS IF WE ARE ONLY ONE BIRD!
  if (_numBirds > 1){
    _sendBirdAddress(addr);
  }
  const unsigned char pointCommand[] = {'B'};
  if (!_sendBirdCommand( pointCommand, 1 )) {
    cerr << "arFOBDriver error: failed to send 'point' command.\n";
    return false;
  }

  // NOTE: AN EXTRA BYTE IS ONLY DELIVERED IN GROUP MODE!
  const int bytesPerBird = 2*_dataSize;
  const int bytesRead = _comPort.ar_read( (char*)_dataBuffer, static_cast<unsigned int>(bytesPerBird) );
  if (bytesRead != bytesPerBird) {
    cerr << "arFOBDriver error: # bytes read (" << bytesRead
         << ") <> # requested (" << bytesPerBird << ")\n";
    return false;
  }
  short* birdData = (short*) _dataBuffer;
  int j;
  for (j=0; j<_dataSize; j++) {
    // Copied & pasted from Ascension code CMDUTIL.C
    *birdData = (short)((((short)(*(unsigned char *) birdData) & 0x7F) |
		(short)(*((unsigned char *) birdData+1)) << 7)) << 2;
    _floatData[j] = (float)*birdData++;
  }
     
  // This matrix maps the coordinate system axes in the szg coordinate system
  // to the coordinate axes in the FOB transmitter coordinate system
  arMatrix4 coordSystemTrans = 
    ar_rotationMatrix( 'z', ar_convertToRad(90))*
    ar_rotationMatrix( 'y', ar_convertToRad(-90)); 
  // The rotation matrix of the bird. PLEASE NOTE: It seems like several
  // Ascension products return the INVERSE (or, equivalently for rotation
  // matrices, the transpose) of the bird's rotation matrix. Certainly,
  // this is also true for the Ascension spacepad!   
  // specific to position+quaternion data mode!!!
  for (j=3; j<7; j++){
    _floatData[j] *= _orientScale;
  }          
  arMatrix4 rotMatrix = arMatrix4( arQuaternion( _floatData + 3 ) );
  // The bird's translation, scaled appropriately.
  arMatrix4 translationMatrix = 
    ar_translationMatrix( _positionScale * arVector3(_floatData[0], 
                                                     _floatData[1], 
                                                     _floatData[2]));
  // The calculation of the matrix to be reported to the input device is
  // a little involved. HOWEVER, this reasoning holds true for ALL
  // 6DOF sensors.
  // In the discussion that follows, the Ascension Flock coordinate frame
  // will be B2 and the Syzygy coordinate reference frame will be B1.
  // In the context of virtual reality, the Syzygy reference frame is the
  // frame of the GRAPHICS.
  // Our final goal is a transformation matrix in B1.
  // First of all, note the following linear algebra.
  // Suppose R2 is a rotation matrix with respect to basis B2.
  // Suppose M is a rotation taking basis B1 to basis B2.
  // Note that if P1 gives a point's coordinates with respect to B1
  // and P2 gives a point's coordinates with respect to B2 then:
  // P2 = (!M)*P1     (here ! is inverse)
  //  and
  // P1 = M*P2
  // Consequently, M maps coordinates in basis B2 to coordinates in B1.
  // !M maps coordinates in B1 to coordinates in B2.
  // Now, suppose that R1 is rotation R2, but with respect to basis B1.
  // The matrix must then be, according to our reasoning,
  //  M*R2*(!M)
  // Note furthermore that the M*T changes a translation T in basis B2
  // to a translation in B1.
  // If the transmitter's reference frame is as above (in relation to the
  // Syzygy reference frame) then the correct matrix is:
  //  M*T*R2*(!M)
  // Now, we must consider sensor rotation. The sensor will be attached to
  // an interaction device of some sort. The sensor has a NEUTRAL physical
  // orientation relative to the transmitter (in this position the rotation
  // matrix is the identity). Consider the rotation N (with respect to the
  // Syzygy reference frame) that takes the sensor from its orientation on 
  // the interaction device to its NEUTRAL position. Note that the interaction
  // device should report a rotation of !N when the sensor is in its NEUTRAL
  // position. Consequently,
  //  M*T*R2*(!M)*(!N)
  // gives the orientation and position of the interaction device.
  // Finally, we must consider transmitter rotation and translation offset.
  // Suppose that the (0,0,0) of the Flock is translated from Syzygy's (0,0,0)
  // by T1. Furthermore, 
  // suppose that the transmitter is rotated by R3 from its neutral position
  // (related to the neutral position of the sensor) from the Syzygy reference
  // frame. The orientation of the interaction device is now given by:
  //   T1*R3*M*T*R2*(!M)*(!N)
  //
  // NOTE: HAVE NOT ACCOUNTED YET FOR SENSOR TRANSLATION OFFSET! 
  // NOTE: the 180 degree rotation about y accounts for the fact that
  //       the bird's NEUTRAL position is flat surface down and tail forward.
  //       (whereas we are assuming flat surface down and tail back).
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

