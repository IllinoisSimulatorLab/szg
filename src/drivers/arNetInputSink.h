//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NET_INPUT_SINK_H
#define AR_NET_INPUT_SINK_H

#include "arInputSink.h"
#include "arInputLanguage.h"
#include "arDataServer.h"
#include "arDriversCalling.h"

// Absorb input-device messages.

class SZG_CALL arNetInputSink: public arInputSink{
  friend void ar_netInputSinkConnectionTask(void*);
 public:
  arNetInputSink();
  virtual ~arNetInputSink() {}

  bool setSlot(unsigned slot);
  bool init(arSZGClient&);
  bool start();
  void setInfo(const string& info);
  virtual void receiveData(int, arStructuredData*);
 private:
  arSZGClient* _szgClient;
  arDataServer _dataServer;
  unsigned _slot;
  string _interface;
  int _port;
  arInputLanguage _inp;
  bool _fValid;
  string _info;
};

#endif
