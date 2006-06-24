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
  string fileName = ar_fileFind(name, subdirectory, path);
  if (fileName == "NULL"){
    // It is always best to let the CALLER print out the error message,
    // if such is desired.
    return false;
  }
  _reset();
  _source = fopen(fileName.c_str(),"r");
  if (!_source){
    // It is always best to let the CALLER print out the error message,
    // if such is desired.
    return false;
  }
  return true;
}

bool arFileTextStream::ar_open(const string& name, const string& path){
  return ar_open(name,"",path);
}

bool arFileTextStream::ar_open(const string& name){
  return ar_open(name,"","");
}

bool arFileTextStream::ar_close(){
  int returnValue = 0;
  if (_source){
    returnValue = fclose(_source);
  }
  _reset();
  return returnValue == 0 ? true : false;
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
    _bufferLength = fread(_buffer, 1, 256, _source);
    _bufferLocation = 0;
  }
  if (_bufferLocation < _bufferLength){
    int temp = (unsigned char)(_buffer[_bufferLocation]);
    _bufferLocation++;
    return temp;
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

