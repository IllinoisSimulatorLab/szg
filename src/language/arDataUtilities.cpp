//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arDatabaseNode.h" // for ar_refNodeList, etc.
#include "arLogStream.h"

#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <limits.h> // for CLK_TCK and error-checking conversions
#include <iterator> // for g++ ostream_iterator<> in ar_replaceAll()

#include "arSTLalgo.h"
using namespace std;

#ifdef AR_USE_WIN_32

#include <io.h>     // for directory listing
#include <direct.h>
#include <time.h>
#include <iostream>
#include <sstream>


string ar_getLastWin32ErrorString() {
  const DWORD errCode = GetLastError();
  LPTSTR s;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode,
            0, (LPTSTR)&s, 0, NULL) == 0) {
    return "no win32 error message for error code " + ar_intToString(errCode);
  }

  const string result(s);
  LocalFree(s);
  return result;
}

#else

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#endif



// Cross-platform clock.
#ifdef AR_USE_WIN_32

// Current time.

// todo: assumes that ticks/second fits in a single int, not true in a few years.
ar_timeval ar_time() {
#if 0
  // an earlier implementation
  struct _timeb t;
  _ftime(&t);
  return ar_timeval(t.time, 1000 * t.millitm);
#endif

  LARGE_INTEGER clockTicks;
  LARGE_INTEGER ticksPerSecond;
  QueryPerformanceCounter(&clockTicks);
  QueryPerformanceFrequency(&ticksPerSecond);
  ar_timeval result;
  result.sec = clockTicks.QuadPart/ticksPerSecond.QuadPart;
  result.usec = (int)(1.e6*((clockTicks.QuadPart/(double)ticksPerSecond.QuadPart)
                            - (double)result.sec));
  return result;
}

#else

ar_timeval ar_time() {
  struct timeval tNow;
  gettimeofday(&tNow, 0);
  return ar_timeval(tNow.tv_sec, tNow.tv_usec);
}

#endif

// Take two timevals closer than 4000000000 usec, and report their diff in usec.

double ar_difftime(const ar_timeval& a, const ar_timeval& b) {
  return 1e6*(a.sec-b.sec) + (a.usec-b.usec);
}

// Return a nonzero value, safe to divide by, e.g. for computing fps.
double ar_difftimeSafe(const ar_timeval& a, const ar_timeval& b) {
  const double diff = ar_difftime(a, b);
  return diff > 1. ? diff : 1.;
}

// Add two ar_timeval's.  Return (a ref to) the first argument.

ar_timeval& ar_addtime(ar_timeval& lhs, const ar_timeval& rhs) {
  lhs.sec += rhs.sec;
  if ((lhs.usec += rhs.usec) >= 1000000) {
    // Carry.
    lhs.usec -= 1000000;
    ++lhs.sec;
  }
  return lhs;
}

// Returns current date & time in following format:
// year:day of year:second of day/pretty human-readable string

string ar_currentTimeString() {
  time_t timeVal;
  time( &timeVal );
  struct tm* t = localtime( &timeVal );
  if (!t)
    return string("");

  string s(ar_intToString(1900+t->tm_year) + ":" +
      ar_intToString(t->tm_yday) + ":" + ar_intToString(3600*t->tm_hour) +
      ar_intToString(60*t->tm_min) + ar_intToString(t->tm_sec));
  char* pch = ctime( &timeVal );
  if (!pch)
    return s;

  // strip trailing \n
  const int n = strlen(pch);
  if (pch[n-1] == '\n')
    pch[n-1] = '\0';
  return s + "/" + pch;
}

arTimer::arTimer( double dur ) :
  _firstStart( true ),
  _startTime( ar_time() ),
  _lastStart( _startTime ),
  _duration( dur ),
  _lapTime(0.),
  _runTime(0.),
  _running( false ) {
}

void arTimer::start( double dur ) {
  if (_firstStart || dur > 0.) {
    reset();
  }
  else if (_running) {
    // reset lap timer
    stop();
  }
  _lastStart = ar_time();
  _duration = dur;
  _running = true;
}

double arTimer::totalTime() const {
  return ar_difftime( ar_time(), _startTime );
}

double arTimer::elapsed() {
  ar_log_error() << "arTimer::elapsed() deprecated. Use arTimer::totalTime().\n";
  return totalTime();
}

double arTimer::lapTime() {
  if (_running)
    _lapTime = ar_difftime( ar_time(), _lastStart );
  return _lapTime;
}

double arTimer::runningTime() {
  return _runTime + (_running ? lapTime() : 0);
}

bool arTimer::done() {
  return runningTime() >= _duration;
}

void arTimer::stop() {
  if (_running) {
    _runTime += lapTime(); // automatically updates _lapTime
    _running = false;
  }
}

void arTimer::reset() {
  _startTime = ar_time();
  _lastStart = _startTime;
  _runTime = 0.;
  _lapTime = 0;
  _running = false;
}

bool arTimer::running() const {
  return _running;
}

