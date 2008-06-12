//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_MEM_DRIVER_H
#define AR_SHARED_MEM_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

// Driver for reading shared-memory segments holding mocap data from VRCO trackd.

void ar_ShmDriverDataTask(void*);

class SZG_CALL arSharedMemDriver: public arInputSource {
  friend void ar_ShmDriverDataTask(void*);
 public:
  arSharedMemDriver();
  ~arSharedMemDriver();

  bool init(arSZGClient& SZGClient);
  bool start();
  bool stop();

 private:
  void _dataThread();
  void _detachMemory();
  bool _inited;
  bool _stopped;
  bool _eventThreadRunning;
  void* _shmFoB;
  void* _shmWand;
  arLock _l;
  int _buttonPrev[8];
  arThread _eventThread;
};

#endif
