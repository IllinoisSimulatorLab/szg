//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_WIIMOTE_DRIVER_H
#define AR_WIIMOTE_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"


class arWiimoteDriver: public arInputSource {
  public:
    arWiimoteDriver();
    bool init(arSZGClient& client);
    bool start();
    bool stop();
    void update();
    void updateStatus();
    void updateSignature( const int, const int, const int );
    void resetIdleTimer();
  protected:
    const char* _version;
    int _findTimeoutSecs;
    int _connected;
  private:
    bool _connect();
    void _disconnect();
    bool _useMotion;
    bool _useIR;
    arThread _eventThread;
    arTimer _idleTimer;
    arTimer _statusTimer;
};

#endif
