//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_USB_DRIVER_H
#define AR_USB_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

// Driver for USB cave-wand,
// and possibly other USB "igorplug" Atmel-AVR devices.

class SZG_CALL arUSBDriver: public arInputSource {
    friend void ar_USBDriverDataTask(void*);
  public:
    arUSBDriver();
    ~arUSBDriver();

    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();

  private:
    void _dataThread();
    bool _inited;
    bool _stopped;
    bool _eventThreadRunning;
    arThread _eventThread;
};

#endif
