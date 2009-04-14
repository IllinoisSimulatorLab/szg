//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LOG_STREAM_H
#define AR_LOG_STREAM_H

#include "arThread.h" // for arLock
#include "arLanguageCalling.h"

#include <iostream>
#include <sstream>

using namespace std;

// SILENT:   Absolutely nothing.
// CRITICAL: Essential state;  unrecoverable errors (app or component terminates).
// ERROR:    Something's wrong.
// WARNING:  The user likely didn't intend this (use WARNING sparingly).
// REMARK:   Everything else.
// DEBUG:    For ISL developers.

enum {
  AR_LOG_SILENT=0,
  AR_LOG_CRITICAL,
  AR_LOG_ERROR,
  AR_LOG_WARNING,
  AR_LOG_REMARK,
  AR_LOG_DEBUG,
  AR_LOG_NIL // invalid value
};

SZG_CALL int ar_stringToLogLevel(const string&);
SZG_CALL string ar_logLevelToString(int);
SZG_CALL bool ar_setLogLevel( const string& level, const bool fVerbose = true );

class SZG_CALL arLogStream {
  friend SZG_CALL arLogStream& ar_log_critical();
  friend SZG_CALL arLogStream& ar_log_error();
  friend SZG_CALL arLogStream& ar_log_warning();
  friend SZG_CALL arLogStream& ar_log_remark();
  friend SZG_CALL arLogStream& ar_log_debug();
  friend SZG_CALL arLogStream& ar_endl(arLogStream& logStream);
  friend SZG_CALL arLogStream& ar_hex (arLogStream& logStream);
 public:
  arLogStream();
  ~arLogStream() {}

  void setStream(ostream&);
  void setHeader(const string&);
  bool setLogLevel(int);
  void setTimestamp(const bool);
  string logLevel();
  bool logLevelDefault();

  arLogStream& operator<<(short n);
  arLogStream& operator<<(int n);
  arLogStream& operator<<(long n);
  arLogStream& operator<<(unsigned short n);
  arLogStream& operator<<(unsigned int n);
  arLogStream& operator<<(unsigned long n);
  arLogStream& operator<<(float f);
  arLogStream& operator<<(double f);
  arLogStream& operator<<(bool n);
  arLogStream& operator<<(const void* p);
  arLogStream& operator<<(char c);
  arLogStream& operator<<(const char* s);
  arLogStream& operator<<(const string& s);
  arLogStream& operator<<(arLogStream& (*func)(arLogStream& logStream));

 private:
  ostream* _output;

  // todo: map thread ID to buffers, for interleaving messages.
  ostringstream _buffer;

  string _header;
  unsigned _maxLineLength;
  int _threshold;
  int _level;
  bool _fTimestamp;

  bool _preAppend();
  void _postAppend(bool flush=false);
  void _finish();
  void _flush(const bool addNewline = true);
  arLogStream& _setLevel(int);

  // Guards _header, _threshold, _flush(), _buffer interleaving.
  // Guards _level *only* when altering.
  arGlobalLock _streamLock;
  void _lock(const char* name) { _streamLock.lock(name); }
  void _unlock() { _streamLock.unlock(); }
};

// Accessors (singleton).
SZG_CALL arLogStream& ar_log();
SZG_CALL arLogStream& ar_log_critical();
SZG_CALL arLogStream& ar_log_error();
SZG_CALL arLogStream& ar_log_warning();
SZG_CALL arLogStream& ar_log_remark();
SZG_CALL arLogStream& ar_log_debug();
SZG_CALL arLogStream& ar_endl(arLogStream& logStream);
SZG_CALL arLogStream& ar_hex(arLogStream& logStream);

SZG_CALL void ar_setLogLabel( const string& label );
SZG_CALL string ar_getLogLabel();
SZG_CALL string ar_logLevelsExpected();

#endif
