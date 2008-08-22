//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLogStream.h"
#include "arDataUtilities.h"

#ifndef AR_USE_WIN_32 // visual c++ 6 lacks toupper
#include <cctype>    // std::toupper
#include <algorithm> // std::transform
#endif

const int AR_LOG_DEFAULT = AR_LOG_WARNING;

bool ar_setLogLevel( const string& level, const bool fVerbose ) {
  const bool ok = ar_log().setLogLevel( ar_stringToLogLevel( level ) );
  if (fVerbose) {
    if (ok) {
      ar_log_critical() << "loglevel is " << level << ".\n";
    } else {
      ar_log_critical() << "ignoring unrecognized loglevel '" << level << "'.\n";
    }
  }
  return ok;
}

int ar_stringToLogLevel(const string& logLevel) {
  // Convert to uppercase, for convenience of typing "-szg log=debug" not DEBUG.
  string s(logLevel);
#ifndef AR_USE_WIN_32
  std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::toupper);
#endif
  return
    (s == "SILENT")   ? AR_LOG_SILENT :
    (s == "CRITICAL") ? AR_LOG_CRITICAL :
    (s == "ERROR")    ? AR_LOG_ERROR :
    (s == "WARNING")  ? AR_LOG_WARNING :
    (s == "REMARK")   ? AR_LOG_REMARK :
    (s == "DEBUG")    ? AR_LOG_DEBUG :
    AR_LOG_NIL;
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

