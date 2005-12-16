//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLogStream.h"

arLogStream& ar_log(){
  static arLogStream logStream;
  return logStream;
}


arLogStream::arLogStream():
  // By default, we log to cout.
  _output(&cout),
  _header(""),
  _maxLineLength(100){
}

void arLogStream::setStream(ostream& externalStream){
  _lock.lock();
  _flushLogBuffer(true);
  _output = &externalStream;
  _lock.unlock();
}

void arLogStream::setHeader(const string& header){
  _lock.lock();
  _header = header;
  _lock.unlock();
}

arLogStream& arLogStream::operator<<(short n){
  _preAppend();
  _buffer << n;
  _postAppend();
}

arLogStream& arLogStream::operator<<(int n){
  _preAppend();
  _buffer << n;
  _postAppend();
}

arLogStream& arLogStream::operator<<(long n){
  _preAppend();
  _buffer << n;
  _postAppend();
}

arLogStream& arLogStream::operator<<(unsigned short n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(unsigned int n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(unsigned long n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(float f){
  _preAppend();
  _buffer << f;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(double f){
  _preAppend();
  _buffer << f;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(bool n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
}

arLogStream& arLogStream::operator<<(const void* p){
  _preAppend();
  _buffer << p;
  _postAppend();  
}

arLogStream& arLogStream::operator<<(char c){
  _preAppend();
  _buffer << c;
  _postAppend();  
}

arLogStream& arLogStream::operator<<(const char* s){
  _preAppend();
  _buffer << s;
  _postAppend();  
}

arLogStream& arLogStream::operator<<(const string& s){
  _preAppend();
  _buffer << s;
  _postAppend();  
}

void arLogStream::_preAppend(){
  // Must ensure atomic access.
  _lock.lock();
  // The appropriate << operator will decide whether a line break has occured.
  _wrapFlag = false;
}

void arLogStream::_postAppend(){
  // It is inefficient to do a big string copy each time...
  // BUT we are already making the assumption that this logging
  // class isn't high performance. At most 1000 accesses
  // per second in a "reasonable" app.
  string s = _buffer.str();
  if (s.length() > _maxLineLength || _wrapFlag){
    _flushLogBuffer(!_wrapFlag);
  }
  _lock.unlock();
}

void arLogStream::_flushLogBuffer(bool addReturn){
  (*_output) << _header << _buffer.str();
  if (addReturn){
    (*_output) << "\n";
  }
  string tmp("");
  // Clears internal buffer.
  _buffer.str(tmp);
}

arLogStream& endl(arLogStream& logStream){
  logStream._lock.lock();
  logStream._flushLogBuffer(false);
  (*logStream._output) << endl;
  logStream._lock.unlock();
}
