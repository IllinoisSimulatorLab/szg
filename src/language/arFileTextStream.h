//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILE_TEXT_STREAM_H
#define AR_FILE_TEXT_STREAM_H

#include "arTextStream.h"
#include "arDataUtilities.h"
#include "arLanguageCalling.h"
#include <string>
using namespace std;

class SZG_CALL arFileTextStream: public arTextStream{
 public:
  arFileTextStream();
  virtual ~arFileTextStream() {}

  bool ar_open(const string& name, const string& subdirectory,
               const string& path);
  bool ar_open(const string& name, const string& path);
  bool ar_open(const string& name);
  bool ar_close();
  bool operator!() const { return !_source; }
  void setSource(FILE* source);

  // Inherited methods.
  virtual int ar_getc();
  virtual void ar_ungetc(int ch);

 private:
  FILE*          _source;
  bool           _useUngetc;
  int            _ungetc;
  char           _buffer[257];
  int            _bufferLocation;
  int            _bufferLength;

  void _reset();
};

#endif
