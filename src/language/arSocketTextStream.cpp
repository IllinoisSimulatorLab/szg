//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSocketTextStream.h"

arSocketTextStream::arSocketTextStream() :
  _socket(NULL),
  _lastChar('\0'),
  _useLastChar(false)
{
}

int arSocketTextStream::ar_getc() {
  if (_useLastChar) {
    _useLastChar = false;
  }
  else {
    char buf;
    _lastChar = (_socket->ar_safeRead(&buf, 1) > 0) ? (unsigned char)(buf) : EOF;
  }
  return _lastChar;
}

// If called multiply without intervening ar_getc()'s,
// only the final ar_ungetc() takes effect.
void arSocketTextStream::ar_ungetc(int ch) {
  _useLastChar = true;
  _lastChar = ch;
}
