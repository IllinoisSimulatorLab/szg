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
  _level(AR_LOG_DEFAULT),
  _l("Global\\szgLog"),
  _fLocked(false){
  if (!_l.valid())
    cerr << "arLogStream warning: no locks. Expect interleaving.\n";
}

void arLogStream::setStream(ostream& externalStream){
  _lock();
    _flush(true);
    _output = &externalStream;
  _unlock();
}

void arLogStream::setHeader(const string& header){
  _lock();
  _header = header;
  _unlock();
}

bool arLogStream::setLogLevel(int l){
  ar_logLevelNormalize(l);
  if (l == AR_LOG_NIL)
    return false;
  _lock();
    _threshold = l; 
  _unlock();
  return true;
}

bool arLogStream::logLevelDefault() {
  _lock();
    const bool f = _threshold == AR_LOG_DEFAULT;
  _unlock();
  return f;
}

string arLogStream::logLevel() {
  _lock();
    const string s(ar_logLevelToString(_threshold));
  _unlock();
  return s;
}

arLogStream& arLogStream::operator<<(short n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(int n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(long n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned short n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned int n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned long n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(float f){
  if (!_preAppend())
    return *this;
  _buffer << f;
  _postAppend(); 
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(double f){
  if (!_preAppend())
    return *this;
  _buffer << f;
  _postAppend();
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(bool n){
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(const void* p){
  if (!_preAppend())
    return *this;
  _buffer << p;
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(char c){
  if (!_preAppend())
    return *this;
  _buffer << c;
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(const char* s){
  if (!_preAppend())
    return *this;
  const char* pch = strchr(s, '\n');
  if (!pch) {
    // The most common case, by far.
    _buffer << s;
  }
  else {
    const unsigned l = strlen(s);
    for (unsigned i = 0; i < l; ++i){
      if (s[i] == '\n'){
	_postAppend(true); 
      }
      else{
	_buffer << s[i];
      }
    }
  }
  _postAppend(); 
  _finish();
  return *this; 
}

arLogStream& arLogStream::operator<<(const string& s){
  if (!_preAppend())
    return *this;

  const unsigned l = s.length();
  unsigned current = 0;
  while (current < l) {
    const unsigned next = s.find('\n', current);
    if (next == string::npos) {
      _buffer << s.substr(current, l-current);
      _postAppend(false);
      break;
    }
    _buffer << s.substr(current, next-current);
    _postAppend(true);
    current = next + 1;
  }
  _finish();
  return *this;  
}

// For ar_endl and ar_hex.
arLogStream& arLogStream::operator<<(arLogStream& (*f)(arLogStream&)){
  return f(*this); 
}

bool arLogStream::_preAppend(){
  // Serialize.
  if (_level <= _threshold) {
    _lock();
    return true;
  }

  // Nothing to print.
  return false;
}

// Called from inside _lock, e.g., between _preAppend() and _finish().
void arLogStream::_postAppend(bool flush){
  // It is inefficient to copy a big string each time...
  // BUT we already assume that arLogStream
  // isn't high performance, at most 1000 accesses per second.
  if (flush || _buffer.str().length() > _maxLineLength)
    _flush();
}

void arLogStream::_finish(){
  _unlock(); 
}

void arLogStream::_flush(const bool addNewline){
  if (!_fLocked && _l.valid())
    cerr << "arLogStream warning: internal lock mismatch.\n";
  if (_buffer.str().empty())
    return;

  if (_level <= _threshold) {
    // Intermediate variable so there's only one << to _output.
    ostringstream s;
    s << _header << ":" << ar_logLevelToString(_level) << ": " << _buffer.str();
    if (addNewline) {
      s << "\n";
    }
    *_output << s.str();
  }
  // Clear internal buffer.
  static const string empty;
  _buffer.str(empty);
}

inline arLogStream& arLogStream::_setLevel(int l){
  _lock();
  if (ar_logLevelNormalize(l) != AR_LOG_NIL)
    _level = l;
  _unlock();
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
  s._lock();
  s._flush(false);
  if (s._level <= s._threshold){
    *s._output << endl;
  }
  s._unlock();
  return s;
}

arLogStream& ar_hex(arLogStream& s){
  s._preAppend();
  s._buffer << std::hex;
  s._postAppend(); 
  s._finish();
  return s;
}
