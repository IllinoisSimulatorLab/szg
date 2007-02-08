//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_MEM_SINK_DRIVER_H
#define AR_SHARED_MEM_SINK_DRIVER_H

#include "arInputSink.h"
#include "arThread.h"
#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

/// Driver for SGI shared-memory segments emulating "trackd" mocap data.
/// Collect data from various other things.

class SZG_CALL arSharedMemSinkDriver: public arInputSink {
    friend void ar_ShmDriverDataTask(void*);
  public:
    arSharedMemSinkDriver();
    ~arSharedMemSinkDriver();
    
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
    int _buttonPrev[10];
    arThread _eventThread;
};

#endif
