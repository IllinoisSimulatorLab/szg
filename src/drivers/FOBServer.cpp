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

//#ifdef AR_USE_LINUX
//#include <fcntl.h>
//#include <stdio.h> // for perror()
//#include <errno.h> // for perror()
//#endif

#ifdef EnableBirdWinDriver
#ifdef AR_USE_WIN_32
#include "Bird.h"
#endif     
#endif

#include "arMath.h"
#include "arThread.h"
#include "arInputNode.h"
#include "arGenericDriver.h"
#include "arNetInputSink.h"

arSZGClient szgClient;
arGenericDriver inputServer;
arNetInputSink  netInputSink;
arInputNode     inputNode;

const double INCHES_TO_FEET = 1./12.;

const int FOB_MAX_DEVICES=31;  // flock can handle up to 30 sensors and an ERC,
                               // so this should be OK.
const int groupID = 1;
int numDevices = -1;

#ifdef AR_USE_WIN_32
WORD comPorts[FOB_MAX_DEVICES];
DWORD baudRate = -1;
DWORD readTimeout = 2000;
DWORD writeTimeout = 2000;
#ifdef EnableBirdWinDriver
BIRDSYSTEMCONFIG sysConfig;
BIRDDEVICECONFIG devConfig[FOB_MAX_DEVICES];
#endif
#else
int comPorts[FOB_MAX_DEVICES];
int baudRate = -1;
int readTimeout = 2000;
int writeTimeout = 2000;
#endif

const int nBaudRates = 7;
int baudRates[] = {2400,4800,9600,19200,38400,57600,115200};
const int nHemi = 6;
string hemispheres[] = {"front","rear","upper","lower","left","right"};
#ifdef AR_USE_WIN_32
#ifdef EnableBirdWinDriver
BYTE hemiNums[] = {BHC_FRONT,BHC_REAR,BHC_UPPER,BHC_LOWER,BHC_LEFT,BHC_RIGHT};
#endif
#endif

bool flockWoken = false;
bool streamingStarted = false;

void stop(void) {
#ifdef EnableBirdWinDriver
  if (flockWoken) {
    if (streamingStarted) {
        birdStopFrameStream(groupID);
        cerr << "FOBServer remark: Stopped data stream.\n";
    }
    birdShutDown(groupID);
    cerr << "FOBServer remark: Flock shut down.\n";    
  }
#endif
}

void cleanExit(int arg) {
  stop();
  exit(arg);
}

bool init(arSZGClient& szgClient) {
  stringstream& initResponse = szgClient.initResponse();
  const string received(szgClient.getAttribute("SZG_FOB", "com_ports"));
  if (received == "NULL") {
    initResponse << "FOBServer error: SZG_FOB/com_ports undefined.\n";
    return false;
  }

  char receivedBuffer[512];
  int intComPorts[FOB_MAX_DEVICES];
  ar_stringToBuffer( received, receivedBuffer, sizeof(receivedBuffer) );
  numDevices = ar_parseIntString( receivedBuffer, intComPorts, 
                                 FOB_MAX_DEVICES );
//intComPorts[0] = 1;
  if (numDevices < 1) {
    initResponse << "FOBServer error: SZG_FOB/com_ports must contain an "
		 << "entry for each bird and ERC\n"
                 << " (use 0 for devices with no direct serial connection).\n";
    return false;
  }
  if (numDevices > FOB_MAX_DEVICES) {
    initResponse << "FOBServer error (SZG_FOB/com_ports):\n"
                 << "     To use more than " << FOB_MAX_DEVICES 
                 << " devices, change FOB_MAX_DEVICES in FOBServer.cpp "
		 << "and rebuild.\n";
    return false;
  }
  initResponse << "FOBServer remark: COM ports: ";
  int i;
  for (i=0; i<numDevices; i++) {
    initResponse << intComPorts[i] << "/";
#ifdef AR_USE_WIN_32
    comPorts[i] = static_cast<WORD>(intComPorts[i]);
#endif
  }
  initResponse << endl;
  baudRate = szgClient.getAttributeInt("SZG_FOB", "baud_rate");
  bool baudRateFound = false;
  for (i=0; i<nBaudRates; i++)
    if (baudRate == baudRates[i])
      baudRateFound = true;
  if (!baudRateFound) {
    initResponse << "FOBServer warning: illegal value for SZG_FOB/baud_rate.\n"
                 << "  Legal values are:";
    for (i=0; i<nBaudRates; i++)
      initResponse << " " << baudRates[i];
    baudRate = 115200;
    initResponse << "\n   Defaulting to " << baudRate << ".\n";
  }
  string hemisphere = szgClient.getAttribute("SZG_FOB", "hemisphere");
  int hemiFound = -1;
  for (i=0; i<nHemi; i++)
    if (hemisphere == hemispheres[i])
      hemiFound = i;
  if (hemiFound == -1) {
    initResponse << "FOBServer warning: illegal value for "
		 << "SZG_FOB/hemisphere.\n"
                 << "     Legal values are:";
    for (i=0; i<nHemi; i++)
      initResponse << " " << hemispheres[i];
    hemisphere = "front";
    initResponse << "\n   Defaulting to " << hemisphere << endl;
    hemiFound = 0;
  }
#ifdef AR_USE_WIN_32
#ifdef EnableBirdWinDriver
  BYTE hemisphereNum = hemiNums[hemiFound];
#endif
#endif

#ifdef EnableBirdWinDriver
  BOOL standAlone = (numDevices == 1);
  if (!birdRS232WakeUp(groupID,standAlone,numDevices,comPorts,
                       baudRate,readTimeout,writeTimeout)) {
    initResponse << "FOBServer error: failed to wake up flock.\n";
    return false;
  }
  initResponse << "FOBServer remark: woke up flock of " 
               << numDevices << " devices.\n";
  flockWoken = true;
  BOOL status = birdGetSystemConfig(groupID,&sysConfig);
  if (!status) {
    initResponse << "FOBServer error: failed to get flock's system config.\n";
    return false;
  }

  if (!standAlone) {
    // check to see how many devices are really present
    int devcnt = 0;
    for (i=0, devcnt= 0; i < FOB_MAX_DEVICES; i++) {
      if (sysConfig.byFlockStatus[i] & BFS_FBBACCESSIBLE)
        ++devcnt;
    }
    if (devcnt != numDevices) {
      initResponse << "FOBServer error: Number of devices found, " << devcnt
	           << ", does not equal number specified, " 
                   << numDevices << ".\n";
      return false;
    }
  }
  for (i=0; i<numDevices; i++) {
    status = birdGetDeviceConfig(groupID,i,&devConfig[i]);
    if (!status) {
      initResponse << "FOBServer error: Unable to get config for device #" 
                   << i << endl;
      return false;
    }
    initResponse << "FOBServer remark: Got status for device #" << i << endl; 
  }
  for (i=0; i<numDevices; i++) {
    devConfig[i].byDataFormat = BDF_POSITIONQUATERNION;
    devConfig[i].byHemisphere = hemisphereNum;
    status = birdSetDeviceConfig(groupID,i,&devConfig[i]);
    if (!status) {
      initResponse << "FOBServer error: Unable to set "
		   << "position/quaternion mode + hemisphere for device #" 
                   << i << endl;
      return false;
    }
  }
  return true;

#else
  initResponse << "FOBServer error: disabled because flock-of-birds "
	       << "library is not installed.\n";
  return false;
#endif
}

