//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXT_STREAM_H
#define AR_TEXT_STREAM_H

#include <stdio.h>
#include "arBuffer.h"
#include "arCallingConventions.h"

/// A base class that provides a very simple stream interface.
/// This is mostly used for XML-style data parsing. Via subclasses, one
/// can use FILE*, a socket, or a string as the stream source.
class SZG_CALL arTextStream{
 public:
  arTextStream(){}
  virtual ~arTextStream(){}

  // NOTE: by convention, ar_getc should return an unsigned char
  //   converted to an int.
  virtual int ar_getc(){ return EOF; }
  // ar_ungetc is expected to behave as follows:
  //   1. The next call to ar_getc() will return the int pushed via
  //      ar_ungetc.
  //   2. Afterwards, ar_getc() gets an int from its data source.
  //   3. Multiple calls to ar_ungetc(..) between calls to ar_getc() just
  //      change the int to be returned at the next ar_getc() call. In other
  //      words, the stack is only guaranteed to be one deep.
  virtual void ar_ungetc(int){}
};

#endif
