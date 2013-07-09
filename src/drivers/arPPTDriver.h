//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PPT_TIMER_DRIVER_H
#define AR_PPT_TIMER_DRIVER_H

#include "arInputSource.h"
#include "arRS232Port.h"

#include "arDriversCalling.h"

// Driver for WorldViz PPT position tracker.

class SZG_CALL arPPTDriver: public arInputSource {
    friend void ar_PPTDriverEventTask(void*);
  public:
    arPPTDriver();
    ~arPPTDriver();
    
    bool init(arSZGClient& SZGClient);
    bool start();
    bool stop();
  
  private:
    bool _processInput();
    float _unstuffBytes(float max, const unsigned char *storage);
    unsigned char _checksum(const unsigned char* buffer, int length) const;
    void _resetStatusTimer();
    bool _inited;
    bool _imAlive;
    bool _stopped;
    bool _eventThreadRunning;
    arThread _eventThread;
    unsigned int _portNum;
    arRS232Port _port;
    char *_inbuf;
    unsigned char *_packetBuf;
    unsigned int _packetOffset;
    unsigned int _lastNumLights;
    arTimer _statusTimer;
};

#endif
