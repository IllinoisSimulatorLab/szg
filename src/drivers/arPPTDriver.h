//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PPT_TIMER_DRIVER_H
#define AR_PPT_TIMER_DRIVER_H

#include "arInputSource.h"
#include "arThread.h"
#include "arMath.h"
#include "arRS232Port.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

/// Driver for the accurate Reaction Timer system.

class SZG_CALL arPPTDriver: public arInputSource {
    friend void ar_PPTDriverEventTask(void*);
  public:
    arPPTDriver();
    ~arPPTDriver();
    
    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();
    bool restart();
  
  private:
    bool _processInput();
    float _unstuffBytes(float max, unsigned char *storage);
    unsigned char _checksum(unsigned char* buffer, int length);
    void _resetStatusTimer();
    bool _inited;
    bool _imAlive;
    bool _stopped;
    bool _eventThreadRunning;
    arThread _eventThread;
    unsigned int _portNum;
    arRS232Port _port;
    char *_inbuf;
    arTimer _statusTimer;
};

#endif        //  #ifndefARPPTTIMERDRIVER_H


