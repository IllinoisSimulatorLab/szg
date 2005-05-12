//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef arUSBDRIVER_H
#define arUSBDRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arMath.h"

/// Driver for USB cave-wand,
/// and possibly other USB "igorplug" Atmel-AVR devices.

class arUSBDriver: public arInputSource {
    friend void ar_USBDriverDataTask(void*);
  public:
    arUSBDriver();
    ~arUSBDriver();
    
    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();
    bool restart();
  
  private:
    void _dataThread();
    bool _inited;
    bool _stopped;
    bool _eventThreadRunning;
    arThread _eventThread;
};

#endif
