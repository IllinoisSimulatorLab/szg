//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_MEM_DRIVER_H
#define AR_SHARED_MEM_DRIVER_H

#include "arThread.h"
#include "arMath.h"
#include "arDriversCalling.h"

// Driver for shared-memory segments holding mocap data from VRCO trackd.

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
    arMutex _lockShm;
    int _buttonPrev[8];
    arThread _eventThread;
};

#endif
