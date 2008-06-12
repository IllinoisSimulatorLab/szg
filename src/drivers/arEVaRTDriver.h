//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_EVART_DRIVER_H
#define AR_EVART_DRIVER_H

#include "arInputSource.h"
#include "arInputHeaders.h"

#include "arDriversCalling.h"

#include <list>
using namespace std;

// Driver for Motion Analysis's EVaRT optical mocap.

class arEVaRTDriver: public arInputSource {
  friend int ar_evartDataHandler(int, void*);
 public:
  arEVaRTDriver();

  bool init(arSZGClient&);
  bool start();

 private:
  arThread _eventThread;
  int _rootNode;
  int _numberSegments;
  arVector3 _rootPosition;
  list<int>* _children;
  arMatrix4*  _segTransform;
  float*  _segLength;
  string* _segName;
  bool _receivedEVaRTHierarchy;
  string _deviceIP;
  string _outputType;
};

#endif
