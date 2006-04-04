//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLogStream.h"

int ar_stringToLogLevel(const string& logLevel){
  if (logLevel == "SILENT")
    return AR_LOG_SILENT;
  if (logLevel == "CRITICAL")
    return AR_LOG_CRITICAL;
  if (logLevel == "ERROR")
    return AR_LOG_ERROR;
  if (logLevel == "WARNING")
    return AR_LOG_WARNING;
  if (logLevel == "REMARK")
    return AR_LOG_REMARK;
  if (logLevel == "DEBUG")
    return AR_LOG_DEBUG;
  return AR_LOG_NIL; 
}

string ar_logLevelToString(int logLevel){
  static const char* s[AR_LOG_NIL+1] = {
    "SILENT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "REMARK",
    "DEBUG",
    "INVALID_LOG_LEVEL" };

  switch(logLevel){
    case AR_LOG_SILENT:
    case AR_LOG_CRITICAL:
    case AR_LOG_ERROR:
    case AR_LOG_WARNING:
    case AR_LOG_REMARK:
    case AR_LOG_DEBUG:
      break;
    default:
      logLevel = AR_LOG_NIL;
  }
  return string(s[logLevel]);
}

arLogStream::arLogStream():
  _output(&cout),
  _header("szg"),
  _maxLineLength(200),
  _logLevel(AR_LOG_WARNING),
  _currentLevel(AR_LOG_WARNING){
}

void arLogStream::setStream(ostream& externalStream){
  _lock.lock();
  // Only flush the buffer if there is, in fact, something in it.
  if (_buffer.str().length() > 0){
    _flushLogBuffer(true);
  }
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

// For ar_endl.
arLogStream& arLogStream::operator<<(arLogStream& (*func)(arLogStream& logStream)){
  return func(*this); 
}

void arLogStream::_preAppend(){
  // Ensure atomic access.
  _lock.lock();
}

void arLogStream::_postAppend(bool flush){
  // It is inefficient to copy a big string each time...
  // BUT we already assume that arLogStream
  // isn't high performance, at most 1000 accesses per second.
  if (_buffer.str().length() > _maxLineLength || flush)
    _flushLogBuffer();
}

void arLogStream::_finish(){
  _lock.unlock(); 
}

void arLogStream::_flushLogBuffer(bool addReturn){
  // Only send to the stream if the level works out.
  if (_currentLevel <= _logLevel){
    *_output << _header << ":" << ar_logLevelToString(_currentLevel) << ": " << _buffer.str();
    if (addReturn){
      *_output << "\n";
    }
  }
  // Clear internal buffer.
  static const string empty;
  _buffer.str(empty);
}

void arLogStream::_setCurrentLevel(int currentLevel){
  _lock.lock();
  _currentLevel = currentLevel;
  _lock.unlock();
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
    *logStream._output << endl;
  }
  logStream._lock.unlock();
  return logStream;
}
