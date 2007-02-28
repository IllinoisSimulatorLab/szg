//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LOG_STREAM_H
#define AR_LOG_STREAM_H

#include "arThread.h"
#include "arLanguageCalling.h"

#include <iostream>
#include <sstream>

using namespace std;

// CRITICAL: Essential state and unrecoverable errors (app terminates).
// ERROR:    Errors that affect appearance.
// WARNING:  Things that will likely surprise the user.
// REMARK:   Everything else.
// DEBUG:    For ISL developers.

enum{
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

class SZG_CALL arLogStream{
  friend SZG_CALL arLogStream& ar_log_critical();
  friend SZG_CALL arLogStream& ar_log_error();
  friend SZG_CALL arLogStream& ar_log_warning();
  friend SZG_CALL arLogStream& ar_log_remark();
  friend SZG_CALL arLogStream& ar_log_debug();
  friend SZG_CALL arLogStream& ar_endl(arLogStream& logStream);
 public:
  arLogStream();
  ~arLogStream(){}
  
  void setStream(ostream&);
  void setHeader(const string&);
  bool setLogLevel(int);
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
  
 protected:
  ostream* _output;   
  arLock _lock;

  // Should really be replaced by a "thread map" (based on thread ID)
  // for good interleaving of messages.
  ostringstream _buffer;

  string _header;
  unsigned _maxLineLength;
  // DEPRECTAED DEPRECATED DEPRECATED DEPRECATED DEPRECATED
  bool _wrapFlag;
  int _threshold;
  int _level;
  
  void _preAppend();
  void _postAppend(bool flush=false);
  void _finish();
  void _flushLogBuffer(const bool addReturn = true);
  arLogStream& _setLevel(const int);
};

// Accessors (singleton).
SZG_CALL arLogStream& ar_log();
SZG_CALL arLogStream& ar_log_critical();
SZG_CALL arLogStream& ar_log_error();
SZG_CALL arLogStream& ar_log_warning();
SZG_CALL arLogStream& ar_log_remark();
SZG_CALL arLogStream& ar_log_debug();
SZG_CALL arLogStream& ar_endl(arLogStream& logStream);

#endif
