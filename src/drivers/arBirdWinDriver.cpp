//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBirdWinDriver.h"

DriverFactory(arBirdWinDriver, "arInputSource")

#ifndef EnableBirdWinDriver

arBirdWinDriver::arBirdWinDriver()
{}

bool arBirdWinDriver::init(arSZGClient&) {
  ar_log_warning() << "arBirdWinDriver disabled: flock-of-birds driver not installed.\n";
  return false;
}

bool arBirdWinDriver::start(){
  return false;
}

bool arBirdWinDriver::stop(){
  return true;
}

#else

arBirdWinDriver::arBirdWinDriver() :
  _flockWoken( false ),
  _streamingStarted( false ),
  _groupID( 1 ),
  _readTimeout( 2000 ),
  _writeTimeout( 2000 )
{}

bool arBirdWinDriver::start(){
  return _eventThread.beginThread(ar_WinBirdDriverEventTask,this);
}

bool arBirdWinDriver::stop(){
  if (_flockWoken) {
    if (_streamingStarted) {
        birdStopFrameStream(_groupID);
        // ar_log_remark() << "arBirdWinDriver remark: Stopped data stream.\n";
    }
    birdShutDown(_groupID);
    // ar_log_remark() << "arBirdWinDriver remark: Flock shut down.\n";    
  }
  return true;
}

void ar_WinBirdDriverEventTask(void* FOBDriver) {
  arBirdWinDriver* fobDriver = (arBirdWinDriver*) FOBDriver;
  BOOL status = birdStartFrameStream( fobDriver->_groupID );
  if (!status) {
    ar_log_warning() << "arBirdWinDriver failed to start data stream.\n";
    fobDriver->stop();
    return;
  }
  // ar_log_remark() << "arBirdWinDriver remark: Started data stream.\n";
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
      
      // Note: we've been reporting the inverse of the correct orientation
      // component. The two lines below correct this.
      arMatrix4 quatMatrix( arQuaternion(quat[0],quat[1],quat[2],quat[3]).inverse() );
      arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );

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
        
        // Note: we've been reporting the inverse of the correct orientation
        // component. The two lines below correct this.
        arMatrix4 quatMatrix( arQuaternion(quat[0],quat[1],quat[2],quat[3]).inverse() );
        arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );

        fobDriver->queueMatrix( i-1, fobMatrix );
      }
    }
    fobDriver->sendQueue();
  }
}

