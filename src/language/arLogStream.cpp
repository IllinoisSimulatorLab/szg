//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLogStream.h"

arLogStream::arLogStream():
  // By default, we log to cout.
  _output(&cout),
  _header("szg_log"),
  _maxLineLength(200),
  _logLevel(AR_LOG_DEBUG),
  _currentLevel(AR_LOG_CRITICAL){
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

void arLogStream::setLogLevel(int logLevel){
  _lock.lock();
  _logLevel = logLevel; 
  _lock.unlock();
}

arLogStream& arLogStream::operator<<(short n){
  _preAppend();
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(int n){
  _preAppend();
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(long n){
  _preAppend();
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned short n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned int n){
  _preAppend();
  _buffer << n;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned long n){
  _preAppend();
  _buffer << n;
  _postAppend();
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(float f){
  _preAppend();
  _buffer << f;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(double f){
  _preAppend();
  _buffer << f;
  _postAppend();
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(bool n){
  _preAppend();
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(const void* p){
  _preAppend();
  _buffer << p;
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(char c){
  _preAppend();
  _buffer << c;
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(const char* s){
  _preAppend();
  unsigned int l = strlen(s);
  unsigned int current = 0;
  while (current < l){
    if (s[current] == '\n'){
      _postAppend(true); 
    }
    else{
      _buffer << s[current];
    }
    current++;
  }
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(const string& s){
  _preAppend();
  unsigned int current = 0;
  while(current < s.length()){
    unsigned int next = s.find('\n', current);
    if (next != string::npos){
      _buffer << s.substr(current, next-current);
      _postAppend(true);
      current = next+1;
    }
    else{
      _buffer << s.substr(current, s.length()-current);
      _postAppend(false);
      current = s.length();
    }
  }
  _finish();
  return *this;  
}

// Needed so that ar_endl works like one expects.
arLogStream& arLogStream::operator<<(arLogStream& (*func)(arLogStream& logStream)){
  return func(*this); 
}

void arLogStream::_preAppend(){
  // Must ensure atomic access.
  _lock.lock();
}

void arLogStream::_postAppend(bool flush){
  // It is inefficient to do a big string copy each time...
  // BUT we are already making the assumption that this logging
  // class isn't high performance. At most 1000 accesses
  // per second in a "reasonable" app.
  string s = _buffer.str();
  if (s.length() > _maxLineLength || flush){
    _flushLogBuffer();
  }
}

void arLogStream::_finish(){
  _lock.unlock(); 
}

void arLogStream::_flushLogBuffer(bool addReturn){
  // Only send to the stream if the level works out.
  if (_currentLevel <= _logLevel){
    (*_output) << _header << ":" << _levelToString(_currentLevel) << ":" << _buffer.str();
    if (addReturn){
      (*_output) << "\n";
    }
  }
  string tmp("");
  // Clears internal buffer.
  _buffer.str(tmp);
}

void arLogStream::_setCurrentLevel(int currentLevel){
  _lock.lock();
  _currentLevel = currentLevel;
  _lock.unlock();
}

string arLogStream::_levelToString(int level){
  switch(level){
  case AR_LOG_SILENT:
    return string("SILENT");
  case AR_LOG_CRITICAL:
    return string("CRITICAL");
  case AR_LOG_ERROR:
    return string("ERROR");
  case AR_LOG_WARNING:
    return string("WARNING");
  case AR_LOG_REMARK:
    return string("REMARK");
  case AR_LOG_DEBUG:
    return string("DEBUG");
  default:
    return string("GARBAGE");
  }
}

arLogStream& ar_log(){
  static arLogStream logStream;
  return logStream;
}

arLogStream& ar_log_critical(){
  arLogStream& temp = ar_log();
  temp._setCurrentLevel(AR_LOG_CRITICAL);
  return temp;
}

arLogStream& ar_log_error(){
  arLogStream& temp = ar_log();
  temp._setCurrentLevel(AR_LOG_ERROR);
  return temp;
}

arLogStream& ar_log_warning(){
  arLogStream& temp = ar_log();
  temp._setCurrentLevel(AR_LOG_WARNING);
  return temp;
}

arLogStream& ar_log_remark(){
  arLogStream& temp = ar_log();
  temp._setCurrentLevel(AR_LOG_REMARK);
  return temp;
}

arLogStream& ar_log_debug(){
  arLogStream& temp = ar_log();
  temp._setCurrentLevel(AR_LOG_DEBUG);
  return temp;
}

arLogStream& ar_endl(arLogStream& logStream){
  logStream._lock.lock();
  logStream._flushLogBuffer(false);
  if (logStream._currentLevel <= logStream._logLevel){
    (*logStream._output) << endl;
  }
  logStream._lock.unlock();
  return logStream;
}

