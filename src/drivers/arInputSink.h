//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_SINK_H
#define AR_INPUT_SINK_H

#include "arStructuredData.h"
#include "arSZGClient.h"
#include "arDriversCalling.h"

// Consume and do something with input-device messages.

class SZG_CALL arInputSink{
  friend class arInputNode;
 public:
  arInputSink(): _autoActivate(true) {}
  virtual ~arInputSink() {}

  virtual bool init(arSZGClient&)
    { return true; }
  virtual bool start()
    { return true; }
  virtual bool stop()
    { return true; }
  virtual bool restart()
    { return stop() && start(); }

  virtual void receiveData(int, arStructuredData*)
    {}
  virtual bool sourceReconfig(int)
    { return true; }
 protected:
  bool _autoActivate;
};

#endif
