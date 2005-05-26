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

#include "arBirdWinDriver.h"

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arBirdWinDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

#ifdef EnableBirdWinDriver
void ar_WinBirdDriverEventTask(void* FOBDriver) {
  arBirdWinDriver* fobDriver = (arBirdWinDriver*) FOBDriver;
  BOOL status = birdStartFrameStream( fobDriver->_groupID );
  if (!status) {
    cerr << "arBirdWinDriver error: failed to start data stream.\n";
    fobDriver->stop();
    return;
  }
  cerr << "arBirdWinDriver remark: Started data stream.\n";
  fobDriver->_streamingStarted = true;
  while (true) {
    const double _INCHES_TO_FEET = 1./12.;
    ar_usleep(5000);
    if (!birdFrameReady( fobDriver->_groupID ))
      continue;
    BIRDFRAME frame;
    birdGetMostRecentFrame( fobDriver->_groupID, &frame );

    // This code should be updated to match the elegant version
    // in FOBServer.cpp's start().
    // Or better yet, factor out the common code from here and there!

    arVector3 pos;
    float quat[4] = {0};
    WORD posScale = 0;
    BIRDREADING* preading = NULL;
    if (fobDriver->_standAlone) {
      posScale = fobDriver->_devConfig[0].wScaling;
      preading = &frame.reading[0]; 
      // convert position and angle data
      pos[0] = _INCHES_TO_FEET * preading->position.nX * posScale / 32767.;
      pos[1] = _INCHES_TO_FEET * preading->position.nY * posScale / 32767.;
      pos[2] = _INCHES_TO_FEET * preading->position.nZ * posScale / 32767.;
      quat[0] = preading->quaternion.nQ0 / 32767.;
      quat[1] = preading->quaternion.nQ1 / 32767.;
      quat[2] = preading->quaternion.nQ2 / 32767.;
      quat[3] = preading->quaternion.nQ3 / 32767.;
      
//      arMatrix4 quatMatrix( arQuaternion(quat[0],quat[1],quat[2],quat[3]) );
//      arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );

      const arMatrix4 fobMatrix( ar_transrotMatrix( pos, arQuaternion( quat ) ) );
      fobDriver->queueMatrix( 0, fobMatrix );
    } else {
      for (int i=1; i<=fobDriver->_numDevices; i++) {
        posScale = fobDriver->_devConfig[i].wScaling;
        preading = &frame.reading[i]; 
        // convert position and angle data
        pos[0] = _INCHES_TO_FEET * preading->position.nX * posScale / 32767.;
        pos[1] = _INCHES_TO_FEET * preading->position.nY * posScale / 32767.;
        pos[2] = _INCHES_TO_FEET * preading->position.nZ * posScale / 32767.;
        quat[0] = preading->quaternion.nQ0 / 32767.;
        quat[1] = preading->quaternion.nQ1 / 32767.;
        quat[2] = preading->quaternion.nQ2 / 32767.;
        quat[3] = preading->quaternion.nQ3 / 32767.;
        
      //	arQuaternion orient( quat[0],quat[1],quat[2],quat[3] );
      //	arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );
      
        const arMatrix4 fobMatrix( ar_transrotMatrix( pos, arQuaternion( quat ) ) );
        fobDriver->queueMatrix( i-1, fobMatrix );
      }
    }
    fobDriver->sendQueue();
  }
#else
void ar_WinBirdDriverEventTask(void* /*FOBDriver*/) {
#endif
}

arBirdWinDriver::arBirdWinDriver() :
  _flockWoken( false ),
  _streamingStarted( false )
#ifdef EnableBirdWinDriver
  ,
  _groupID( 1 ),
  _readTimeout( 2000 ),
  _writeTimeout( 2000 )
#endif
{}

#ifdef EnableBirdWinDriver
bool arBirdWinDriver::init(arSZGClient& SZGClient) {
#ifdef AR_USE_WIN_32
  const int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
  const BYTE hemiNums[] = {BHC_FRONT,BHC_REAR,BHC_UPPER,BHC_LOWER,BHC_LEFT,BHC_RIGHT};
  const string hemispheres[] = {"front","rear","upper","lower","left","right"};
  string received = SZGClient.getAttribute("SZG_FOB", "com_ports");
  char receivedBuffer[512];
  int intComPorts[_FOB_MAX_DEVICES];
  ar_stringToBuffer( received, receivedBuffer, sizeof(receivedBuffer) );
  _numDevices = ar_parseIntString( receivedBuffer, intComPorts, _FOB_MAX_DEVICES );
  if (_numDevices < 1) {
    cerr << "arBirdWinDriver error: SZG_FOB/com_ports must contain an "
         << "entry for each bird and ERC.\n"
         << "     (use '0' if a given device doesn't have a direct serial "
	       << "connection).\n";
    stop();
    return false;
  }
  if (_numDevices > _FOB_MAX_DEVICES) {
    cerr << "arBirdWinDriver::init() error: (SZG_FOB/com_ports):\n"
         << "     To use more than " << _FOB_MAX_DEVICES 
         << " devices, change _FOB_MAX_DEVICES in arBirdWinDriver.h and rebuild.\n";
    stop();
    return false;
  }
  _setDeviceElements( 0, 0, _numDevices );
  _standAlone = (_numDevices == 1);
  int i = 0;
  if (_standAlone) {  // for standalone, we put the COM port # at array position 0
    cerr << "arBirdWinDriver remark: standalone configuration using COM port " << intComPorts[0] << endl;
    _comPorts[0] = static_cast<WORD>( intComPorts[0] );
  } else {   // for multiple, we start loading the COM port #s into array at position 1
    cerr << "arBirdWinDriver remark: COM ports: ";
    for (i=0; i<_numDevices; i++) {
      cerr << intComPorts[i] << "/";
      _comPorts[i+1] = static_cast<WORD>(intComPorts[i]);
    }
    cerr << endl;
  }
  _baudRate = SZGClient.getAttributeInt("SZG_FOB", "baud_rate");
  bool baudRateFound = false;
  for (i=0; i<_nBaudRates; i++) {
    if (_baudRate == baudRates[i]) {
      baudRateFound = true;
      break;
    }
  }
  if (!baudRateFound) {
    cerr << "arBirdWinDriver warning: illegal value for SZG_FOB/baud_rate.\n"
         << "  Legal values are:";
    for (i=0; i<_nBaudRates; i++)
      cerr << " " << baudRates[i];
    _baudRate = 115200;
    cerr << "\n   Defaulting to " << _baudRate << ".\n";
  }
  string hemisphere(SZGClient.getAttribute("SZG_FOB", "hemisphere"));
  int hemiFound = -1;
  BYTE hemisphereNum;
  for (i=0; i<_nHemi; i++)
    if (hemisphere == hemispheres[i])
      hemiFound = i;
  if (hemiFound==-1) {
    cerr << "arBirdWinDriver warning: illegal value for SZG_FOB/hemisphere.\n"
         << "     Legal values are:";
    for (i=0; i<_nHemi; i++)
      cerr << " " << hemispheres[i];
    hemisphere = "front";
    cerr << "\n   Defaulting to " << hemisphere << endl;
    hemiFound = 0;
  }
  _hemisphereNum = hemiNums[hemiFound];
  
  if (!birdRS232WakeUp(_groupID,_standAlone,_numDevices,_comPorts,
                       _baudRate,_readTimeout,_writeTimeout)) {
    cerr << "arBirdWinDriver error: failed to wake up the flock.\n";
    stop();
    return false;
  }
  cerr << "arBirdWinDriver remark: Woke up flock consisting of " 
       << _numDevices << " devices.\n";
  _flockWoken = true;
  BOOL status = birdGetSystemConfig(_groupID,&_sysConfig);
  if (!status) {
    cerr << "arBirdWinDriver error: failed to get flock's system config.\n";
    stop();
    return false;
  }
  if (!_standAlone) {
    // check to see how many devices are really present
    int devcnt = 0;
    for (i=0; i < _FOB_MAX_DEVICES; i++) {
      if (_sysConfig.byFlockStatus[i] & BFS_FBBACCESSIBLE)
        devcnt++;
    }
    if (devcnt != _numDevices) {
      cerr << "arBirdWinDriver error: Number of devices found, "
           << devcnt
	   << ", does not equal number specified, "
	   << _numDevices
	   << ".\n";
      stop();
      return false;
    }
  }
  if (_standAlone) {
    status = birdGetDeviceConfig(_groupID,0,&_devConfig[0]);
    if (!status) {
      cerr << "arBirdWinDriver error: failed to get config for the bird\n" ;
       stop();
      return false;
    }
    _devConfig[0].byDataFormat = BDF_POSITIONQUATERNION;
    _devConfig[0].byHemisphere = _hemisphereNum;
    status = birdSetDeviceConfig(_groupID,0,&_devConfig[0]);
    if (!status) {
      cerr << "arBirdWinDriver error: failed to set position/quaternion "
     << "mode + hemisphere for the bird" << endl;
      stop();
      return false;
    }
  } else {
    for (i=1; i<=_numDevices; i++) {
      status = birdGetDeviceConfig(_groupID,i,&_devConfig[i]);
      if (!status) {
        cerr << "arBirdWinDriver error: failed to get config for device #" 
             << i << endl;
        stop();
        return false;
      }
      cerr << "arBirdWinDriver remark: Got status for device #" << i << endl;
    }
    for (i=1; i<=_numDevices; i++) {
      _devConfig[i].byDataFormat = BDF_POSITIONQUATERNION;
      _devConfig[i].byHemisphere = _hemisphereNum;
      status = birdSetDeviceConfig(_groupID,i,&_devConfig[i]);
      if (!status) {
        cerr << "arBirdWinDriver error: failed to set position/quaternion "
       << "mode + hemisphere for device #" << i << endl;
        stop();
        return false;
      }
    }
  }
  return true;
#else
  // do lots of complicated stuff to wake up the flock.
  cerr << "arBirdWinDriver error: No linux support yet, sorry\n";
  return false;
#endif

#else
bool arBirdWinDriver::init(arSZGClient& /*SZGClient*/) {
  cerr << "arBirdWinDriver error: disabled because flock-of-birds driver is not installed.\n";
  return false;
#endif
}

bool arBirdWinDriver::start(){
#ifdef DisableFOB
  return false;
#else
  return _eventThread.beginThread(ar_WinBirdDriverEventTask,this);
#endif
}

bool arBirdWinDriver::stop(){
#ifdef EnableBirdWinDriver
  if (_flockWoken) {
    if (_streamingStarted) {
        birdStopFrameStream(_groupID);
        cerr << "arBirdWinDriver remark: Stopped data stream.\n";
    }
    birdShutDown(_groupID);
    cerr << "arBirdWinDriver remark: Flock shut down.\n";    
  }
#endif
  return true;
}

bool arBirdWinDriver::restart(){
  return stop() && start();
}