bool arBirdWinDriver::init(arSZGClient& SZGClient) {
#ifndef AR_USE_WIN_32
  // do lots of complicated stuff to wake up the flock.
  ar_log_warning() << "arBirdWinDriver error: implemented only for win32.\n  (Try arFOBDriver instead.)";
  return false;
#else
  const int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
  const BYTE hemiNums[] = {BHC_FRONT,BHC_REAR,BHC_UPPER,BHC_LOWER,BHC_LEFT,BHC_RIGHT};
  const string hemispheres[] = {"front","rear","upper","lower","left","right"};
  string received = SZGClient.getAttribute("SZG_FOB", "com_ports");
  char receivedBuffer[512];
  int intComPorts[_FOB_MAX_DEVICES];
  ar_stringToBuffer( received, receivedBuffer, sizeof(receivedBuffer) );
  _numDevices = ar_parseIntString( receivedBuffer, intComPorts, _FOB_MAX_DEVICES );
  if (_numDevices < 1) {
    ar_log_warning() << "arBirdWinDriver error: SZG_FOB/com_ports must contain an "
         << "entry for each bird and ERC.\n"
         << "     (use '0' if a given device doesn't have a direct serial "
	       << "connection).\n";
    return false;
  }
  if (_numDevices > _FOB_MAX_DEVICES) {
    ar_log_warning() << "arBirdWinDriver::init() error: (SZG_FOB/com_ports):\n"
         << "     To use more than " << _FOB_MAX_DEVICES 
         << " devices, change _FOB_MAX_DEVICES in arBirdWinDriver.h and rebuild.\n";
    return false;
  }
  _setDeviceElements( 0, 0, _numDevices );
  _standAlone = (_numDevices == 1);
  int i = 0;
  if (_standAlone) {  // for standalone, we put the COM port # at array position 0
    ar_log_remark() << "arBirdWinDriver remark: standalone configuration using COM port " << intComPorts[0] << "\n";
    _comPorts[0] = static_cast<WORD>( intComPorts[0] );
  } else {   // for multiple, we start loading the COM port #s into array at position 1
    ar_log_remark() << "arBirdWinDriver remark: COM ports: ";
    for (i=0; i<_numDevices; i++) {
      ar_log_remark() << intComPorts[i] << "/";
      _comPorts[i+1] = static_cast<WORD>(intComPorts[i]);
    }
    ar_log_remark() << "\n";
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
    ar_log_warning() << "arBirdWinDriver warning: illegal value for SZG_FOB/baud_rate.\n"
         << "  Legal values are:";
    for (i=0; i<_nBaudRates; i++)
      ar_log_warning() << " " << baudRates[i];
    _baudRate = 115200;
    ar_log_warning() << "\n   Defaulting to " << _baudRate << ".\n";
  }
  string hemisphere(SZGClient.getAttribute("SZG_FOB", "hemisphere"));
  int hemiFound = -1;
  BYTE hemisphereNum;
  for (i=0; i<_nHemi; i++)
    if (hemisphere == hemispheres[i])
      hemiFound = i;
  if (hemiFound==-1) {
    ar_log_warning() << "arBirdWinDriver warning: illegal value for SZG_FOB/hemisphere.\n"
         << "     Legal values are:";
    for (i=0; i<_nHemi; i++)
      ar_log_warning() << " " << hemispheres[i];
    hemisphere = "front";
    ar_log_warning() << "\n   Defaulting to " << hemisphere << "\n";
    hemiFound = 0;
  }
  _hemisphereNum = hemiNums[hemiFound];
  
  if (!birdRS232WakeUp(_groupID,_standAlone,_numDevices,_comPorts,
                       _baudRate,_readTimeout,_writeTimeout)) {
    ar_log_warning() << "arBirdWinDriver error: failed to wake up the flock.\n";
    return false;
  }
  ar_log_remark() << "arBirdWinDriver remark: Woke up flock consisting of " 
       << _numDevices << " devices.\n";
  _flockWoken = true;
  BOOL status = birdGetSystemConfig(_groupID,&_sysConfig);
  if (!status) {
    ar_log_warning() << "arBirdWinDriver error: failed to get flock's system config.\n";
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
      ar_log_warning() << "arBirdWinDriver error: Number of devices found, "
           << devcnt
	   << ", does not equal number specified, "
	   << _numDevices
	   << ".\n";
      return false;
    }
  }
  if (_standAlone) {
    status = birdGetDeviceConfig(_groupID,0,&_devConfig[0]);
    if (!status) {
      ar_log_warning() << "arBirdWinDriver error: failed to get config for the bird\n" ;
      return false;
    }
    _devConfig[0].byDataFormat = BDF_POSITIONQUATERNION;
    _devConfig[0].byHemisphere = _hemisphereNum;
    status = birdSetDeviceConfig(_groupID,0,&_devConfig[0]);
    if (!status) {
      ar_log_warning() << "arBirdWinDriver error: failed to set position/quaternion "
     << "mode + hemisphere for the bird" << "\n";
      return false;
    }
  } else {
    for (i=1; i<=_numDevices; i++) {
      status = birdGetDeviceConfig(_groupID,i,&_devConfig[i]);
      if (!status) {
        ar_log_warning() << "arBirdWinDriver error: failed to get config for device #" 
             << i << "\n";
        return false;
      }
      // ar_log_remark() << "arBirdWinDriver remark: Got status for device #" << i << "\n";
    }
    for (i=1; i<=_numDevices; i++) {
      _devConfig[i].byDataFormat = BDF_POSITIONQUATERNION;
      _devConfig[i].byHemisphere = _hemisphereNum;
      status = birdSetDeviceConfig(_groupID,i,&_devConfig[i]);
      if (!status) {
        ar_log_warning() << "arBirdWinDriver error: failed to set position/quaternion "
       << "mode + hemisphere for device #" << i << "\n";
        return false;
      }
    }
  }
  return true;
#endif
}

#endif
