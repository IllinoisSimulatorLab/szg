//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOCKET_TEXT_STREAM_H
#define AR_SOCKET_TEXT_STREAM_H

#include "arTextStream.h"
#include "arSocket.h"
#include "arLanguageCalling.h"

class SZG_CALL arSocketTextStream: public arTextStream{
 public:
  arSocketTextStream();
  virtual ~arSocketTextStream() {}

  void setSource(arSocket* socket) { _socket = socket; }
  virtual int ar_getc();
  virtual void ar_ungetc(int ch);
 private:
  arSocket*      _socket;
  int            _lastChar;
  bool           _useLastChar;
};

#endif
