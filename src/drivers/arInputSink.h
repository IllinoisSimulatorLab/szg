//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_SINK_H
#define AR_INPUT_SINK_H

#include "arStructuredData.h"
#include "arSZGClient.h"

/// Something which absorbs input-device messages and does something with them.

class arInputSink{
  friend class arInputNode;
 public:
  arInputSink() {_autoActivate = true; }
  virtual ~arInputSink() {}

  virtual bool init(arSZGClient&)
    { return true; }
  virtual bool start()
    { return true; }
  virtual bool stop()
    { return true; }
  virtual bool restart()
    { return true; }

  virtual void receiveData(int,arStructuredData*)
    {}
  virtual bool sourceReconfig(int)
    { return true; }
 protected:
  bool _autoActivate;
};

#endif
