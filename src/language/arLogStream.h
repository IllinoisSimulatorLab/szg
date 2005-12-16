//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LOG_STREAM_H
#define AR_LOG_STREAM_H

#include <iostream>
#include <sstream>
#include "arThread.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arLanguageCalling.h"

using namespace std;

class SZG_CALL arLogStream{
  friend SZG_CALL arLogStream& ar_endl(arLogStream& logStream);
 public:
  arLogStream();
  ~arLogStream(){}
  
  void setStream(ostream& externalStream);
  void setHeader(const string& header);
  
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
  
 protected:
  ostream* _output;   
  arLock _lock;
  // Should really be replaced by a "thread map" (based on thread ID)
  // for good interleaving of messages.
  ostringstream _buffer;
  string _header;
  int _maxLineLength;
  bool _wrapFlag;
  
  void _preAppend();
  void _postAppend();
  void _flushLogBuffer(bool addReturn);
};

// Using the singleton pattern.
// ALL ACCESSES TO THE LOG MUST USE THIS FUNCTION!
SZG_CALL arLogStream& ar_log();

SZG_CALL arLogStream& ar_endl(arLogStream& logStream);

#endif
