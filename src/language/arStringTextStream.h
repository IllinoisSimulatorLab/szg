//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRING_TEXT_STREAM_H
#define AR_STRING_TEXT_STREAM_H

#include <sstream>
#include "arBuffer.h"
#include "arTextStream.h"
#include "arLanguageCalling.h"

using namespace std;

/// A base class that provides a very simple stream interface.
/// This is mostly used for XML-style data parsing. Via subclasses, one
/// can use FILE*, a socket, or a string as the stream source.

// WARNING: If a class ONLY exists in the .h, then we SHOULD NOT decorate it
// with SZG_CALL. In the Win32 case, this will lead to the linker getting
// confused when trying to import the class.
class arStringTextStream: public arTextStream{
 public:
  arStringTextStream(){}
  arStringTextStream(const string& newString){ _stream.str(newString); }
  virtual ~arStringTextStream(){}

  virtual int ar_getc(){ return _stream.get(); }
  // Since stringstream has a perfectly good unget, no need to get fancy.
  virtual void ar_ungetc(int){ _stream.unget(); }

  void setString(const string& newString){ _stream.str(newString); }
 private:
  stringstream _stream;
};

#endif
