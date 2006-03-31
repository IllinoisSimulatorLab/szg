//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataUtilities.h"
// Needed for the ar_refNodeList, etc.
#include "arDatabaseNode.h"
#include "arLogStream.h"
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <limits.h> // for CLK_TCK and error-checking conversions
#include <sstream>
#include "arSTLalgo.h"
using namespace std;

#ifndef AR_USE_WIN_32
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef AR_USE_WIN_32
#include <io.h>     // needed for directory listing
#include <direct.h>
#include <time.h>
#include <iostream>

/// \todo call WSACleanup somewhere!

bool ar_winSockInit(){
  WSADATA wsaData;
  switch(WSAStartup(MAKEWORD(2,0), &wsaData))
    {
  case 0:
    return true;
  case WSASYSNOTREADY:
    ar_log_error() << "syzygy client error initializing network: network not ready.\n";
    break;
  case WSAVERNOTSUPPORTED:
    ar_log_error() << "syzygy client error initializing network: wrong winsock "
	           << "version, expected 2.0.\n";
    break;
  case WSAEINPROGRESS:
    ar_log_error() << "syzygy client error initializing network: blocking winsock "
	           << "operation in progress.\n";
    break;
  case WSAEPROCLIM:
    ar_log_error() << "syzygy client error initializing network: winsock startup "
	           << "failed:  too many tasks.\n";
    break;
  case WSAEFAULT:
  default:
    ar_log_error() << "syzygy client error initializing network: ar_winSockInit "
	           << "internal error.\n";
    break;
    }
  return false;

}
#endif

// cross platform clock access functions.
#ifdef AR_USE_WIN_32

//;;;; this doxygen comment fails!
/// Current time.

