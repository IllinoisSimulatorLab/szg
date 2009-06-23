//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_REACTION_TIMER_DRIVER_H
#define AR_REACTION_TIMER_DRIVER_H

#include "arInputSource.h"
#include "arRS232Port.h"

#include "arDriversCalling.h"

// Driver for the accurate Reaction Timer system.

class SZG_CALL arReactionTimerDriver: public arInputSource {
    friend void ar_RTDriverEventTask(void*);
  public:
    arReactionTimerDriver();
    ~arReactionTimerDriver();
    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();

  private:
    bool _processInput();
    void _resetStatusTimer();
    bool _inited;
    bool _imAlive;
    bool _stopped;
    bool _eventThreadRunning;
    arThread _eventThread;
    unsigned int _portNum;
    arRS232Port _port;
    char *_inbuf;
    std::string _bufString;
    arTimer _statusTimer;
    arTimer _rtTimer;
};

#endif