void start(void) {
#ifdef EnableBirdWinDriver
#ifdef AR_USE_WIN_32
  BOOL status = birdStartFrameStream(groupID);
  if (!status) {
    cerr << "FOBServer error: Unable to start data stream.\n";
    cleanExit(-1);
  }
  cerr << "FOBServer remark: Started data stream.\n";
  streamingStarted = true;
  while (true) {
    ar_usleep(5000);
    if (!birdFrameReady(groupID))
      continue;

    BIRDFRAME frame;
    birdGetMostRecentFrame(groupID,&frame);
    for (int i=0; i<numDevices; i++) {
      const float posScale 
        = float(devConfig[i].wScaling) * INCHES_TO_FEET / 32767.;

      BIRDREADING* preading;
      // reading array is indexed by device address (1 relative)
      preading = &frame.reading[i]; 

      // convert position and angle data
      const arVector3 pos(
	preading->position.nX * posScale,
	preading->position.nY * posScale,
	preading->position.nZ * posScale);
      const double quat[4] = {
	preading->quaternion.nQ0 / 32767.,
	preading->quaternion.nQ1 / 32767.,
	preading->quaternion.nQ2 / 32767.,
	preading->quaternion.nQ3 / 32767. };
      const arMatrix4 quatMatrix( arQuaternion(quat[0],quat[1],
                                               quat[2],quat[3]) );
      const arMatrix4 fobMatrix( ar_translationMatrix(pos)*quatMatrix );
      inputServer.queueMatrix( i, fobMatrix );
    }
    inputServer.sendQueue();
  }
#endif
#else
  // Linux code to get position & quaternion, convert to matrix, send.
#endif
}

int main(int argc, char** argv) {
#ifdef EnableBirdWinDriver
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;
  if (!init(szgClient)){
    szgClient.sendInitResponse(false);
    stop();
    return 1;
  }
  // we succeeded in initing
  szgClient.sendInitResponse(true);
  
    
  // set up the language and start sending data
  inputServer.setSignature(0,0,numDevices);
  netInputSink.setSlot(0);
  inputNode.addInputSource(&inputServer,false);
  inputNode.addInputSink(&netInputSink,false);
  inputNode.init(szgClient);
  if (!inputNode.start()) {
    szgClient.sendStartResponse(false);
    stop();
    return 1;
  }

  // we succeeded in starting
  szgClient.sendStartResponse(true);

  arThread dummy(ar_messageTask, &szgClient);
  start();
  // This line may never be reached.
  stop();
  return 0;
#else
  cerr << " FOBServer error: disabled because flock-of-birds library "
       << "is not installed.\n";
  return 1;
#endif
}
