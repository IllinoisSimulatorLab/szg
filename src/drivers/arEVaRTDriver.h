//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_EVART_DRIVER_H
#define AR_EVART_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arInputHeaders.h"
#include "arSocket.h"
#include "arDataUtilities.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <list>
using namespace std;

/// Driver for Motion Analysis EVaRT system

class arEVaRTDriver: public arInputSource {
  friend int ar_evartDataHandler(int,void*);
 public:
  arEVaRTDriver();
  ~arEVaRTDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();
  bool restart();

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
