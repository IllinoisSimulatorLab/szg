//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_MEM_SINK_DRIVER_H
#define AR_SHARED_MEM_SINK_DRIVER_H

#include "arInputSink.h"

#include "arDriversCalling.h"

// Driver for SGI shared-memory segments emulating "trackd" mocap data.
// Collect data from whatever slot arAppLauncher assigns to the DeviceServer

class SZG_CALL arSharedMemSinkDriver: public arInputSink {
    friend void ar_ShmSinkDriverDataTask(void*);
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
    arLock _l;
    arInputNode _node;
    arNetInputSource _source;
    int _buttonPrev[256];
    arThread _eventThread;
};

#endif
