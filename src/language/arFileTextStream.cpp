//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFileTextStream.h"

using namespace std;

arFileTextStream::arFileTextStream(){
  _reset();
}

bool arFileTextStream::ar_open(const string& name, const string& subdirectory,
                               const string& path){
  // No error diagnostics: leave that to the caller.
  const string fileName = ar_fileFind(name, subdirectory, path);
  if (fileName == "NULL"){
    return false;
  }

  _reset();
  _source = fopen(fileName.c_str(), "r");
  return _source != NULL;
}

bool arFileTextStream::ar_open(const string& name, const string& path){
  return ar_open(name,"",path);
}

bool arFileTextStream::ar_open(const string& name){
  return ar_open(name,"","");
}

bool arFileTextStream::ar_close(){
  const bool ok = !_source || fclose(_source)==0;
  _reset();
  return ok;
}

void arFileTextStream::setSource(FILE* source){
  _reset();
  _source = source;
}

int arFileTextStream::ar_getc(){
  if (_useUngetc){
    _useUngetc = false;
    return _ungetc;
  }

  if (_bufferLocation == 256){
    _bufferLocation = 0;
    _bufferLength = fread(_buffer, 1, 256, _source);
  }
  if (_bufferLocation < _bufferLength){
    return (unsigned char)(_buffer[_bufferLocation++]);
  }
  return EOF;
}

void arFileTextStream::ar_ungetc(int ch){
  _useUngetc = true;
  _ungetc = ch;
}

void arFileTextStream::_reset(){
  _source = NULL;
  _useUngetc = false;
  _ungetc = EOF;
  _bufferLocation = 256;
  _buffer[256] = '\0';
  _bufferLength = 0;
}
