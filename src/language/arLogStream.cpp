//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLogStream.h"

const int AR_LOG_DEFAULT = AR_LOG_WARNING;

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

inline int ar_logLevelNormalize(int& l) {
  switch(l) {
  case AR_LOG_SILENT:
  case AR_LOG_CRITICAL:
  case AR_LOG_ERROR:
  case AR_LOG_WARNING:
  case AR_LOG_REMARK:
  case AR_LOG_DEBUG:
    break;
  default:
    l = AR_LOG_NIL;
  }
  return l;
}

string ar_logLevelToString(int l){
  static const char* s[AR_LOG_NIL+1] = {
    "SILENT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "REMARK",
    "DEBUG",
    "INVALID_LOG_LEVEL" };
  return s[ar_logLevelNormalize(l)];
}

arLogStream::arLogStream():
  _output(&cout),
  _header("szg"),
  _maxLineLength(200),
  _threshold(AR_LOG_DEFAULT),
  _level(AR_LOG_DEFAULT){
}

void arLogStream::setStream(ostream& externalStream){
  _lock.lock();
    if (!_buffer.str().empty()){
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

bool arLogStream::setLogLevel(int l){
  ar_logLevelNormalize(l);
  if (l == AR_LOG_NIL)
    return false;
  _lock.lock();
    _threshold = l; 
  _lock.unlock();
  return true;
}

bool arLogStream::logLevelDefault() {
  _lock.lock();
    bool f = _threshold == AR_LOG_DEFAULT;
  _lock.unlock();
  return f;
}

string arLogStream::logLevel() {
  _lock.lock();
    const string s(ar_logLevelToString(_threshold));
  _lock.unlock();
  return s;
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

// Called from inside _lock, e.g., between _preAppend() and _finish().
void arLogStream::_postAppend(bool flush){
  // It is inefficient to copy a big string each time...
  // BUT we already assume that arLogStream
  // isn't high performance, at most 1000 accesses per second.
  if (flush || _buffer.str().length() > _maxLineLength)
    _flushLogBuffer();
}

void arLogStream::_finish(){
  _lock.unlock(); 
}

// Called from inside _lock.
void arLogStream::_flushLogBuffer(const bool addReturn){
  if (_level <= _threshold){
    *_output << _header << ":" << ar_logLevelToString(_level) << ": " << _buffer.str();
    if (addReturn){
      *_output << "\n";
    }
  }
  // Clear internal buffer.
  static const string empty;
  _buffer.str(empty);
}

inline arLogStream& arLogStream::_setLevel(const int l){
  _lock.lock();
  _level = l;
  _lock.unlock();
  return *this;
}

arLogStream& ar_log(){
  static arLogStream logStream;
  return logStream;
}

arLogStream& ar_log_critical(){
  return ar_log()._setLevel(AR_LOG_CRITICAL);
}

arLogStream& ar_log_error(){
  return ar_log()._setLevel(AR_LOG_ERROR);
}

arLogStream& ar_log_warning(){
  return ar_log()._setLevel(AR_LOG_WARNING);
}

arLogStream& ar_log_remark(){
  return ar_log()._setLevel(AR_LOG_REMARK);
}

arLogStream& ar_log_debug(){
  return ar_log()._setLevel(AR_LOG_DEBUG);
}

arLogStream& ar_endl(arLogStream& s){
  s._lock.lock();
  s._flushLogBuffer(false);
  if (s._level <= s._threshold){
    *s._output << endl;
  }
  s._lock.unlock();
  return s;
}
