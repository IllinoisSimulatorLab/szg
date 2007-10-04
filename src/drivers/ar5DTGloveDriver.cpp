//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "ar5DTGloveDriver.h"
#include "fglove.h"

DriverFactory(ar5DTGloveDriver, "arInputSource")

static fdGlove *__pGlove    = NULL;

string __getGloveType(void) {
  if (!__pGlove) {
    ar_log_error() << "Called __getGloveType() with NULL glove pointer.\n";
    return "NULL";
  }
	int glovetype = FD_GLOVENONE;
	glovetype = fdGetGloveType( __pGlove );
	switch (glovetype) {
	case FD_GLOVENONE: 
    return "None";
	case FD_GLOVE7:
    return "Glove7";
	case FD_GLOVE7W:
    return "Glove7W";
	case FD_GLOVE16:
    return "Glove16";
	case FD_GLOVE16W:
    return "Glove16W";
	case FD_GLOVE5U:
    return "DG5 Ultra serial";
	case FD_GLOVE5UW:
    return "DG5 Ultra serial, wireless";
	case FD_GLOVE5U_USB:
    return "DG5 Ultra USB";
	case FD_GLOVE14U:
    return "DG14 Ultra serial";
	case FD_GLOVE14UW:
    return "DG14 Ultra serial, wireless";
	case FD_GLOVE14U_USB:
    return "DG14 Ultra USB";
  default:
    return "Unknown";
	}
}

string __getGloveHandedNess(void) {
  if (!__pGlove) {
    ar_log_error() << "Called __getGloveHandedNess() with NULL glove pointer.\n";
    return "NULL";
  }
  return fdGetGloveHand( __pGlove )== (FD_HAND_RIGHT) ? ("right"):("left");
}


#ifdef Enable5DT
void ar_5dtGloveDriverEventTask(void* gloveDriver){
  ar5DTGloveDriver* g = (ar5DTGloveDriver*) gloveDriver;

  if (__pGlove == NULL) {
    ar_log_error() << "ar_5dtGloveDriverEventTask() started with NULL glove pointer, exiting...\n";
    return;
  }

  float* sensorValues = new float[g->_numSensors];
  if (sensorValues == NULL) {
    ar_log_error() << "ar_5dtGloveDriverEventTask() failed to allocate sensor buffer, exiting...\n";
    g->stop();
    return;
  }

  // Poll dataglove.
  while (true){
    ar_usleep(20000); // 50 Hz, about

    fdGetSensorScaledAll( __pGlove, sensorValues );
    for (int i=0; i<g->_numSensors; ++i) {
      g->queueAxis( i, sensorValues[i] );
    }

    int currentGesture = fdGetGesture( __pGlove );
    if (currentGesture != g->_lastGesture) {
      if (g->_lastGesture != -1) {
        g->queueButton( g->_lastGesture, 0 );
      }
      if (currentGesture != -1) {
        g->queueButton( currentGesture, 1 );
      }
      g->_lastGesture = currentGesture;
    }

    // Send all this accumulated data...
    g->sendQueue();
  }
  delete[] sensorValues;
}
#endif

bool ar5DTGloveDriver::init(arSZGClient& szgClient){
  int sig[3] = {0,0,0};
#ifndef Enable5DT
  ar_log_error() << "5DT dataglove support not compiled.\n";
  return false;
#else
  string deviceName = szgClient.getAttribute( "SZG_5DT", "port_name" );
  if (deviceName == "NULL") {
#ifdef AR_USE_WIN_32
    deviceName = "USB0";
#else
    deviceName = "/dev/fglove";
#endif
  }
  __pGlove = fdOpen( const_cast<char *>(deviceName.c_str()) );
	if (__pGlove == NULL) {
    ar_log_error() << "Failed to open 5DT dataglove.\n";
		return false;
	}
  ar_log_remark() << "Opened 5DT dataglove.\n";
  ar_log_remark() << "DataGlove type: " << __getGloveType() << ar_endl;
  ar_log_remark() << "DataGlove hand: " << __getGloveHandedNess() << ar_endl;

	_numSensors = fdGetNumSensors( __pGlove );
	_numGestures = fdGetNumGestures( __pGlove );

  ar_log_remark() << "DataGlove has " << _numSensors << " sensors and "
                  << _numGestures << " gestures.\n";
  
  sig[0] = _numGestures;
  sig[1] = _numSensors;

  _setDeviceElements(sig);
  _lastGesture = -1;

  return true;
#endif
}

bool ar5DTGloveDriver::start(){
#ifndef Enable5DT
  ar_log_error() << "5DT dataglove support not compiled.\n";
  return false;
#else
  return _eventThread.beginThread(ar_5dtGloveDriverEventTask,this);
#endif
}

bool ar5DTGloveDriver::stop(){
#ifndef Enable5DT
  ar_log_error() << "5DT dataglove support not compiled.\n";
  return false;
#else
  if (__pGlove == NULL) {
    ar_log_error() << "stop() started with NULL glove pointer, exiting...\n";
    return true;
  }

	fdClose( __pGlove );
	printf( "DataGlove closed.\n" );
  return true;
#endif
}



