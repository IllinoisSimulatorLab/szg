//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NET_INPUT_SINK_H
#define AR_NET_INPUT_SINK_H

#include "arInputSink.h"
#include "arInputLanguage.h"
#include "arDataServer.h"

/// Something that absorbs input-device messages over ethernet.

class arNetInputSink: public arInputSink{
  friend void ar_netInputSinkConnectionTask(void*);
 public:
  arNetInputSink();
  ~arNetInputSink() {}

  void setSlot(int slot);

  bool init(arSZGClient&);
  bool start();
  bool stop();
  bool restart();

  virtual void receiveData(int,arStructuredData*);

  void setInfo(const string& info);
 private:
  arSZGClient* _client;
  arDataServer _dataServer;
  int _slot;
  string _interface;
  int _port;
  arInputLanguage _inp;
  bool _fValid;
  string _info;
};

#endif
