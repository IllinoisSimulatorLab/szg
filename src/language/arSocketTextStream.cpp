//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSocketTextStream.h"

arSocketTextStream::arSocketTextStream(){
  _socket = NULL;
  _lastChar = '\0';
  _useLastChar = false;
}

int arSocketTextStream::ar_getc(){
  if (_useLastChar){
    _useLastChar = false;
  }
  else{
    char buffer[1];
    if (_socket->ar_safeRead(buffer, 1) <= 0){
      _lastChar = EOF;
    }
    else{
      _lastChar = (unsigned char)(buffer[0]);
    }
  }
  return _lastChar;
}

void arSocketTextStream::ar_ungetc(int ch){
  _useLastChar = true;
  _lastChar = ch;
}