/// \todo assumes that ticks/second fits in a single int, not true in a few years.
ar_timeval ar_time(){
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

ar_timeval ar_time(){
  struct timeval tNow;
  gettimeofday(&tNow, 0);
  return ar_timeval(tNow.tv_sec, tNow.tv_usec);
}

#endif

//;;;; this doxygen comment fails!
/// Take two times closer than 4000000000 usec and report their diff in usec.

double ar_difftime(const ar_timeval& a, const ar_timeval& b){
  return 1e6*(a.sec-b.sec) + (a.usec-b.usec);
}

// Return a nonzero value, safe to divide by for e.g. computing fps.
double ar_difftimeSafe(const ar_timeval& a, const ar_timeval& b){
  const double diff = ar_difftime(a, b);
  return diff > 1. ? diff : 1.;
}

//;;;; this doxygen comment fails!
/// Add two ar_timeval's together.
/// Returning a reference is safe since it's one of the arguments.

ar_timeval& ar_addtime(ar_timeval& result, const ar_timeval& addend){
  result.sec += addend.sec;
  if ((result.usec += addend.usec) >= 1000000) {
    result.usec -= 1000000;
    ++result.sec;
  }
  return result;
}

//;;;; this doxygen comment fails!
/// Returns current date & time in following format:
/// year:day of year:second of day/pretty human-readable string

string ar_currentTimeString() {
  time_t timeVal;
  time( &timeVal );
  struct tm* t = localtime( &timeVal );
  if (t==NULL)
    return string("");
  ostringstream stringStream;
  stringStream << (t->tm_year)+1900 << ":" << t->tm_yday << ":"
               << (t->tm_hour)*3600 + (t->tm_min)*60 + t->tm_sec;
  char* timePtr = ctime( &timeVal );
  if (timePtr==NULL)
    return stringStream.str();;
//   strip trailing carriage return
  int n = strlen(timePtr);
  if (timePtr[n-1] == '\n')
    timePtr[n-1] = '\0';
  stringStream << "/" << timePtr;
  return stringStream.str();
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
  if (_firstStart || dur > 0.)
    reset();
  else if (_running) // calling start while running resets lap timer
    stop();
  _lastStart = ar_time();
//  _runTime = 0.;
//  _lapTime = 0;
  _duration = dur;
  _running = true;
}

double arTimer::elapsed() {
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

double arTimer::totalTime() {
  return ar_difftime( ar_time(), _startTime );
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

bool arTimer::running() {
  return _running;
}

/// Safely read from a (unix) pipe.
/// @param pipeID Pipe's file descriptor
/// @param theData Buffer into which the data gets packed
/// @param numBytes Number of data bytes requested
bool ar_safePipeRead(int pipeID, char* theData, int numBytes){
#ifdef AR_USE_WIN_32
  return false;
#else
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

/// Safely read from a Unix pipe with timeout.
/// @param piepID Pipe's file descriptor
/// @param theData Buffer into which the data gets packed
/// @param numBytes Number of data bytes requested
/// @param timeout Maximum number of milliseconds we will block
/// Returns number of bytes read.
int ar_safePipeReadNonBlock(int pipeID, char* theData, int numBytes,
		            int timeout){
#ifdef AR_USE_WIN_32
  return 0;
#else
  fd_set rset;
  fd_set wset;
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(pipeID, &rset);
  int maxFD = pipeID + 1;
  ar_timeval originalTime = ar_time();
  struct timeval waitTime;
  int requestedBytes = numBytes;
  while (numBytes>0){
    const double elapsedMicroseconds = ar_difftime(ar_time(), originalTime);
    const int remainingTime = timeout*1000 - int(elapsedMicroseconds);
    if (remainingTime <= 0){
      break;
    }
    waitTime.tv_sec = remainingTime/1000000;
    waitTime.tv_usec = remainingTime%1000000;
    select(maxFD, &rset, &wset, NULL, &waitTime);
    if (FD_ISSET(pipeID, &rset)){
      int n = read(pipeID, theData, numBytes);
      if (n<0){
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
void stupid_compiler_placeholder(){
}

/// Safely write to a (unix) pipe.
/// @param pipeID Pipe's file descriptor
/// @param theData Buffer from which data is read
/// @param numBytes number of data bytes written
bool ar_safePipeWrite(int pipeID, const char* theData, int numBytes){
#ifdef AR_USE_WIN_32
  return false;
#else
  while (numBytes>0) {
    const int n = write(pipeID,theData,numBytes);
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

void ar_packData(ARchar* destination, const void* data,
                 arDataType type, int dimension){
  const int totalSize = dimension * arDataTypeSize(type);
  memcpy(destination,data,totalSize);
}

void ar_unpackData(const ARchar* source, void* destination,
                   arDataType type, int dimension){
  const int totalSize = dimension * arDataTypeSize(type);
  memcpy(destination,source,totalSize);
}

ARint ar_rawDataGetSize(ARchar* data){
  ARint result = -1;
  ar_unpackData(data,(void*) &result, AR_INT, 1);
  return result;
}

ARint ar_rawDataGetID(ARchar* data){
  ARint result = -1;
  ar_unpackData(data+AR_INT_SIZE,(void*) &result, AR_INT, 1);
  return result;
}

ARint ar_rawDataGetFields(ARchar* data){
  ARint result = -1;
  ar_unpackData(data+2*AR_INT_SIZE,(void*) &result, AR_INT, 1);
  return result;
}

arStreamConfig ar_getLocalStreamConfig(){
  arStreamConfig config;
  config.endian = AR_ENDIAN_MODE;
  return config;
}

ARint ar_translateInt(ARchar* buffer,arStreamConfig conf){
  ARint result = -1;
  char* dest = (char*) &result;
  if (conf.endian == AR_ENDIAN_MODE){
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

ARint ar_fieldSize(arDataType theType, ARint dim){
  // We pad out char fields to end on a 4-byte boundary.
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
                     ARchar* src, ARint& srcPos, arStreamConfig conf){
  src += srcPos;
  dest += destPos;
  if (conf.endian != AR_ENDIAN_MODE){
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
                       arDataType theType, ARint dim, arStreamConfig conf){
  srcPos  += ar_fieldOffset(theType,srcPos);
  destPos += ar_fieldOffset(theType,destPos);
  const ARint srcFieldSize = ar_fieldSize(theType,dim);
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
        for (i=0; i<dim; i++){
          dest[0] = src[3];
          dest[1] = src[2];
          dest[2] = src[1];
          dest[3] = src[0];
	  src  += 4;
	  dest += 4;
        }
        break;
      case 8:
        for (i=0; i<dim; i++){
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

void ar_usleep(int microseconds){
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

// string->number conversions with error checking
bool ar_stringToLongValid( const string& theString, long& theLong ) {
  if (theString == "NULL") {
    // Don't complain.  Let the caller do that: it has more context.
    return false;
  }
  char *endPtr = NULL;
  theLong = strtol( theString.c_str(), &endPtr, 10 );
  if ((theString.c_str()+theString.size())!=endPtr) {
    ar_log_warning() << "arStringToLong warning: conversion failed for '" << theString << "'." << ar_endl;
    return false;
  }
  if ((theLong==LONG_MAX)||(theLong==LONG_MIN)) {
    if (errno==ERANGE) {
      ar_log_warning() << "arStringToLong warning: string " << theString << " clipped to " << theLong << ar_endl;
      return false;
    }
  }
  return true;
}

bool ar_longToIntValid( const long theLong, int& theInt ) {
  theInt = (int)theLong;
  if ((theLong > INT_MAX)||(theLong < INT_MIN)) {
    ar_log_warning() << "arLongToIntValid warning: out-of-range value " << theLong << ".\n";
    return false;
  }
  return true;
}

bool ar_stringToIntValid( const string& theString, int& theInt ) {
  long theLong = -1;
  return ar_stringToLongValid( theString, theLong ) &&
    ar_longToIntValid( theLong, theInt );
}

bool ar_stringToDoubleValid( const string& theString, double& theDouble ) {
  char *endPtr = NULL;
  theDouble = strtod( theString.c_str(), &endPtr );
  if ((theString.c_str()+theString.size())!=endPtr) {
    ar_log_warning() << "arStringToDouble warning: conversion failed for " << theString << ar_endl;
    return false;
  }
  if ((theDouble==HUGE_VAL)||(theDouble==-HUGE_VAL)||(theDouble==0.)) {
    if (errno==ERANGE) {
      ar_log_warning() << "arStringToDouble warning: string " << theString << " clipped to " << theDouble << ar_endl;
      return false;
    }
  }
  return true;
}

bool ar_doubleToFloatValid( const double theDouble, float& theFloat ) {
  theFloat = (float)theDouble;
  // NOTE: We assume that we don't care about loss of precision, only values
  // out of range.
  if ((theDouble > FLT_MAX)||(theDouble < -FLT_MAX)) {
    ar_log_warning() << "arDoubleToFloatValid warning: value " << theDouble << " out of range.\n";
    return false;
  }
  return true;
}

bool ar_stringToFloatValid( const string& theString, float& theFloat ) {
  double theDouble;
  if (!ar_stringToDoubleValid( theString, theDouble ))
    return false;
  return ar_doubleToFloatValid( theDouble, theFloat );
}

// len is the size of outArray.
int ar_parseFloatString(const string& theString, float* outArray, int len){

  // Takes a string which is a sequence of floats delimited by /
  // and fills an array of floats.
  // Returns how many floats were found.
  // Returns 0 or sometimes 1 if none are found (atof's error handling sucks).
  // input example = 0.998/-0.876/99.87/3.4/5/17

  if (len <= 0) {
    ar_log_warning() << "arParseFloatString warning: nonpositive length!\n";
    return -1;
  }

  char buf[1024]; /// \todo fixed size buffer
  ar_stringToBuffer(theString, buf, sizeof(buf));
  const int length = theString.length();
  if (length < 1 || ((*buf<'0' || *buf>'9') && *buf!='.' && *buf!='-'))
    return 0;
  int currentPosition = 0;
  int dimension = 0;
  bool flag = false;
  while (!flag){
    if (dimension >= len){
      ar_log_warning() << "arParseFloatString warning: truncating \""
                       << theString << "\" after "
	               << len << " floats.\n";
      return dimension;
    }
    outArray[dimension++] = atof(buf+currentPosition);
    while (buf[currentPosition]!='/' && !flag){
      if (++currentPosition >= length){
        flag = true;
      }
    }
    ++currentPosition;
  }
  return dimension;
}

int ar_parseIntString(const string& theString, int* outArray, int len){
  // Takes a string of slash-delimited ints and fills an array of ints.
  // Returns how many ints were found.

  if (len <= 0) {
    ar_log_warning() << "ar_parseIntString warning: nonpositive length.\n";
    return -1;
  }
  if (!outArray) {
    ar_log_error() << "ar_parseIntString error: NULL target.\n";
    return -1;
  }

  int length = theString.length();
  if (length < 1)
    return 0;
  string localString = theString; //;;;; copy only if needed.  Use a string* pointing to the original or the copy.
  if (localString[length-1] != '/')
    localString = localString + "/";    
  std::istringstream inStream( localString );
  int numValues = 0;
  string wordString;
  while (numValues < len) {
    getline( inStream, wordString, '/' );
    // && numValues > 1, that's needed to handle the case of a 1-int string.  I think.
    if (wordString == "" && numValues > 1) {
      ar_log_warning() << "ar_parseIntString warning: empty field.\n";
      break;
    }
    if (inStream.fail())
      // Warn?
      break;
    if (wordString == "NULL") {
      // Don't complain.  Let the caller do that: it has more context.
      break;
    }
    int theInt = -1;
    if (!ar_stringToIntValid( wordString, theInt )) {
      ar_log_warning() << "ar_parseIntString warning: invalid field value \"" << wordString << "\".\n";
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
    ar_log_warning() << "arParseLongString warning: nonpositive length!\n";
    return -1;
  }

  string localString = theString; //;;;; copy only if needed.  Use a string* pointing to the original or the copy.
  int length = localString.length();
  if (length < 1)
    return 0;
  if (localString[length-1] != '/')
    localString = localString + "/";    
  std::istringstream inStream( localString );
  int numValues = 0;
  string wordString;
  while (numValues < len) {
    getline( inStream, wordString, '/' );
    if (wordString == "") {
      ar_log_warning() << "ar_parseLongString warning: empty field.\n";
      break;
    }
    if (inStream.fail())
      // Error message?
      break;
    long l = -1;
    if (!ar_stringToLongValid( wordString, l )) {
      ar_log_warning() << "ar_parseLongString warning: invalid field \"" << wordString << "\".\n";
      break;
    }
    outArray[numValues++] = l;
  }
  return numValues;
}

void ar_stringToBuffer(const string& s, char* buf, int len){
  if (len <= 0) {
    ar_log_warning() << "ar_stringToBuffer warning: nonpositive length!\n";
    *buf = '\0';
    return;
  }
  if (s.length() < unsigned(len)) {
    strcpy(buf, s.c_str());
  }
  else {
    ar_log_warning() << "ar_stringToBuffer warning: truncating \""
                     << s << "\" after "
		    << len << " characters.\n";
    strncpy(buf, s.c_str(), len);
    buf[len-1] = '\0';
  }
}

// Returns 0 on error (string too long, or atoi fails).
int ar_stringToInt(const string& s)
{
  const unsigned int maxlen = 30;
  char buf[maxlen+2]; // Fixed size buffer is okay here, for a single int.
  if (s.length() > maxlen) {
    ar_log_warning() << "arStringToInt warning: \""
                     << s << "\" too long, returning 0.\n";
    return 0;
  }
  ar_stringToBuffer(s, buf, sizeof(buf));
  return atoi(buf);
}

/// \todo unify arPathToken with ar_semicolonstring; use an iterator pattern.

string ar_pathToken(const string& theString, int& start){
  // starts scanning theString at location start until the first ';'
  // appears (or end of file). start is modified to be the first position
  // after the ';'... for easier iterative calling
  const int length = theString.length();
  const int original = start;
  while (start<length && theString[start]!=';')
    ++start;
  string result;
  if (start>original)
    result.assign(theString,original,start-original);
  ++start; // skip past the ';'
  return result;
}

/// Returns the path delimiter for the particular operating system.
char ar_pathDelimiter() {
#ifdef AR_USE_WIN_32
  return '\\';
#else
  return '/';
#endif
}

/// Returns the other delimiter. This is used, for instance, to automatically
/// convert Win32 paths to Unix paths. This can be useful for software
/// that must store is data in portable way across various systems.
char ar_oppositePathDelimiter(){
#ifdef AR_USE_WIN_32
  return '/';
#else
  return '\\';
#endif
}

/// Makes sure a given path string has the right delimiter character for
/// our OS (i.e / or \)
string& ar_scrubPath(string& s){
  for (unsigned int i=0; i<s.length(); i++){
    if (s[i] == ar_oppositePathDelimiter()){
      s[i] = ar_pathDelimiter();
    }
  }
  return s;
}

/// Append a path-separator to arg; return arg.
string& ar_pathAddSlash(string& s){
  if (s[s.length()-1] != ar_pathDelimiter())
    s += ar_pathDelimiter();
  return s;
}

int arDelimitedString::size() const {
  const unsigned len = length();
  if (len <= 0)			// empty string
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
  for (int j = 0; j < which; ++j){
    // skip past the next slash
    i = find(_delimiter,i) + 1;
  }
  // find the end of the substring
  if (which == numstrings-1){
    // the last substring
    return substr(i, length()-i);
  }
  return substr(i, find(_delimiter,i) - i);
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
    if (s!="")
      outList.push_back(s);
  } while (!inputStream.fail());
  return true;
}

string ar_packParameters(int argc, char** argv){
  string result;
  for (int i=0; i<argc; i++){
    const string nextParam(argv[i]);
    result += nextParam;
    if (i != argc-1){
      result += " ";
    }
  }
  return result;
}

/// Reduce an executable name to a canonical form,
/// to convert argv[0] into a component name.
/// Remove the path.  On Win32, remove the trailing .EXE.
string ar_stripExeName(const string& name){
  // Find the last '/' or '\'.
  const int position = name.find_last_of("/\\") == string::npos ?
    0 : name.find_last_of("/\\")+1;

  bool extension = false;
#ifdef AR_USE_WIN_32
  // Some win32 shells append a ".exe".  Truncate such extensions.
  if (name.length() >= 4){
    const string& lastFour = name.substr(name.length()-4, 4);
    extension = lastFour == ".EXE" || lastFour == ".exe";
  }
#endif
  const int length = name.length()-position - (extension ? 4 : 0);
  return name.substr(position, length);
}

/// Given a full path executable name, return just the path. For example,
/// if the executable is:
///   /home/public/schaeffr/bin/linux/atlantis
/// return:
///   /home/public/schaeffr/bin/linux
/// This is used in szgd when setting the DLL library path.
string ar_exePath(const string& name){
  unsigned int position = 0;
  if (name.find_last_of("/\\") != string::npos){
    position = name.find_last_of("/\\");
  }
  return name.substr(0, position);
}

/// Takes a file name, finds the last period in the name, and returns
/// a string filled with the characters to the right of the period.
/// We want to do this to determine if something is a python script,
/// for instance.
string ar_getExtension(const string& name){
  const unsigned position = name.find_last_of(".");
  if (position == string::npos || position == name.length())
    return string("");
  return name.substr(position+1, name.length()-position-1);
}

/// Append the appropriate shared lib extension for the system.
void ar_addSharedLibExtension(string& name){
#ifdef AR_USE_WIN_32
  name += ".dll";
#else
  name += ".so";
#endif
}

void ar_setenv(const string& variable, const string& value){

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

void ar_setenv(const string& variableName, int variableValue){
  // bug: overkill.  Just use sprintf.
  stringstream theValue;
  theValue << variableValue;
  ar_setenv(variableName, theValue.str().c_str());
}

string ar_getenv(const string& variable){
  char buf[1024]; /// \todo fixed size buffer
  ar_stringToBuffer(variable, buf, sizeof(buf));
  const char* res = getenv(buf);
  if (!res)
    return string("NULL");
  return string(res);
}

string ar_getUser(){
#ifdef AR_USE_WIN_32
  const unsigned long size = 1024; /// \todo fixed size buffer
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
  const char* res = getenv("USER");
  if (!res)
    return string("NULL");
  return string(res);
#endif
}

// If return value is false, 2nd & 3rd args are invalid.
// If item does not exist (2nd arg == false), 3rd is invalid
// If item exists, 3rd arg indicates whether or not it is a regular file
bool ar_fileExists( const string name, bool& exists, bool& isFile ) {
  if (!ar_fileItemExists( name, exists ))
    return false;
  if (exists)
    isFile = ar_isFile( name.c_str() );
  return true;
}

// If return value is false, 2nd & 3rd args are invalid.
// If item does not exist (2nd arg == false), 3rd is invalid
// If item exists, 3rd arg indicates whether or not it is a directory
bool ar_directoryExists( const string name, bool& exists, bool& isDirectory ) {
  if (!ar_fileItemExists( name, exists ))
    return false;
  if (exists)
    isDirectory = ar_isDirectory( name.c_str() );
  return true;
}

// return value indicates success or failure, 2nd arg indicates existence
bool ar_fileItemExists( const string name, bool& exists ) {
  struct stat fileInfo;
  if (stat(name.c_str(),&fileInfo) != 0) {
    if (errno == ENOENT) {
      exists = false;
      return true;
    } else {
      ar_log_error() << "ar_fileItemExists error: "
	             << "stat() failed for an unknown reason.\n";
      return false;
    }
  } else {
    exists = true;
    return true;
  }
}

bool ar_getWorkingDirectory( string& name ) {
  char dirBuf[1000];
  if (getcwd( dirBuf, 999 )==NULL)
    return false;
  name = string( dirBuf );
  return true;
}

bool ar_setWorkingDirectory( const string name ) {
  return (chdir( name.c_str() ) == 0);
}

bool ar_isFile(const char* name){
  struct stat infoBuffer;
  stat(name, &infoBuffer);
  return (infoBuffer.st_mode & S_IFMT) == S_IFREG;
}

bool ar_isDirectory(const char* name){
  struct stat infoBuffer;
  stat(name, &infoBuffer);
  return (infoBuffer.st_mode & S_IFMT) == S_IFDIR;
}

/// \todo copy-paste from ar_fileOpen
/// Returns the first file name of a file that can be opened on 
/// <path component>/<subdirectory>/<name>
string ar_fileFind(const string& name, 
		   const string& subdirectory,
		   const string& path){
  // First, search the explicitly given path
  FILE* result = NULL;
  int location = 0;
  string possiblePath("junk");
  while (!result && possiblePath != ""){
    possiblePath = ar_pathToken(path, location);
    if (possiblePath == "")
      continue;
    if (subdirectory == ""){
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    else{
      possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    // Make sure to "scrub" the path (i.e. replace '/' by '\' or vice-versa,
    // as required by platform. This is necessary to allow the subdirectory
    // to have multiple levels in a cross-platform sort of way.
    ar_scrubPath(possiblePath);
    result = fopen(possiblePath.c_str(), "r");
    if (result && ar_isDirectory(possiblePath.c_str())){
      // Reject this directory.
      fclose(result);
      result = NULL;
    }
  }
  
  // Next, try to find the file locally
  if (!result){
    possiblePath = name;
    // Do not forget to "scrub" the path.
    ar_scrubPath(possiblePath);
    result = fopen(possiblePath.c_str(),"r");
    if (result && ar_isDirectory(possiblePath.c_str())){
      // Reject this directory.
      fclose(result);
      result = NULL;
    }
  }

  if (result){
    fclose(result);
    return possiblePath;
  }
  else{
    return string("NULL");
  }
}

/// \todo copy-paste from ar_fileOpen and ar_fileFind
/// Returns the first directory name determined like so:
/// <path component>/<subdirectory>/<name>
string ar_directoryFind(const string& name, 
		        const string& subdirectory,
		        const string& path){
  // First, search the explicitly given path
  bool fileExists = false;
  bool isDirectory = false;
  bool result = false;
  int location = 0;
  string possiblePath("junk");
  while (!result && possiblePath != ""){
    possiblePath = ar_pathToken(path, location);
    if (possiblePath == "")
      continue;
    if (subdirectory == ""){
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    else{
      possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    ar_scrubPath(possiblePath);
    if (ar_directoryExists(possiblePath, fileExists, isDirectory) &&
        isDirectory){
      result = true;
    }
  }
  
  // Next, try to find the file locally
  if (!result){
    possiblePath = name;
    ar_scrubPath(possiblePath);
    if (ar_directoryExists(possiblePath, fileExists, isDirectory) &&
        isDirectory){
      result = true;
    }
  }

  return result ? possiblePath : string("NULL");
}

/// \todo cut-and-paste with ar_directoryOpen and ar_fileFind
FILE* ar_fileOpen(const string& name,
                  const string& subdirectory, 
                  const string& path,
                  const string& operation){
  // First, search the explicitly given path
  FILE* result = NULL;
  int location = 0;
  string possiblePath("junk");
  while (!result && possiblePath != ""){
    possiblePath = ar_pathToken(path, location);
    if (possiblePath == "")
      continue;
    if (subdirectory == ""){
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    else{
      possiblePath = ar_pathAddSlash(possiblePath)+subdirectory;
      possiblePath = ar_pathAddSlash(possiblePath)+name;
    }
    // Make sure to "scrub" the path (i.e. replace '/' by '\' or vice-versa,
    // as required by platform. This is necessary to allow the subdirectory
    // to have multiple levels in a cross-platform sort of way.
    ar_scrubPath(possiblePath);
    result = fopen(possiblePath.c_str(), operation.c_str());
    if (result && ar_isDirectory(possiblePath.c_str())){
      // Reject this directory.
      fclose(result);
      result = NULL;
    }
  }
  
  // Next, try to find the file locally
  if (!result){
    possiblePath = name;
    // Do not forget to "scrub" the path.
    ar_scrubPath(possiblePath);
    result = fopen(possiblePath.c_str(), operation.c_str());
    if (result && ar_isDirectory(possiblePath.c_str())){
      // Reject this directory.
      fclose(result);
      result = NULL;
    }
  }

  return result;
}

FILE* ar_fileOpen(const string& name, const string& path,
                  const string& operation){
  return ar_fileOpen(name, "", path, operation);
}

/// If the file system entity referenced by "name" is not a directory,
/// return the empty list. If it is, return a list containing the full
/// paths to each of its contents (since this is what is needed
/// for further recursive operations on those contents).
/// NOTE: we do NOT filter out the standard "." and ".."
/// entries.
list<string> ar_listDirectory(const string& name){
  // We will return a full path to the included file.
  string directoryPrefix = name;
  ar_scrubPath(directoryPrefix);
  ar_pathAddSlash(directoryPrefix);
  list<string> result;
  struct stat statbuf;
  if (stat(name.c_str(), &statbuf) == -1){
    // Cannot access the file system entity.
    return result;
  }
  if ((statbuf.st_mode & S_IFMT) != S_IFDIR){
    // Exists but is not a directory.
    return result;
  }

  // Determining contents of a directory is VERY different in Unix and Win32.
#ifndef AR_USE_WIN_32
  DIR* directory = opendir(name.c_str());
  if (!directory){
    return result;
  }
  dirent* directoryEntry = NULL;
  while ((directoryEntry = readdir(directory)) != NULL){
    // There is another entry. Push the name.
    result.push_back(directoryPrefix+string(directoryEntry->d_name));
  }
  closedir(directory);
#else
  // Browse through a list of files that match a name
  // (including wildcards). 
  string fileSpecification = name;
  // Make sure the path is "scrubbed" and that it has a trailing slash of the
  // right type.
  ar_scrubPath(fileSpecification);
  ar_pathAddSlash(fileSpecification);
  // Finally, make sure that we add a "wildcard" character, since we want
  // everything in the directory.
  fileSpecification += "*";
  _finddata_t fileinfo;
  // NOTE: Visual Studio .NET uses intptr_t below instead of int, but
  // Visual Studio 6 does not define intptr_t. I'm guessing that int is
  // safe (the docs for VS 6 say long).
  int fileHandle = _findfirst(fileSpecification.c_str(), &fileinfo);
  if (fileHandle == -1){
    // No file matching the "specification".
    return result;
  }
  result.push_back(directoryPrefix+string(fileinfo.name));
  while (_findnext(fileHandle, &fileinfo) != -1)
    result.push_back(directoryPrefix+string(fileinfo.name));
  _findclose(fileHandle);
#endif
  return result;
}

int ar_fileClose(FILE* pf){
  return !pf ? 0 : fclose(pf);
}

/*
ifstream ar_istreamOpen(const string& name,
                  const string& subdirectory, 
                  const string& path,
                  int mode ){
  // First, search the explicitly given path
  char bName[1024]; /// \todo fixed size buffer
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
  if (!istr.is_open()){
    ar_stringToBuffer(name, bName, sizeof(bName));
    istr.open( bName, mode );
    if (istr.is_open() && ar_isDirectory(bName)){
      // Reject this directory.
      istr.close();
    }
  }

  return istr;
}

ifstream ar_istreamOpen(const string& name, const string& path, int mode ){
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
    ar_log_warning() << "syzygy warning: buffer out of memory.\n";
    return false;
  }
  return true;
}

void* ar_allocateBuffer( arDataType theType, unsigned int size ) {
  return (void*) new ARchar[ size * arDataTypeSize( theType )];
}

void ar_deallocateBuffer( void* ptr ) {
  delete[] (ARchar*) ptr;
}

void ar_copyBuffer( void* const outBuf, const void* const inBuf,
    arDataType theType, unsigned int size ) {
  memcpy( outBuf, inBuf, size * arDataTypeSize(theType) );
}

void ar_refNodeList(list<arDatabaseNode*>& nodeList){
  for (list<arDatabaseNode*>::iterator i = nodeList.begin();
       i != nodeList.end(); i++){
    (*i)->ref();
  }
}

void ar_unrefNodeList(list<arDatabaseNode*>& nodeList){
  for (list<arDatabaseNode*>::iterator i = nodeList.begin();
       i != nodeList.end(); i++){
    (*i)->unref();
  }
}