string ar_logLevelToString(int l) {
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

string ar_logLevelsExpected() {
  return "SILENT, CRITICAL, ERROR, WARNING, REMARK, DEBUG";
}

arLogStream::arLogStream():
  _output(&cout),
  _header("NULL"),
  _maxLineLength(200),
  _threshold(AR_LOG_DEFAULT),
  _level(AR_LOG_DEFAULT),
  _fTimestamp(false),
  _streamLock("Global\\szgLog") {
  if (!_streamLock.valid())
    cerr << "arLogStream warning: no locks. Expect interleaving.\n";
}

void arLogStream::setStream(ostream& externalStream) {
  _lock("arLogStream::setStream");
    _flush(true);
    _output = &externalStream;
  _unlock();
}

void arLogStream::setHeader(const string& header) {
  _lock("arLogStream::setHeader");
    _header = header;
  _unlock();
}

void arLogStream::setTimestamp(const bool f) {
  _lock("arLogStream::setTimestamp");
    _fTimestamp = f;
  _unlock();
}

bool arLogStream::setLogLevel(int l) {
  ar_logLevelNormalize(l);
  if (l == AR_LOG_NIL)
    return false;
  _lock("arLogStream::setLogLevel");
    _threshold = l;
  _unlock();
  return true;
}

bool arLogStream::logLevelDefault() {
  _lock("arLogStream::logLevelDefault");
    const bool f = _threshold == AR_LOG_DEFAULT;
  _unlock();
  return f;
}

string arLogStream::logLevel() {
  _lock("arLogStream::logLevel");
    const string s(ar_logLevelToString(_threshold));
  _unlock();
  return s;
}

arLogStream& arLogStream::operator<<(short n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(int n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(long n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned short n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned int n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(unsigned long n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(float f) {
  if (!_preAppend())
    return *this;
  _buffer << f;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(double f) {
  if (!_preAppend())
    return *this;
  _buffer << f;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(bool n) {
  if (!_preAppend())
    return *this;
  _buffer << n;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(const void* p) {
  if (!_preAppend())
    return *this;
  _buffer << p;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(char c) {
  if (!_preAppend())
    return *this;
  _buffer << c;
  _postAppend();
  _finish();
  return *this;
}

arLogStream& arLogStream::operator<<(const char* s) {
  if (!_preAppend())
    return *this;
  const char* pch = strchr(s, '\n');
  if (!pch) {
    // The most common case, by far.
    _buffer << s;
  }
  else {
    const unsigned l = strlen(s);
    for (unsigned i = 0; i < l; ++i) {
      if (s[i] == '\n') {
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

arLogStream& arLogStream::operator<<(const string& s) {
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
arLogStream& arLogStream::operator<<(arLogStream& (*f)(arLogStream&)) {
  return f(*this);
}

bool arLogStream::_preAppend() {
  // Serialize.
  if (_level <= _threshold) {
    _lock("arLogStream::_preAppend");
    return true;
  }

  // Nothing to print.
  return false;
}

// Called from inside _lock, e.g., between _preAppend() and _finish().
void arLogStream::_postAppend(bool flush) {
  // It is inefficient to copy a big string each time...
  // BUT we already assume that arLogStream
  // isn't high performance, at most 1000 accesses per second.
  if (flush || _buffer.str().length() > _maxLineLength)
    _flush();
}

void arLogStream::_finish() {
  _unlock();
}

void arLogStream::_flush(const bool addNewline) {
//  if (!_l.locked() && _l.valid())
//    cerr << "arLogStream warning: internal lock mismatch.\n";
  if (_buffer.str().empty())
    return;

  if (_level <= _threshold) {
    // Accumulator, so there's only one << to _output.
    string s(_header=="NULL" ? ar_getLogLabel() : _header);

    if (_fTimestamp) {
      string now(ar_currentTimeString());

      // Skip past gobbledegook.
      unsigned pos = now.find('/');
      if (pos == string::npos)
        goto LDone;
      now = now.substr(pos+1);

      // Skip past day-of-week.
      pos = now.find(' ');
      if (pos == string::npos)
        goto LDone;
      now = now.substr(pos+1);

      // Truncate year.
      pos = now.find("200");
      if (pos != string::npos)
	now = now.substr(0, pos-1);

      // Strip leading zero of day-of-month.
      pos = now.find(" 0");
      if (pos != string::npos) {
	// now == "Oct 02 08:01:24"
	now = now.substr(0, pos+1) + now.substr(pos+2);
	}

      // Strip leading zero of hour.
      pos = now.find(" 0", pos+5);
      if (pos != string::npos) {
	// now == "Oct 2 08:01:24"
	now = now.substr(0, pos+1) + now.substr(pos+2);
	}

      s += " " + now;
    }
LDone:
    s += " " + ar_logLevelToString(_level) + ": " + _buffer.str();
    if (addNewline) {
      s += "\n";
    }
    cout << s;
    if (_output && (*_output != cout)) {
      *_output << s;
    }
  }

  static const string empty;
  _buffer.str(empty);
}

inline arLogStream& arLogStream::_setLevel(int l) {
  _lock("arLogStream::_setLevel");
  if (ar_logLevelNormalize(l) != AR_LOG_NIL)
    _level = l;
  _unlock();
  return *this;
}

arLogStream& ar_log() {
  static arLogStream l;
  return l;
}

arLogStream& ar_log_critical() {
  return ar_log()._setLevel(AR_LOG_CRITICAL);
}

arLogStream& ar_log_error() {
  return ar_log()._setLevel(AR_LOG_ERROR);
}

arLogStream& ar_log_warning() {
  return ar_log()._setLevel(AR_LOG_WARNING);
}

arLogStream& ar_log_remark() {
  return ar_log()._setLevel(AR_LOG_REMARK);
}

arLogStream& ar_log_debug() {
  return ar_log()._setLevel(AR_LOG_DEBUG);
}

arLogStream& ar_endl(arLogStream& s) {
  s._lock("ar_endl");
  s._flush(false);
  if (s._level <= s._threshold) {
    cout << endl;
    if (s._output && (*s._output != cout)) {
      *s._output << endl;
    }
  }
  s._unlock();
  return s;
}

arLogStream& ar_hex(arLogStream& s) {
  s._preAppend();
  s._buffer << std::hex;
  s._postAppend();
  s._finish();
  return s;
}

static string __processLabel("szg");
static arLock __processLabelLock; // guard __processLabel

void ar_setLogLabel( const string& label ) {
  __processLabelLock.lock("ar_setLogLabel");
  __processLabel = label;
  __processLabelLock.unlock();
}

string ar_getLogLabel() {
  __processLabelLock.lock("ar_getLogLabel");
  const string label(__processLabel);
  __processLabelLock.unlock();
  return label;
}