// Safely read from a (unix) pipe.
// @param pipeID Pipe's file descriptor
// @param theData Buffer into which the data gets packed
// @param numBytes Number of data bytes requested
#ifdef AR_USE_WIN_32
bool ar_safePipeRead(int, char*, int) {
  return false;
#else
bool ar_safePipeRead(int pipeID, char* theData, int numBytes) {
  while (numBytes>0) {
    const int n = read(pipeID, theData, numBytes);
    if (n<0) {
      return false;
    }
    numBytes -= n;
    theData += n;
  }
  return true;
#endif
}

// Safely read from a Unix pipe with timeout.
// @param piepID Pipe's file descriptor
// @param theData Buffer into which the data gets packed
// @param numBytes Number of data bytes requested
// @param timeout Maximum number of milliseconds we will block
// Returns number of bytes read.
#ifdef AR_USE_WIN_32
int ar_safePipeReadNonBlock(int, char*, int, int) {
  return 0;
#else
int ar_safePipeReadNonBlock(int pipeID, char* theData, int numBytes,
                            int timeout) {
  fd_set rset;
  fd_set wset;
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(pipeID, &rset);
  int maxFD = pipeID + 1;
  ar_timeval originalTime = ar_time();
  struct timeval waitTime;
  int requestedBytes = numBytes;
  while (numBytes>0) {
    const double elapsedMicroseconds = ar_difftime(ar_time(), originalTime);
    const int remainingTime = timeout*1000 - int(elapsedMicroseconds);
    if (remainingTime <= 0) {
      break;
    }
    waitTime.tv_sec = remainingTime/1000000;
    waitTime.tv_usec = remainingTime%1000000;
    select(maxFD, &rset, &wset, NULL, &waitTime);
    if (FD_ISSET(pipeID, &rset)) {
      int n = read(pipeID, theData, numBytes);
      if (n<0) {
        // This is an error, I think
        break;
      }
      numBytes -= n;
      theData += n;
    }
  }
  return requestedBytes - numBytes;
#endif
}

// for some unknown reason, VC++ 6.0 barfs on linking TestLanguageClient.exe
// if this isn't in here
void stupid_compiler_placeholder() {
}

// Safely write to a (unix) pipe.
// @param pipeID Pipe's file descriptor
// @param theData Buffer from which data is read
// @param numBytes number of data bytes written
#ifdef AR_USE_WIN_32
bool ar_safePipeWrite(int, const char*, int) {
  return false;
#else
bool ar_safePipeWrite(int pipeID, const char* theData, int numBytes) {
  while (numBytes>0) {
    const int n = write(pipeID, theData, numBytes);
    if (n<0) {
      return false;
    }
    numBytes -= n;
    theData += n;
  }
  return true;
#endif
}

// The following functions are platform dependent (endianness).

void ar_packData(ARchar* dst, const void* src, arDataType type, int dim) {
  memcpy(dst, src, dim * arDataTypeSize(type));
}

void ar_unpackData(const ARchar* src, void* dst, arDataType type, int dim) {
  memcpy(dst, src, dim * arDataTypeSize(type));
}

ARint ar_rawDataGetSize(ARchar* data) {
  ARint result = -1;
  ar_unpackData(data, (void*) &result, AR_INT, 1);
  return result;
}

ARint ar_rawDataGetID(ARchar* data) {
  ARint result = -1;
  ar_unpackData(data+AR_INT_SIZE, (void*) &result, AR_INT, 1);
  return result;
}

ARint ar_rawDataGetFields(ARchar* data) {
  ARint result = -1;
  ar_unpackData(data+2*AR_INT_SIZE, (void*) &result, AR_INT, 1);
  return result;
}

arStreamConfig ar_getLocalStreamConfig() {
  arStreamConfig config;
  config.endian = AR_ENDIAN_MODE;
  // Bug: the other fields are uninitialized garbage. Should arStreamConfig be a class not a struct, with a proper constructor?
  return config;
}

ARint ar_translateInt(ARchar* buffer, arStreamConfig conf) {
  ARint result = -1;
  char* dest = (char*) &result;
  if (conf.endian == AR_ENDIAN_MODE) {
    dest[0] = buffer[0];
    dest[1] = buffer[1];
    dest[2] = buffer[2];
    dest[3] = buffer[3];
  }
  else{
    dest[0] = buffer[3];
    dest[1] = buffer[2];
    dest[2] = buffer[1];
    dest[3] = buffer[0];
  }
  return result;
}

ARint ar_fieldSize(arDataType theType, ARint dim) {
  // Pad char fields to the next 4-byte boundary.
  ARint result = arDataTypeSize(theType) * dim;
  if (theType == AR_CHAR && dim%4)
    result += 4 - dim%4;
  return result;
}


// want double fields to be aligned on an 8-byte boundary....
// but there's no reason to make everyone align this way
ARint ar_fieldOffset(arDataType theType, ARint startOffset) {
  return (theType == AR_DOUBLE && startOffset%8) ? 4 : 0;
}

// Special case of ar_translateField(...arInt...).
// Implicitly assumes that AR_INT_SIZE == 4.
ARint ar_translateInt(ARchar* dest, ARint& destPos,
                     ARchar* src, ARint& srcPos, arStreamConfig conf) {
  src += srcPos;
  dest += destPos;
  if (conf.endian != AR_ENDIAN_MODE) {
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
  }
  else{
#ifdef THIS_IS_THE_VERBOSE_WAY
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
#else
    *(ARint*)dest = *(ARint*)src;
#endif
  }
  srcPos += AR_INT_SIZE;
  destPos += AR_INT_SIZE;
  return *(ARint*)dest;
}

void ar_translateField(ARchar* dest, ARint& destPos,
                       ARchar* src, ARint& srcPos,
                       arDataType theType, ARint dim, arStreamConfig conf) {
  srcPos  += ar_fieldOffset(theType, srcPos);
  destPos += ar_fieldOffset(theType, destPos);
  const ARint srcFieldSize = ar_fieldSize(theType, dim);
  src  += srcPos;
  dest += destPos;
  if (conf.endian == AR_ENDIAN_MODE) {
    memcpy(dest, src, srcFieldSize);
  }
  else{
    int i;
    switch (arDataTypeSize(theType)) {
      case 1:
        memcpy(dest, src, dim);
        break;
      case 4:
        for (i=0; i<dim; i++) {
          dest[0] = src[3];
          dest[1] = src[2];
          dest[2] = src[1];
          dest[3] = src[0];
          src  += 4;
          dest += 4;
        }
        break;
      case 8:
        for (i=0; i<dim; i++) {
          dest[0] = src[7];
          dest[1] = src[6];
          dest[2] = src[5];
          dest[3] = src[4];
          dest[4] = src[3];
          dest[5] = src[2];
          dest[6] = src[1];
          dest[7] = src[0];
          src  += 8;
          dest += 8;
        }
        break;
    }
  }
  srcPos  += srcFieldSize;
  destPos += srcFieldSize;
}

void ar_usleep(int microseconds) {
#if defined(AR_USE_WIN_32)
  Sleep(microseconds/1000);
#elif defined(AR_USE_SGI)
  if (microseconds >= 1000000)
    // usleep fails in this case.  Sigh.
    sginap(microseconds * CLK_TCK / 1000000);
  else
    usleep(microseconds);
#else
  usleep(microseconds);
#endif
}

arSleepBackoff::arSleepBackoff(const float msecMin, const float msecMax, const float ratio) :
  _uMin(msecMin*1000.),
  _uMax(msecMax*1000.),
  _ratio(ratio) {
  const float uMinMin = 1000.;
  if (_uMin < uMinMin) {
    ar_log_error() << "arSleepBackoff minimum " << _uMin << " rounded up to " << uMinMin/1000. << " msec.\n";
    _uMin = uMinMin;
  }
  if (_uMax < _uMin) {
    ar_log_error() << "arSleepBackoff maximum " << _uMax << " rounded up to minimum " << _uMin << ".\n";
    _uMax = _uMin;
  }
  const float ratioMin = 1.001;
  if (_ratio < ratioMin) {
    ar_log_error() << "arSleepBackoff ratio " << _ratio << " rounded up to " << ratioMin << ".\n";
    _ratio = ratioMin;
  }
  reset();
}

void arSleepBackoff::sleep() {
  ar_usleep(int(_u));
  _uElapsed += _u;
  _u *= _ratio;
  if (_u > _uMax)
    _u = _uMax;
}

void arSleepBackoff::reset() {
  _u = _uMin;
  resetElapsed();
}

void arSleepBackoff::resetElapsed() {
  _uElapsed = 0;
}

// string->number conversions with error checking
bool ar_stringToLongValid( const string& theString, long& theLong ) {
  if (theString == "NULL") {
    // Don't complain.  Let the caller do that: it has more context.
    return false;
  }
  char *endPtr = NULL;
  theLong = strtol( theString.c_str(), &endPtr, 10 );
  if ((theString.c_str()+theString.size())!=endPtr) {
    ar_log_error() << "arStringToLong failed to convert '" << theString << "'." << ar_endl;
    return false;
  }

  if ((theLong==LONG_MAX || theLong==LONG_MIN) && errno==ERANGE) {
    ar_log_error() << "arStringToLong clipped out-of-range '" << theString << "' to " << theLong << ar_endl;
    return false;
  }
  return true;
}

bool ar_longToIntValid( const long theLong, int& theInt ) {
  theInt = (int)theLong;
  if ((theLong > INT_MAX)||(theLong < INT_MIN)) {
    ar_log_error() << "arLongToIntValid: out-of-range " << theLong << ".\n";
    return false;
  }
  return true;
}

// Returns 0 on error (string too long, or atoi fails).
int ar_stringToInt(const string& s)
{
  const unsigned int maxlen = 30;
  char buf[maxlen+2]; // Fixed size buffer is okay here, for a single int.
  if (s.length() > maxlen) {
    ar_log_error() << "arStringToInt: '" << s << "' too long, returning 0.\n";
    return 0;
  }

  ar_stringToBuffer(s, buf, sizeof(buf));
  return atoi(buf);
}

string ar_intToString(const int i) {
  const unsigned int maxlen = 30;
  char buf[maxlen+2]; // Fixed size buffer is okay here, for a single int.
  // snprintf(buf, maxlen, "%d", i);
  sprintf(buf, "%d", i);
  return string(buf);
}

bool ar_stringToIntValid( const string& theString, int& theInt ) {
  long theLong = -1;
  return ar_stringToLongValid( theString, theLong ) &&
    ar_longToIntValid( theLong, theInt );
}

bool ar_stringToDoubleValid( const string& theString, double& theDouble ) {
  if (theString == "NULL") {
    ar_log_error() << "arStringToDouble ignoring 'NULL'.\n";
    return false;
  }

  char *endPtr = NULL;
  theDouble = strtod( theString.c_str(), &endPtr );
  if ((theString.c_str()+theString.size()) != endPtr) {
    ar_log_error() << "arStringToDouble failed to convert '" << theString << "'.\n";
    return false;
  }

  if ((theDouble==HUGE_VAL || theDouble==-HUGE_VAL || theDouble==0.) && errno==ERANGE) {
    ar_log_error() << "arStringToDouble clipped out-of-range '" << theString << "' to " << theDouble << ar_endl;
    return false;
  }
  return true;
}

void ar_stringToBuffer(const string& s, char* buf, int len) {
  if (len <= 0) {
    ar_log_error() << "ar_stringToBuffer: nonpositive length.\n";
    *buf = '\0';
    return;
  }

  if (s.length() < unsigned(len)) {
    strcpy(buf, s.c_str());
  }
  else {
    ar_log_error() << "ar_stringToBuffer truncating '" << s << "' after " <<
      len << " characters.\n";
    strncpy(buf, s.c_str(), len);
    buf[len-1] = '\0';
  }
}

bool ar_doubleToFloatValid( const double theDouble, float& theFloat ) {
  theFloat = (float)theDouble;
  // Ignore loss of precision.
  if ((theDouble > FLT_MAX)||(theDouble < -FLT_MAX)) {
    ar_log_error() << "arDoubleToFloatValid: out-of-range " << theDouble << ".\n";
    return false;
  }
  return true;
}

bool ar_stringToFloatValid( const string& theString, float& theFloat ) {
  double theDouble;
  return ar_stringToDoubleValid( theString, theDouble ) &&
         ar_doubleToFloatValid( theDouble, theFloat );
}

// len is the size of outArray.
int ar_parseFloatString(const string& theString, float* outArray, int len) {

  // Converts a string which is a slash-delimited sequence of floats
  // into an array of floats.
  // Returns how many floats were found.
  // Returns 0 or sometimes 1 if none are found (atof's error handling sucks).
  // input example = "0.998/-0.876/99.87/3.4/5/17"

  if (len <= 0) {
    ar_log_error() << "arParseFloatString: nonpositive length.\n";
    return -1;
  }

  char buf[1024]; // todo: fixed size buffer
  ar_stringToBuffer(theString, buf, sizeof(buf));
  const int length = theString.length();
  if (length < 1 || ((*buf<'0' || *buf>'9') && *buf!='.' && *buf!='-'))
    return 0;

  int currentPosition = 0;
  int dimension = 0;
  bool flag = false;
  while (!flag) {
    if (dimension >= len) {
      ar_log_error() << "arParseFloatString truncating '" <<
        theString << "' after " << len << " floats.\n";
      return dimension;
    }
    outArray[dimension++] = atof(buf+currentPosition);
    while (buf[currentPosition]!='/' && !flag) {
      if (++currentPosition >= length) {
        flag = true;
      }
    }
    ++currentPosition;
  }
  return dimension;
}

// todo: refactor massive copypaste ar_parseIntString ar_parseLongString

int ar_parseIntString(const string& theString, int* outArray, int len) {
  // Takes a string of slash-delimited ints and fills an array of ints.
  // Returns how many ints were found, possibly fewer than len (which is ok).

  if (len <= 0) {
    ar_log_error() << "ar_parseIntString: nonpositive length.\n";
    return -1;
  }
  if (!outArray) {
    ar_log_error() << "ar_parseIntString: NULL target.\n";
    return -1;
  }

  const int length = theString.length();
  if (length < 1)
    return 0;

  string localString = theString; //;;;; copy only if needed.  Use a string* pointing to the original or the copy.
  if (localString[length-1] != '/')
    localString += "/";
  std::istringstream inStream( localString );
  int numValues = 0;
  string wordString;
  while (numValues < len) {
    getline( inStream, wordString, '/' );
    // "&& numValues > 1" handles the case of a 1-int string?
    if (wordString == "" && numValues > 1) {
      // Empty field.  Don't complain.  It just means we're done.
      break;
    }
    if (inStream.fail()) {
      // NOTE: This is sometimes OK, i.e. if you have to parse a
      // variable-length item list and you give the max. num. items...
      ar_log_debug() << "ar_parseIntString: stream failed during field " <<
        numValues+1 << " of " << len << " in '" << theString << "'.\n";
      break;
    }
    if (wordString == "NULL") {
      // Don't complain.  Let the caller do that: it has more context.
      break;
    }
    int theInt = -1;
    if (!ar_stringToIntValid( wordString, theInt )) {
      ar_log_error() << "ar_parseIntString: invalid field '" << wordString << "'.\n";
      break;
    }
    outArray[numValues++] = theInt;
  }
  return numValues;
}

int ar_parseLongString(const string& theString, long* outArray, int len) {
  // takes a string which is a sequence of longs delimited by /
  // and fills an array of longs
  // Returns how many longs were found.

  if (len <= 0) {
    ar_log_error() << "ar_parseLongString: nonpositive length.\n";
    return -1;
  }
  if (!outArray) {
    ar_log_error() << "ar_parseLongString: NULL target.\n";
    return -1;
  }

  string localString = theString; //;;;; copy only if needed.  Use a string* pointing to the original or the copy.
  const int length = localString.length();
  if (length < 1)
    return 0;

  if (localString[length-1] != '/')
    localString += "/";
  std::istringstream inStream( localString );
  int numValues = 0;
  string wordString;
  while (numValues < len) {
    getline( inStream, wordString, '/' );
    if (wordString == "") {
      ar_log_error() << "ar_parseLongString: empty field in '" << theString << "'.\n";
      break;
    }
    if (inStream.fail())
      // Error message?
      break;
    long l = -1;
    if (!ar_stringToLongValid( wordString, l )) {
      ar_log_error() << "ar_parseLongString: invalid field '" << wordString << "'.\n";
      break;
    }
    outArray[numValues++] = l;
  }
  return numValues;
}

// todo: unify arPathToken with ar_semicolonstring; use an iterator pattern.

string ar_pathToken(const string& theString, int& start) {
  // starts scanning theString at location start until the first ';'
  // appears (or end of file). start is modified to be the first position
  // after the ';'... for easier iterative calling
  const int length = theString.length();
  const int original = start;
  while (start<length && theString[start]!=';')
    ++start;
  string result;
  if (start>original)
    result.assign(theString, original, start-original);
  ++start; // skip past the ';'
  return result;
}

// Returns the path delimiter for the particular operating system.
char ar_pathDelimiter() {
#ifdef AR_USE_WIN_32
  return '\\';
#else
  return '/';
#endif
}

// Returns the other delimiter. This is used, for instance, to automatically
// convert Win32 paths to Unix paths. This can be useful for software
// that must store is data in portable way across various systems.
char ar_oppositePathDelimiter() {
#ifdef AR_USE_WIN_32
  return '/';
#else
  return '\\';
#endif
}

// Makes sure a given path string has the right delimiter character for
// our OS (i.e / or \)
string& ar_fixPathDelimiter(string& s) {
  for (unsigned int i=0; i<s.length(); i++) {
    if (s[i] == ar_oppositePathDelimiter()) {
      s[i] = ar_pathDelimiter();
    }
  }
  return s;
}

string& ar_scrubPath(string& s) {
  ar_log_warning() << "ar_scrubPath() is deprecated, please change to ar_fixPathDelimiter() (same syntax).\n";
  return ar_fixPathDelimiter( s );
}

// Append a path-separator to arg; return arg.
string& ar_pathAddSlash(string& s) {
  if (s[s.length()-1] != ar_pathDelimiter())
    s += ar_pathDelimiter();
  return s;
}

int arDelimitedString::size() const {
  const unsigned len = length();
  if (len <= 0)                        // empty string
    return 0;
  const char* cstr = this->c_str();
  return 1 + std::count( cstr, cstr+len, _delimiter );
}

string arDelimitedString::operator[](int which) const {
  const int numstrings = size();
  if (which < 0 || which >= numstrings)
    return string("");

  // find the beginning of this particular substring
  int i = 0;
  for (int j = 0; j < which; ++j) {
    // skip past the next slash
    i = find(_delimiter, i) + 1;
  }
  // find the end of the substring
  if (which == numstrings-1) {
    // the last substring
    return substr(i, length()-i);
  }
  return substr(i, find(_delimiter, i) - i);
}
// Append delimiter IF length <> 0 and it doesn't already end with one
void arDelimitedString::appendDelimiter() {
  if (length() == 0)
    return;
  if (string::operator[](length()-1) == _delimiter)
    return;
  char delim[2];
  delim[0] = _delimiter;
  delim[1] = '\0';
  append(string(delim));
}

arDelimitedString& arDelimitedString::operator/=(const string& rhs) {
  appendDelimiter();
  return (arDelimitedString&)append(rhs);
}

arSlashString& arSlashString::operator/=(const string& rhs) {
  appendDelimiter();
  return (arSlashString&)append(rhs);
}

arSemicolonString& arSemicolonString::operator/=(const string& rhs) {
  appendDelimiter();
  return (arSemicolonString&)append(rhs);
}

arPathString& arPathString::operator/=(const string& rhs) {
  appendDelimiter();
  return (arPathString&)append(rhs);
}

bool ar_getTokenList( const string& inString,
                      vector<string>& outList,
                      const char delim) {
  std::istringstream inputStream(inString);
  do {
    string s;
    getline( inputStream, s, delim );
    if (s != "")
      outList.push_back(s);
  } while (!inputStream.fail());
  return true;
}

string ar_packParameters(int argc, char** argv) {
  string result;
  for (int i=0; i<argc; i++) {
    const string nextParam(argv[i]);
    result += nextParam;
    if (i != argc-1) {
      result += " ";
    }
  }
  return result;
}

// Reduce an executable name to a canonical form,
// to convert argv[0] into a component name.
// Remove the path.  On Win32, remove the trailing .EXE.
string ar_stripExeName(const string& name) {
  // Find the last '/' or '\'.
  const int position = name.find_last_of("/\\") == string::npos ?
    0 : name.find_last_of("/\\")+1;

  bool extension = false;
#ifdef AR_USE_WIN_32
  // Some win32 shells append a ".exe".  Truncate such extensions.
  if (name.length() >= 4) {
    const string& lastFour = name.substr(name.length()-4, 4);
    extension = lastFour == ".EXE" || lastFour == ".exe";
  }
#endif
  const int length = name.length()-position - (extension ? 4 : 0);
  return name.substr(position, length);
}


// Return the path to the currently-running executable.
string ar_currentExePath() {
#ifndef AR_USE_WIN_32
  string fullFileName("");
  // Code taken from: http://www.gamedev.net/community/forums/topic.asp?topic_id=459511
  string path("");
  pid_t pid = getpid();
  ostringstream os;
  os << "/proc/" << pid << "/exe"; 
  char proc[512];
  int ch = readlink( os.str().c_str(), proc, 512 );
  if (ch != -1) {
    proc[ch] = 0;
    path = proc;
    string::size_type t = path.find_last_of("/");
    path = path.substr(0,t);
  }
  fullFileName = path + string("/");
  
  return fullFileName;
#else

  std::vector<char> executablePath(MAX_PATH);

  // Try to get the executable path with a buffer of MAX_PATH characters.
  DWORD result = ::GetModuleFileNameA(
    NULL, &executablePath[0], static_cast<DWORD>(executablePath.size())
  );

  // As long the function returns the buffer size, it is indicating that the buffer
  // was too small. Keep enlarging the buffer by a factor of 2 until it fits.
  while(result == executablePath.size()) {
    executablePath.resize(executablePath.size() * 2);
    result = ::GetModuleFileNameA(
      NULL, &executablePath[0], static_cast<DWORD>(executablePath.size())
    );
  }

  // If the function returned 0, something went wrong
  if(result == 0) {
    ar_log_error() << "GetModuleFileNameA() failed.\n";
    return "NULL";
  }

  // We've got the path, construct a standard string from it
  return std::string( executablePath.begin(), executablePath.begin() + result );
  
#endif
}

// Given a full path executable name, return just the path. For example,
// if the executable is:
//   /home/public/schaeffr/bin/linux/atlantis
// return:
//   /home/public/schaeffr/bin/linux
// This is used in szgd when setting the DLL library path.
string ar_exePath(const string& name) {
  unsigned int position = 0;
  if (name.find_last_of("/\\") != string::npos) {
    position = name.find_last_of("/\\");
  }
  return name.substr(0, position);
}

// Takes a file name, finds the last period in the name, and returns
// a string filled with the characters to the right of the period.
// We want to do this to determine if something is a python script,
// for instance.
string ar_getExtension(const string& name) {
  const string::size_type position = name.find_last_of(".");
  if (position == string::npos || position == name.length())
    return string("");
  return name.substr(position+1, name.length()-position-1);
}

// Append the appropriate shared lib extension for the system.
void ar_addSharedLibExtension(string& name) {
#ifdef AR_USE_WIN_32
  name += ".dll";
#else
  name += ".so";
#endif
}

// For ar_replaceAll(), pop the first word off the front of s, and return that word.
inline string popword(string& s, const string& delim) {
  const string::size_type i = s.find(delim);
  const string w(s.substr(0, i));
  if (i == string::npos)
    s.erase(); // s was only one word
  else
    s.erase(0, i + delim.size());
  return w;
}

string ar_replaceAll(const string& s, const string& from, const string& to) {
  vector<string> v;
  if (s.empty())
    return "";

  string t(s);
  while (!t.empty() && t.find(from) != string::npos)
    v.push_back(popword(t, from));
  v.push_back(t);
  ostringstream os;
  copy(v.begin(), v.end()-1, ostream_iterator<string>(os, to.c_str()));
  return os.str() + *(v.end() - 1);
}

void ar_setenv(const string& variable, const string& value) {

  // putenv is OS-dependent.  On Win32, the string is copied
  // into the environment. On Linux, it is referenced.
  // So create a new string each time,
  // a memory leak, so we can use putenv in linux AND win32.

  const int totalLength = variable.length()+value.length()+2;
  char* buf = new char[totalLength]; // memory leak
  ar_stringToBuffer(variable, buf, totalLength);
  int i = strlen(buf);
  buf[i++] = '=';
  ar_stringToBuffer(value, buf+i, totalLength-i);
  putenv(buf);
}

void ar_setenv(const string& variableName, int val) {
  ar_setenv(variableName, ar_intToString(val));
}

string ar_getenv(const string& variable) {
  char buf[1024]; // todo: fixed size buffer
  ar_stringToBuffer(variable, buf, sizeof(buf));
  const char* res = getenv(buf);
  return string(res ? res : "NULL");
}

#ifndef AR_USE_DARWIN
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
bool ar_getSzgEnv( map< string,string, less<string> >& envMap ) {
  ar_log_warning() << "ar_getSzgEnv() does not work with Visual C++ 8 & 9.\n";
  return false;
}
#else
extern char **environ;

bool ar_getSzgEnv( map< string,string, less<string> >& envMap ) {
  envMap.clear();
  if (environ == NULL) {
    ar_log_error() << "NULL environment pointer.\n";
    return false;
  }
  char **e;
	for (e = environ; *e != NULL; e++) {
		char *p = strchr(*e, '=');
		if (p == NULL)
			continue;
    string name( *e, (int)(p-*e) );
    if (name.find( "SZG" )!=0) {
      continue;
    }
    string value( p+1 );
    if (envMap.find( name )!=envMap.end()) {
      ar_log_warning() << "ar_getSzgEnv() ignoring duplicate environment entry.\n";
      continue;
    }
    envMap.insert( map< string, string, less<string> >::value_type
        (name, value) );
  }
  return true;
}
#endif
#else
bool ar_getSzgEnv( map< string,string, less<string> >& envMap ) {
  ar_log_warning() << "ar_getSzgEnv() does not work on MacOS.\n";
  return false;
}
#endif

string ar_getUser() {
#ifdef AR_USE_WIN_32
  const unsigned long size = 1024; // todo: fixed size buffer
  char buf[size];
  unsigned long sizeNew = size;
  GetUserName(buf, &sizeNew);
  return string(buf);
#else
  //***************************************************
  // this isn't quite correct... much better would
  // be an API function... this environment variable
  // scheme isn't guarenteed OK
  //***************************************************
  const char* s = getenv("USER");
  return string(s ? s : "NULL");
#endif
}

// Returns false if 2nd & 3rd args are invalid.
// If item does not exist (2nd arg == false), 3rd is invalid
// If item exists, 3rd arg indicates if it is a regular file
bool ar_fileExists( const string name, bool& exists, bool& isFile ) {
  if (!ar_fileItemExists( name, exists ))
    return false;
  if (exists)
    isFile = ar_isFile( name.c_str() );
  return true;
}

// Returns false if 2nd & 3rd args are invalid.
// If item does not exist (2nd arg == false), 3rd is invalid
// If item exists, 3rd arg indicates if it is a directory
bool ar_directoryExists( const string name, bool& exists, bool& isDirectory ) {
  if (!ar_fileItemExists( name, exists ))
    return false;
  if (exists)
    isDirectory = ar_isDirectory( name.c_str() );
  return true;
}

// return value indicates success, 2nd arg indicates existence
bool ar_fileItemExists( const string name, bool& exists ) {
  struct stat fileInfo;
  if (stat(name.c_str(), &fileInfo) == 0) {
    exists = true;
    return true;
  }
  if (errno == ENOENT) {
    exists = false;
    return true;
  }
#ifndef AR_USE_WIN_32
  if (errno == EOVERFLOW) {
    // 32-bit app queries a file longer than 2<<31 bits
    exists = true;
    return true;
  }
#endif
  ar_log_error() << "ar_fileItemExists: stat() failed for " << name << ".\n";
  return false;
}

bool ar_getWorkingDirectory( string& name ) {
  char dirBuf[1000]; // bug: fixed size buffer (but no overflow)
  if (getcwd( dirBuf, sizeof(dirBuf)-2 )==NULL)
    return false;
  name = string( dirBuf );
  return true;
}

bool ar_setWorkingDirectory( const string name ) {
  return chdir(name.c_str()) == 0;
}

bool ar_isFile(const char* name) {
  struct stat infoBuffer;
  stat(name, &infoBuffer);
  return (infoBuffer.st_mode & S_IFMT) == S_IFREG;
}

bool ar_isDirectory(const char* name) {
  struct stat infoBuffer;
  stat(name, &infoBuffer);
  return (infoBuffer.st_mode & S_IFMT) == S_IFDIR;
}

// todo: copy-paste from ar_fileOpen
// Returns the first file name of a file that can be opened on
// <path>/<subdirectory>/<name>
string ar_fileFind(const string& name,
                   const string& subdirectory,
                   const string& path) {
  FILE* result = NULL;
  string possiblePath("junk");
  if (name[0] != '/' && path != "NULL") { //;;;;;;;; copypaste this elsewhere
    // First, search the explicitly given path
    int location = 0;
    while (!result && possiblePath != "") {
      possiblePath = ar_pathToken(path, location);
      if (possiblePath == "")
	continue;
      if (subdirectory != "")
	possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
      possiblePath = ar_pathAddSlash(possiblePath)+name;
      // Make sure to "scrub" the path (i.e. replace '/' by '\' or vice-versa,
      // as required by platform. This is necessary to allow the subdirectory
      // to have multiple levels in a cross-platform sort of way.
      ar_fixPathDelimiter(possiblePath);
      result = fopen(possiblePath.c_str(), "r");
      if (result && ar_isDirectory(possiblePath.c_str())) {
	// Reject this directory.
	fclose(result);
	result = NULL;
      }
    }
  }

  // Next, try to find the file locally
  if (!result) {
    possiblePath = name;
    // Do not forget to "scrub" the path.
    ar_fixPathDelimiter(possiblePath);
    result = fopen(possiblePath.c_str(), "r");
    if (result && ar_isDirectory(possiblePath.c_str())) {
      // Reject this directory.
      fclose(result);
      result = NULL;
    }
  }

  if (!result) {
    return string("NULL");
  }

  fclose(result);
  return possiblePath;
}

// todo: copy-paste from ar_fileOpen and ar_fileFind
// Returns the first directory name determined like so:
// <path component>/<subdirectory>/<name>
string ar_directoryFind(const string& name,
                        const string& subdirectory,
                        const string& path) {
  // First, search the explicitly given path
  bool fileExists = false;
  bool isDirectory = false;
  bool result = false;
  int location = 0;
  string possiblePath("junk");
  while (!result && possiblePath != "") {
    possiblePath = ar_pathToken(path, location);
    if (possiblePath == "")
      continue;
    if (subdirectory != "")
      possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
    possiblePath = ar_pathAddSlash(possiblePath)+name;
    ar_fixPathDelimiter(possiblePath);
    if (ar_directoryExists(possiblePath, fileExists, isDirectory) &&
        isDirectory) {
      result = true;
    }
  }

  // Next, try to find the file locally
  if (!result) {
    possiblePath = name;
    ar_fixPathDelimiter(possiblePath);
    if (ar_directoryExists(possiblePath, fileExists, isDirectory) &&
        isDirectory) {
      result = true;
    }
  }

  return result ? possiblePath : string("NULL");
}

// todo: cut-and-paste with ar_directoryOpen and ar_fileFind
FILE* ar_fileOpen(const string& name,
                  const string& subdirectory,
                  const string& path,
                  const string& operation,
                  const char* warner) {
  // Search the path
  FILE* result = NULL;
  int location = 0;
  string possiblePath("junk");
  while (!result && possiblePath != "") {
    possiblePath = ar_pathToken(path, location);
    if (possiblePath == "")
      continue;
    if (subdirectory != "")
      possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
    possiblePath = ar_pathAddSlash(possiblePath)+name;
    // "Scrub" the path (i.e. replace '/' by '\' or vice versa),
    // for OS-independent subsubdirectories.
    ar_fixPathDelimiter(possiblePath);
    result = fopen(possiblePath.c_str(), operation.c_str());
    if (result && ar_isDirectory(possiblePath.c_str())) {
      fclose(result);
      result = NULL;
    }
  }

  if (result)
    ar_log_remark() << "ar_fileOpen opened '" << possiblePath << "' as '" << operation << "'.\n";

  // Find the file locally
  if (!result) {
    possiblePath = name;
    ar_fixPathDelimiter(possiblePath);
    result = fopen(possiblePath.c_str(), operation.c_str());
    if (result && ar_isDirectory(possiblePath.c_str())) {
      fclose(result);
      result = NULL;
    }
    if (result)
      ar_log_remark() << "ar_fileOpen opened '" << possiblePath << "' as '" << operation << "'.\n";
  }

  if (!result && warner) {
    ar_log_error() << warner << " failed to open '" << name << "' as '" << operation <<
      "' in subdirectory '" << subdirectory << "' on search path '" << path << "'.\n";

  }

  return result;
}

FILE* ar_fileOpen(const string& name, const string& path, const string& operation) {
  return ar_fileOpen(name, "", path, operation);
}

// If the file system entity referenced by "name" is not a directory,
// return the empty list. If it is, return a list containing the full
// paths to each of its contents (since this is what is needed
// for further recursive operations on those contents).
// We filter out the standard ".." entries, but not ".".
list<string> ar_listDirectory(const string& name) {
  // We will return a full path to the included file.
  string directoryPrefix = name;
  string fileNameString;
  ar_fixPathDelimiter(directoryPrefix);
  ar_pathAddSlash(directoryPrefix);
  list<string> result;
  struct stat statbuf;
  if (stat(name.c_str(), &statbuf) == -1) {
    // Cannot access the file system entity.
    return result;
  }
  if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
    // Exists but is not a directory.
    return result;
  }

#ifndef AR_USE_WIN_32

  DIR* directory = opendir(name.c_str());
  if (!directory) {
    return result;
  }

  dirent* directoryEntry = NULL;
  while ((directoryEntry = readdir(directory)) != NULL) {
    // There is another entry. Push the name.
    fileNameString = string( directoryEntry->d_name );
    if (fileNameString != "..") {
      result.push_back(directoryPrefix+fileNameString);
    }
  }
  closedir(directory);

#else

  // Browse a list of files that match a name (including wildcards).
  string fileSpecification(name);
  ar_fixPathDelimiter(fileSpecification);
  ar_pathAddSlash(fileSpecification);
  // Add a wildcard character, for everything in the directory.
  fileSpecification += "*";
  // Visual Studio .NET uses intptr_t below instead of int, but
  // Visual Studio 6 does not define intptr_t.
  // int may be safe, though 6's docs say long.
  _finddata_t fileinfo;
  const int fileHandle = _findfirst(fileSpecification.c_str(), &fileinfo);
  if (fileHandle == -1) {
    // No file matches the specification.
    return result;
  }

  result.push_back(directoryPrefix + string(fileinfo.name));
  while (_findnext(fileHandle, &fileinfo) != -1) {
    fileNameString = string( fileinfo.name );
    if (fileNameString != "..") {
      result.push_back(directoryPrefix + fileNameString);
    }
  }
  _findclose(fileHandle);

#endif

  return result;
}

int ar_fileClose(FILE* pf) {
  return pf ? fclose(pf) : 0;
}

/*
ifstream ar_istreamOpen(const string& name,
                  const string& subdirectory,
                  const string& path,
                  int mode ) {
  // First, search the explicitly given path
  char bName[1024]; // todo: fixed size buffer
  ar_stringToBuffer(name, bName, sizeof(bName));
  int location = 0;
  string partialPath("junk");
  string fullPath("junk");
  ifstream istr;
  while (!istr.is_open() && partialPath != "") {
    partialPath = ar_pathToken(path, location);
    if (partialPath == "")
      continue;
    // ar_pathAddSlash modifies its argument
    ar_pathAddSlash(partialPath);
    fullPath = partialPath+subdirectory;
    ar_stringToBuffer(ar_pathAddSlash(fullPath)+name,
                      bName, sizeof(bName));
    istr.open( bName, mode );
    if (istr.is_open() && ar_isDirectory(bName)) {
      // Reject this directory.
      istr.close();
    }
  }

  // Next, try to find the file locally
  if (!istr.is_open()) {
    ar_stringToBuffer(name, bName, sizeof(bName));
    istr.open( bName, mode );
    if (istr.is_open() && ar_isDirectory(bName)) {
      // Reject this directory.
      istr.close();
    }
  }

  return istr;
}

ifstream ar_istreamOpen(const string& name, const string& path, int mode ) {
  return ar_istreamOpen(name, "", path, mode);
}
*/

bool ar_growBuffer(ARchar*& buf, int& size, int newSize) {
  if (newSize <= size)
    return true;

  // At least double the size, if it grows at all.
  // This reduces calls to new [].
  const int doublesize = 2*size;
  size = (newSize > doublesize) ? newSize : doublesize;
  delete [] buf;
  buf = new ARchar[size];
  if (!buf) {
    ar_log_error() << "ar_growBuffer out of memory.\n";
    return false;
  }
  return true;
}

void* ar_allocateBuffer( arDataType theType, unsigned int size ) {
  void* buf = (void*) new ARchar[ size * arDataTypeSize( theType )];
  if (!buf) {
    ar_log_error() << "ar_allocateBuffer out of memory.\n";
  }
  return (void*) buf;
}

void ar_deallocateBuffer( void* ptr ) {
  delete[] (ARchar*) ptr;
}

void ar_copyBuffer( void* const outBuf, const void* const inBuf,
    arDataType theType, unsigned size ) {
  memcpy( outBuf, inBuf, size * arDataTypeSize(theType) );
}

void ar_refNodeList(list<arDatabaseNode*>& nodeList) {
  for (list<arDatabaseNode*>::iterator i = nodeList.begin();
       i != nodeList.end(); i++) {
    (*i)->ref();
  }
}

void ar_unrefNodeList(list<arDatabaseNode*>& nodeList) {
  for (list<arDatabaseNode*>::iterator i = nodeList.begin();
       i != nodeList.end(); i++) {
    (*i)->unref();
  }
}

// convert between vector<> of strings and '\0'-delimited char buffer.
char* ar_packStringVector( std::vector< std::string >& stringVec, unsigned& totalSize ) {
  totalSize = 0;
  std::vector< std::string >::iterator iter;
  for (iter=stringVec.begin(); iter != stringVec.end(); ++iter) {
    totalSize += iter->size()+1;
  }
  char* outbuf = new char[totalSize];
  if (!outbuf) {
    ar_log_error() << "ar_packStringVector() out of memory.\n";
    return NULL;
  }

  char* ptr = outbuf;
  for (iter=stringVec.begin(); iter != stringVec.end(); ++iter) {
    const unsigned strSize = iter->size();
    memcpy( ptr, iter->c_str(), strSize );
    ptr[strSize] = '\0';
    ptr += strSize + 1;
  }
  return outbuf;
}


void ar_unpackStringVector( char* inbuf, unsigned numStrings, std::vector< std::string >& stringVec ) {
  stringVec.clear();
  char* ptr = inbuf;
  for (unsigned i=0; i<numStrings; ++i) {
    stringVec.push_back( std::string(ptr) );
    ptr += strlen(ptr) + 1;
  }
}

void ar_vectorToArgcArgv( std::vector< std::string >& stringVec,
                                    int& argc, char**& argv ) {
  argc = stringVec.size();
  argv = new char*[argc];
  for (int i=0; i<argc; ++i) {
    const string& s = stringVec[i];
    argv[i] = new char[s.size()+1];
    memcpy( argv[i], s.c_str(), s.size()+1 );
  }
}

void ar_cleanupArgcArgv( int& argc, char**& argv ) {
  for (int i=0; i<argc; ++i) {
    delete[] argv[i];
  }
  delete[] argv;
  argv = NULL;
  argc = 0;
}

void ar_argcArgvToVector( int argc, char** argv, std::vector< std::string >& stringVec ) {
  stringVec.clear();
  for (int i=0; i<argc; ++i) {
    stringVec.push_back( string(argv[i]) );
  }
}

