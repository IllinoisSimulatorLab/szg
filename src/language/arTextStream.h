//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXT_STREAM_H
#define AR_TEXT_STREAM_H

#include <stdio.h>
#include "arBuffer.h"
#include "arLanguageCalling.h"

// Simple stream interface for XML-style data parsing.
// Subclasses can provide FILE*, a socket, or a string as the stream source.

class SZG_CALL arTextStream{
 public:
  arTextStream() {}
  virtual ~arTextStream() {}

  // By convention, ar_getc should return an unsigned char converted to an int.
  virtual int ar_getc() { return EOF; }

  // ar_ungetc is expected to behave as follows:
  //   1. The next call to ar_getc() returns the int pushed via ar_ungetc.
  //   2. Afterwards, ar_getc() gets an int from its data source.
  //   3. Multiple calls to ar_ungetc() between calls to ar_getc() just
  //      change the int to be returned at the next ar_getc() call.
  //      In other words, the stack is only guaranteed to be one deep.
  virtual void ar_ungetc(int) {}
};

#endif
