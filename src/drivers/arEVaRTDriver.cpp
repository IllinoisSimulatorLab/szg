//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#ifdef EnableEVaRT
  #include "EVaRT.h"
#endif
#include "arEVaRTDriver.h"

DriverFactory(arEVaRTDriver, "arInputSource")

arEVaRTDriver::arEVaRTDriver() :
  _receivedEVaRTHierarchy(false),
  _deviceIP("NULL") {
}

#ifndef EnableEVaRT

bool arEVaRTDriver::init(arSZGClient&) {
  ar_log_warning() << "arEVaRTDriver unsupported on this platform.\n";
  return false;
}

bool arEVaRTDriver::start() {
  ar_log_warning() << "arEVaRTDriver unsupported on this platform.\n";
  return false;
}

#else

arEVaRTDriver* __globalEVaRTDriver = NULL;

int ar_evartDataHandler(int iType, void* data){
  int i = 0;
  if (iType == HIERARCHY){
    ar_log_remark() << "arEVaRTDriver got hierarchy data packet.\n";
    sHierarchy* Hierarchy = (sHierarchy*) data;
    __globalEVaRTDriver->_numberSegments = Hierarchy->nSegments;
    ar_log_remark() << "arEVaRTDriver Number segments = " 
         << __globalEVaRTDriver->_numberSegments << "\n";

    __globalEVaRTDriver->_children 
      = new list<int>[__globalEVaRTDriver->_numberSegments];
    __globalEVaRTDriver->_segTransform 
      = new arMatrix4[__globalEVaRTDriver->_numberSegments];
    __globalEVaRTDriver->_segLength 
      = new float[__globalEVaRTDriver->_numberSegments];
    __globalEVaRTDriver->_segName 
      = new string[__globalEVaRTDriver->_numberSegments];

    for (i=0; i< __globalEVaRTDriver->_numberSegments; i++){
      __globalEVaRTDriver->_segName[i] = string(Hierarchy->szSegmentNames[i],
                                         strlen(Hierarchy->szSegmentNames[i]));
      ar_log_remark() << "Segment name " << i << ": " 
           << __globalEVaRTDriver->_segName[i] << "\n";
      int parent =  Hierarchy->iParents[i];
      if (parent == -1){
        // this must be the rootnode
        __globalEVaRTDriver->_rootNode = i;
        ar_log_remark() << "arEVaRTDriver root node ID = " << i << "\n";
      }
      else{
        __globalEVaRTDriver->_children[parent].push_back(i);
      }
    }
    __globalEVaRTDriver->_receivedEVaRTHierarchy = true;
  }

  if (iType == TRC_DATA){
    sTrcFrame* TrcFrame = (sTrcFrame*)data;
    for (i=0; i<40; i++){
      // garbage data gets placed out at infinity by the EVaRT software
      if (fabs(TrcFrame->Markers[i][0]) < 100000. &&
	  fabs(TrcFrame->Markers[i][1]) < 100000. &&
	  fabs(TrcFrame->Markers[i][2]) < 100000.){
	// data isn't garbage
        __globalEVaRTDriver->queueAxis(3*i  , TrcFrame->Markers[i][0]);
	__globalEVaRTDriver->queueAxis(3*i+1, TrcFrame->Markers[i][1]);
	__globalEVaRTDriver->queueAxis(3*i+2, TrcFrame->Markers[i][2]);
      }
    }
    __globalEVaRTDriver->sendQueue();
  }

  else if (iType == GTR_DATA){
    sGtrFrame* GtrFrame = (sGtrFrame*) data;
    for (i=0; i<__globalEVaRTDriver->_numberSegments; i++){
      const float angleX = GtrFrame->Segments[i][3];
      const float angleY = GtrFrame->Segments[i][4];
      const float angleZ = GtrFrame->Segments[i][5];
      const arMatrix4 sendMatrix(ar_translationMatrix(
	  GtrFrame->Segments[i][0],
	  GtrFrame->Segments[i][1],
	  GtrFrame->Segments[i][2]) *
	ar_rotationMatrix('z', ar_convertToRad(angleZ)) *
	ar_rotationMatrix('y', ar_convertToRad(angleY)) *
	ar_rotationMatrix('x', ar_convertToRad(angleX)));
      __globalEVaRTDriver->queueMatrix(i, sendMatrix);
      __globalEVaRTDriver->queueAxis(i, GtrFrame->Segments[i][6]);
    }
    __globalEVaRTDriver->sendQueue();
  }
#if 0
  else if (iType == HTR_DATA){
    sHtrFrame* HtrFrame = (sHtrFrame*) data;
    float tempRoot[3];
    tempRoot[0] = HtrFrame->RootPosition[0];
    tempRoot[1] = HtrFrame->RootPosition[1];
    tempRoot[2] = HtrFrame->RootPosition[2];
    if (fabs(tempRoot[0]) < 100000 && fabs(tempRoot[1]) < 100000 
        && fabs(tempRoot[2]) < 100000){  
      __globalEVaRTDriver->_rootPosition[0] = tempRoot[0];
      __globalEVaRTDriver->_rootPosition[1] = tempRoot[1];
      __globalEVaRTDriver->_rootPosition[2] = tempRoot[2];
    }
    for (i=0; i< __globalEVaRTDriver->_numberSegments; i++){
      __globalEVaRTDriver->_segLength[i] = HtrFrame->Segments[i][3];
      const float angleX = HtrFrame->Segments[i][0];
      const float angleY = HtrFrame->Segments[i][1];
      const float angleZ = HtrFrame->Segments[i][2];
      arMatrix4 rotationMatrix;
      if (fabs(angleX) < 100000 && fabs(angleY) < 100000 && fabs(angleZ) < 100000){
        rotationMatrix =
          ar_rotationMatrix('z', ar_convertToRad(angleZ)) *
	  ar_rotationMatrix('y', ar_convertToRad(angleY)) *
	  ar_rotationMatrix('x', ar_convertToRad(angleX)));
      }
      if (i == __globalEVaRTDriver->_rootNode){
        __globalEVaRTDriver->_segTransform[i] = 
          ar_translationMatrix(__globalEVaRTDriver->_rootPosition[0],
			       __globalEVaRTDriver->_rootPosition[1],
			       __globalEVaRTDriver->_rootPosition[2]) *
	  rotationMatrix;
      }
      else{
        __globalEVaRTDriver->_segTransform[i] = rotationMatrix;
      }
      __globalEVaRTDriver
        ->queueMatrix(i,__globalEVaRTDriver->_segTransform[i]);
      __globalEVaRTDriver->queueAxis(3*i, ar_convertToRad(angleX));
      __globalEVaRTDriver->queueAxis(3*i+1, ar_convertToRad(angleY));
      __globalEVaRTDriver->queueAxis(3*i+2, ar_convertToRad(angleZ));
    }
    __globalEVaRTDriver->sendQueue();
  }
