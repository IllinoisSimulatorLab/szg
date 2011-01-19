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
  ar_log_error() << "arBirdWinDriver disabled: flock-of-birds driver not installed.\n";
  return false;
}

bool arBirdWinDriver::start() {
  return false;
}

bool arBirdWinDriver::stop() {
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

bool arBirdWinDriver::start() {
  return _eventThread.beginThread(ar_WinBirdDriverEventTask, this);
}

bool arBirdWinDriver::stop() {
  if (_flockWoken) {
    if (_streamingStarted) {
        birdStopFrameStream(_groupID);
        // ar_log_debug() << "arBirdWinDriver stopped data stream.\n";
    }
    birdShutDown(_groupID);
    // ar_log_debug() << "arBirdWinDriver shut down Flock.\n";
  }
  return true;
}

void ar_WinBirdDriverEventTask(void* FOBDriver) {
  arBirdWinDriver* fobDriver = (arBirdWinDriver*) FOBDriver;
  if (!birdStartFrameStream( fobDriver->_groupID )) {
    ar_log_error() << "arBirdWinDriver failed to start data stream.\n";
    fobDriver->stop();
    return;
  }

  // ar_log_debug() << "arBirdWinDriver started data stream.\n";
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
      arMatrix4 quatMatrix( arQuaternion(quat[0], quat[1], quat[2], quat[3]).inverse() );
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
        arMatrix4 quatMatrix( arQuaternion(quat[0], quat[1], quat[2], quat[3]).inverse() );
        arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );

        fobDriver->queueMatrix( i-1, fobMatrix );
      }
    }
    fobDriver->sendQueue();
  }
}

bool arBirdWinDriver::init(arSZGClient& SZGClient) {
#ifndef AR_USE_WIN_32
  ar_log_error() << "arBirdWinDriver implemented only for win32.\n  (Try arFOBDriver instead.)";
  return false;
#else
  // do lots of complicated stuff to wake up the flock.
  const unsigned baudRates[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200};
  const BYTE hemiNums[] = {BHC_FRONT, BHC_REAR, BHC_UPPER, BHC_LOWER, BHC_LEFT, BHC_RIGHT};
  const string hemispheres[] = {"front", "rear", "upper", "lower", "left", "right"};
  string received = SZGClient.getAttribute("SZG_FOB", "com_ports");
  char receivedBuffer[512];
  int intComPorts[_FOB_MAX_DEVICES];
  ar_stringToBuffer( received, receivedBuffer, sizeof(receivedBuffer) );
  _numDevices = ar_parseIntString( receivedBuffer, intComPorts, _FOB_MAX_DEVICES );
  if (_numDevices < 1) {
    ar_log_error() << "SZG_FOB/com_ports lacks one entry per bird and ERC (0 if no direct serial connection).\n";
    return false;
  }
  if (_numDevices > _FOB_MAX_DEVICES) {
    ar_log_error() << "more than " << _FOB_MAX_DEVICES <<
      " devices.  Increase _FOB_MAX_DEVICES in arBirdWinDriver.h and recompile.\n";
    return false;
  }
  _setDeviceElements( 0, 0, _numDevices );
  _standAlone = (_numDevices == 1);
  int i = 0;
  if (_standAlone) {  // for standalone, we put the COM port # at array position 0
    ar_log_remark() << "arBirdWinDriver: standalone configuration using COM port " << intComPorts[0] << "\n";
    _comPorts[0] = static_cast<WORD>( intComPorts[0] );
  } else {   // for multiple, we start loading the COM port #s into array at position 1
    ar_log_remark() << "arBirdWinDriver: COM ports: ";
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
    ar_log_error() << "unexpected SZG_FOB/baud_rate.  Try one of:";
    for (i=0; i<_nBaudRates; i++)
      ar_log_error() << " " << baudRates[i];
    _baudRate = 115200;
    ar_log_error() << "\n   Defaulting to " << _baudRate << ".\n";
  }
  string hemisphere(SZGClient.getAttribute("SZG_FOB", "hemisphere"));
  int hemiFound = -1;
  for (i=0; i<_nHemi; i++)
    if (hemisphere == hemispheres[i])
      hemiFound = i;
  if (hemiFound==-1) {
    ar_log_error() << "Unexpected SZG_FOB/hemisphere.  Try one of:";
    for (i=0; i<_nHemi; i++)
      ar_log_error() << " " << hemispheres[i];
    hemisphere = "front";
    ar_log_error() << "\n   Defaulting to " << hemisphere << "\n";
    hemiFound = 0;
  }
  _hemisphereNum = hemiNums[hemiFound];

  if (!birdRS232WakeUp(_groupID, _standAlone, _numDevices, _comPorts,
                       _baudRate, _readTimeout, _writeTimeout)) {
    ar_log_error() << "failed to wake up the flock.\n";
    return false;
  }

  ar_log_critical() << "woke Flock of Birds with " << _numDevices << " devices.\n";
  _flockWoken = true;
  if (!birdGetSystemConfig(_groupID, &_sysConfig)) {
    ar_log_error() << "failed to get flock's system config.\n";
    return false;
  }

  if (_standAlone) {
    if (!birdGetDeviceConfig(_groupID, 0, _devConfig)) {
      ar_log_error() << "failed to get bird config.\n";
      return false;
    }
    _devConfig->byDataFormat = BDF_POSITIONQUATERNION;
    _devConfig->byHemisphere = _hemisphereNum;
    if (!birdSetDeviceConfig(_groupID, 0, _devConfig)) {
      ar_log_error() << "failed to set bird's position/quaternion mode + hemisphere\n";
      return false;
    }
  } else {
    int devcnt = 0;
    for (i=0; i < _FOB_MAX_DEVICES; ++i) {
      if (_sysConfig.byFlockStatus[i] & BFS_FBBACCESSIBLE)
        ++devcnt;
    }
    if (devcnt != _numDevices) {
      ar_log_error() << "Found " << devcnt <<
        " devices, mismatching number specified, " << _numDevices << ".\n";
      for (i=1; i<=_numDevices; i++) {
        if (!birdGetDeviceConfig(_groupID, i, &_devConfig[i])) {
          ar_log_error() << "    failed to get config for device " << i << "\n";
        } else {
          ar_log_error() << "    Device " << i << ": ID="
            << (unsigned int)_devConfig[i].byID << ", status="
            << (unsigned int)_devConfig[i].byStatus << ", error code="
            << (unsigned int)_devConfig[i].byError << ar_endl;
        }
      }
      return false;
    }

    for (i=1; i<=_numDevices; i++) {
      if (!birdGetDeviceConfig(_groupID, i, &_devConfig[i])) {
        ar_log_error() << "failed to get config for device " << i << "\n";
        return false;
      }
    }
    for (i=1; i<=_numDevices; i++) {
      _devConfig[i].byDataFormat = BDF_POSITIONQUATERNION;
      _devConfig[i].byHemisphere = _hemisphereNum;
      if (!birdSetDeviceConfig(_groupID, i, &_devConfig[i])) {
        ar_log_error() <<
          "failed to set position/quaternion mode + hemisphere for device " << i << "\n";
        return false;
      }
    }
  }
  return true;
#endif
}

#endif
