//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRING_TEXT_STREAM_H
#define AR_STRING_TEXT_STREAM_H

#include <sstream>
#include "arBuffer.h"
#include "arTextStream.h"
#include "arLanguageCalling.h"

using namespace std;

// Simple stream interface for XML-style data parsing.
// Subclasses can provide FILE*, a socket, or a string as the stream source.

// If a class exists only in the .h, do not decorate it with SZG_CALL. 
// In Win32, this confuses the linker when importing the class.

class arStringTextStream: public arTextStream{
 public:
  arStringTextStream(){}
  arStringTextStream(const string& newString){ _stream.str(newString); }
  virtual ~arStringTextStream(){}

  virtual int ar_getc(){ return _stream.get(); }
  // Stringstream has a perfectly good unget.  Don't get fancy.
  virtual void ar_ungetc(int){ _stream.unget(); }

  void setString(const string& newString){ _stream.str(newString); }
 private:
  stringstream _stream;
};

#endif
