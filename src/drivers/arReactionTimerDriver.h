//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARREACTIONTIMERDRIVER_H
#define ARREACTIONTIMERDRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arMath.h"
#include "arRS232Port.h"

/// Driver for the accurate Reaction Timer system.

class arReactionTimerDriver: public arInputSource {
    friend void ar_RTDriverEventTask(void*);
  public:
    arReactionTimerDriver();
    ~arReactionTimerDriver();
    
    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();
    bool restart();
  
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

#endif        //  #ifndefARREACTIONTIMERDRIVER_H