#endif
  return 0;
}

bool arEVaRTDriver::init(arSZGClient& SZGClient){
  __globalEVaRTDriver = this;
  if (SZGClient.getAttribute("SZG_EVART", "output_type") == "position"){
    _outputType = string("position");
    // allocate space for the x,y,z of 40 markers
    _setDeviceElements(0,120,0);
  }
  else{
    _outputType = string("skeleton");
    // allocate space for 30 matrices
    _setDeviceElements(0,90,30);
  }
  
  _deviceIP = SZGClient.getAttribute("SZG_EVART", "IPhost");
  if (_deviceIP == "NULL"){
    ar_log_warning() << "arEVaRTDriver: no SZG_EVART/IPhost IP address for EVaRT.\n";
    return false;
  }
  return true;
}

bool arEVaRTDriver::start(){
  EVaRT_Initialize();
  EVaRT_SetDataHandlerFunc(ar_evartDataHandler);
  char buffer[256];
  ar_stringToBuffer(_deviceIP,buffer,256);
  if (EVaRT_Connect(buffer)){
    ar_log_debug() << "arEVaRTDriver Connected to EVaRT.\n";
  }
  else{
    ar_log_warning() << "arEVaRTDriver failed to connect to EVaRT.\n";
    return false;
  }

  if (_outputType == "skeleton"){
    EVaRT_SetDataTypesWanted(GTR_DATA);
    EVaRT_RequestHierarchy(); // needed before data is usable
    ar_log_debug() << "arEVaRTDriver: Hierarchy Requested.\n";

    arSleepBackoff a(20, 150, 1.05);
    while (!_receivedEVaRTHierarchy)
      a.sleep();

    ar_log_debug() << "arEVaRTDriver: Hierarchy Received.\n";
  }
  else{
    // Ask the EVaRT for the markers' positions.
    ar_log_debug() << "arEVaRTDriver requesting position data.\n";
    EVaRT_SetDataTypesWanted(TRC_DATA);
  }

  EVaRT_StartStreaming();
  return true;
}

#endif
