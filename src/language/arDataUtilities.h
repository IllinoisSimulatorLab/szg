//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_UTILITIES
#define AR_DATA_UTILITIES

#ifdef AR_USE_WIN_32
#include "arPrecompiled.h"
#endif

#include "arDataType.h"
#include "arSocket.h"

#include "arLanguageCalling.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <fstream>

using namespace std;

#ifdef AR_USE_WIN_32
SZG_CALL string ar_getLastWin32ErrorString();
#endif


// Version info

// Syzygy version string, e.g. "1.3.0" or "Syzygy version: 1.3.0.\n"
SZG_CALL string ar_versionString(bool fVerbose=true);

// Bazaar version info. Returns meaningful info only if
// bzr and Python are present when libraries are built.
SZG_CALL string ar_versionInfo(bool fVerbose=true);


// Small utility classes.

// Wrapper for 3 ints.
class SZG_CALL arIntTriple{
 public:
  int a, b, c;
  arIntTriple(): a(0), b(0), c(0) {}
  arIntTriple(int x, int y, int z): a(x), b(y), c(z) {}
};


// Timing.

#ifdef AR_USE_WIN_32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

// Internal representation of time.

class SZG_CALL ar_timeval {
 public:
  int sec;
  int usec;
  ar_timeval() : sec(0), usec(0) {}
  ar_timeval(int s, int u) : sec(s), usec(u) {}
  bool zero() const { return sec==0 && usec==0; }
};

SZG_CALL ar_timeval ar_time();
SZG_CALL double ar_difftime(const ar_timeval& end, const ar_timeval& start);
SZG_CALL double ar_difftimeSafe(const ar_timeval& end, const ar_timeval& start);
SZG_CALL ar_timeval& ar_addtime(ar_timeval& result,
                                const ar_timeval& addend); // returns "result"
SZG_CALL string ar_currentTimeString();

class SZG_CALL arTimer {
  public:
    arTimer( double dur = 0. );
    virtual ~arTimer() {}
    void start( double dur = 0. );
    bool running() const;
    // time since last restart() (or construction or start(), if none)
    double lapTime();
    // time running since construction or start()
    double runningTime();
    // time, running or not, since construction or start()
    double totalTime() const;
    double elapsed(); // deprecated: use totalTime()
    bool done();
    void stop();
    void reset();
    void setRuntime( double dur ) { _runTime = dur; }
    double duration() const { return _duration; }
  private:
    bool _firstStart;
    ar_timeval _startTime;
    ar_timeval _lastStart;
    double _duration;
    double _lapTime;
    double _runTime;
    bool _running;
};

// Manipulating records.

// Configuration of a data stream.
struct SZG_CALL arStreamConfig{
  ARchar endian;
  int  version;
  int  ID;
  bool valid; // For a success/failure return value.
  bool refused; // If true, the other end rejected our connection attempt.
};

#if defined(AR_BIG_ENDIAN)
// paranoia for Darwin
#undef AR_BIG_ENDIAN
#endif
SZG_CALL enum {
  AR_LITTLE_ENDIAN = 0,
  AR_BIG_ENDIAN,
  AR_UNDEFINED_ENDIAN,
  AR_GARBAGE_ENDIAN
};

#if defined( AR_USE_LINUX )
  #define AR_ENDIAN_MODE AR_LITTLE_ENDIAN
#elif defined( AR_USE_WIN_32 )
  #define AR_ENDIAN_MODE AR_LITTLE_ENDIAN
#elif defined( AR_USE_SGI )
  #define AR_ENDIAN_MODE AR_BIG_ENDIAN
#elif defined( AR_USE_DARWIN )
  #if defined( AR_USE_INTEL_MAC )
    #define AR_ENDIAN_MODE AR_LITTLE_ENDIAN
  #else
    #define AR_ENDIAN_MODE AR_BIG_ENDIAN
  #endif
#else
  #define AR_ENDIAN_MODE AR_UNDEFINED_ENDIAN
#endif

// The pipe read/write code works only on Unix.
// This code establishes a connection between szgd and
// the forked process in which the exe runs on the Unix side.
// As such, uses are found in szgd and arSZGClient.cpp
SZG_CALL bool ar_safePipeRead(int pipeID, char* theData, int numBytes);
SZG_CALL int ar_safePipeReadNonBlock(int pipeID, char* theData, int numBytes,
                                     int timeout);
SZG_CALL bool ar_safePipeWrite(int pipeID, const char* theData, int numBytes);

SZG_CALL void ar_packData(ARchar*, const void*, arDataType, int);
SZG_CALL void ar_unpackData(const ARchar*, void*, arDataType, int);
SZG_CALL ARint ar_rawDataGetSize(ARchar*);
SZG_CALL ARint ar_rawDataGetID(ARchar*);
SZG_CALL ARint ar_rawDataGetFields(ARchar*);

SZG_CALL arStreamConfig ar_getLocalStreamConfig();
SZG_CALL ARint ar_translateInt(ARchar*, arStreamConfig);
SZG_CALL ARint ar_fieldSize(arDataType, ARint);
SZG_CALL ARint ar_fieldOffset(arDataType, ARint);
SZG_CALL ARint ar_translateInt(ARchar*, ARint&, ARchar*, ARint&, arStreamConfig);
SZG_CALL void ar_translateField(ARchar*, ARint&, ARchar*, ARint&, arDataType,
                                ARint, arStreamConfig);

// type conversions with error checking
SZG_CALL bool ar_stringToLongValid( const string& theString, long& theLong );
SZG_CALL bool ar_longToIntValid( const long theLong, int& theInt );
SZG_CALL bool ar_stringToIntValid( const string& theString, int& theIntg );
SZG_CALL bool ar_stringToDoubleValid( const string& theString,
                                      double& theDouble );
SZG_CALL bool ar_doubleToFloatValid( const double theDouble,
                                     float& theFloat );
SZG_CALL bool ar_stringToFloatValid( const string& theString,
                                     float& theFloat );
SZG_CALL string ar_intToString(const int);

// string manipulation
SZG_CALL int ar_parseFloatString(const string& theString,
                                 float* outArray, int len);
SZG_CALL int ar_parseIntString(const string& theString,
                               int* outArray, int len);
SZG_CALL int ar_parseLongString(const string& theString,
                                long* outArray, int len);
SZG_CALL void ar_stringToBuffer(const string& theString,
                                char* theBuffer, int len);
SZG_CALL int ar_stringToInt(const string& theString);
// parse semicolon-delimited paths
SZG_CALL string ar_pathToken(const string& theString, int& start);
// Deal with the idiosyncracies of paths across Unix and Win32
SZG_CALL char ar_pathDelimiter();
SZG_CALL char ar_oppositePathDelimiter();
SZG_CALL string& ar_fixPathDelimiter(string& s);
SZG_CALL string& ar_scrubPath(string& s);
SZG_CALL string& ar_pathAddSlash(string& s); // append a / or a \ if necessary

// for slash-delimited string lists, "string1/string2/.../stringn"
class SZG_CALL arDelimitedString : public string {
public:
  arDelimitedString(const char delim = '/') : string(), _delimiter(delim) {}
  arDelimitedString(const char* sz, const char delim) : string(sz), _delimiter(delim) {}
  arDelimitedString(const string& s, const char delim) : string(s), _delimiter(delim) {}
  // Return number of substrings.
  int size() const;
  bool empty() const { return size() == 0; }
  // Return i'th substring, NOT i'th char.
  string operator[](int i) const;
  void appendDelimiter();
  // Append another substring, inserting _delimiter if needed.  (Bad pun.)
  arDelimitedString& operator/=(const string&);
private:
  char _delimiter;
};

class SZG_CALL arSlashString : public arDelimitedString {
public:
  arSlashString() : arDelimitedString('/') {}
  arSlashString(const char* sz) : arDelimitedString(sz, '/') {}
  arSlashString(const string& s) : arDelimitedString(s, '/') {}
  // Return number of substrings.
  arSlashString& operator/=(const string&);
};

// for semicolon-delimited string lists, "string1;string2;...;stringn"
class SZG_CALL arSemicolonString : public arDelimitedString {
public:
  arSemicolonString() : arDelimitedString(';') {}
  arSemicolonString(const char* sz) : arDelimitedString(sz, ';') {}
  arSemicolonString(const string& s) : arDelimitedString(s, ';') {}
  // Append another substring, inserting ';' if needed.
  arSemicolonString& operator/=(const string&);
};

class SZG_CALL arPathString : public arDelimitedString {
public:
  arPathString() : arDelimitedString(ar_pathDelimiter()) {}
  arPathString(const char* sz) : arDelimitedString(sz, ar_pathDelimiter()) {}
  arPathString(const string& s) : arDelimitedString(s, ar_pathDelimiter()) {}
  // Append another substring, inserting appropriate delimiter if needed.
  arPathString& operator/=(const string&);
};

// convert between vector<> of strings and '\0'-delimited char buffer.
SZG_CALL char* ar_packStringVector( std::vector< std::string >& stringVec,
                                    unsigned int& totalSize );
SZG_CALL void ar_unpackStringVector( char* inbuf, unsigned int numStrings,
                                     std::vector< std::string >& stringVec );

SZG_CALL void ar_vectorToArgcArgv( std::vector< std::string >& stringVec,
                                    int& argc, char**& argv );
void ar_cleanupArgcArgv( int& argc, char**& argv );
SZG_CALL void ar_argcArgvToVector( int argc, char** argv, std::vector< std::string >& stringVec );
SZG_CALL bool ar_getTokenList( const std::string& inString,
                       std::vector<std::string>& outList,
                       const char delim = ' ' );
// pack a string with the parameters
SZG_CALL string ar_packParameters(int, char**);

// strip pathname and .EXE from win32 exe's
SZG_CALL string ar_stripExeName(const string&);
// return path of current executable
SZG_CALL string ar_currentExePath();
// get the path only from a fully qualified executable name.
SZG_CALL string ar_exePath(const string&);
// find the extension of a particular file name (i.e. jpg or ppm or obj)
SZG_CALL string ar_getExtension(const string&);
// append .dll or .so
SZG_CALL void ar_addSharedLibExtension(string& name);
// replace every "from" with "to"
SZG_CALL string ar_replaceAll(const string& s, const string& from, const string& to);

// manipulating system characteristics
SZG_CALL string ar_getenv(const string&);
SZG_CALL bool ar_getSzgEnv( map< string,string,less<string> >& envMap );
SZG_CALL void   ar_setenv(const string&, const string&);
SZG_CALL void   ar_setenv(const string& variableName, int variableValue);
SZG_CALL string ar_getUser();

// file access
SZG_CALL bool ar_fileExists( const string name, bool& exists, bool& isFile );
SZG_CALL bool ar_directoryExists( const string name, bool& exists,
                                  bool& isDirectory );
SZG_CALL bool ar_fileItemExists( const string name, bool& exists );
SZG_CALL bool ar_getWorkingDirectory( string& name );
SZG_CALL bool ar_setWorkingDirectory( const string name );
SZG_CALL bool ar_isFile(const char* name);
SZG_CALL bool ar_isDirectory(const char* name);
SZG_CALL FILE* ar_fileOpen(const string& name,
                           const string& subdirectory,
                           const string& path,
                           const string& operation,
                           const char* warner = NULL);
SZG_CALL FILE* ar_fileOpen(const string& name,
                           const string& path,
                           const string& operation);

// Looks for the named file in the given subdirectory of the path
SZG_CALL string ar_fileFind(const string& name,
                            const string& subdirectory,
                            const string& path);
// Looks for the named directory in the given subdirectory of the path
SZG_CALL string ar_directoryFind(const string& name,
                                 const string& subdirectory,
                                 const string& path);
SZG_CALL list<string> ar_listDirectory(const string& name);
SZG_CALL int ar_fileClose(FILE* pf);

// ARchar buffer management
SZG_CALL bool ar_growBuffer(ARchar*& buf, int& size, int newSize);

// wrapper for usleep
SZG_CALL void ar_usleep(int);

// ar_usleep a little longer each time, from min to max.
// Stops clients from DDOS'ing a server.  "Exponential Backoff."
class SZG_CALL arSleepBackoff {
  float _u;        // current naptime
  float _uMin;  // initial short nap
  float _uMax;  // final long nap
  float _ratio; // ratio between successive naptimes
  float _uElapsed; // total time slept
 public:
  arSleepBackoff(const float msecMin, const float msecMax, const float ratio);
  void sleep();
  void reset();
  void resetElapsed();
  float msecElapsed() const { return _uElapsed / 1000.; }
};

SZG_CALL void* ar_allocateBuffer( arDataType theType, unsigned int size );
SZG_CALL void ar_deallocateBuffer( void* ptr );
// note order below is to match memcpy, which this is an extension of
SZG_CALL void ar_copyBuffer( void* const outBuf, const void* const inBuf,
                             arDataType theType, unsigned int size );

// Global database node manipulation.
class arDatabaseNode;
SZG_CALL void ar_refNodeList(list<arDatabaseNode*>& nodeList);
SZG_CALL void ar_unrefNodeList(list<arDatabaseNode*>& nodeList);

#define CALL_MEMBER_FUNCTION( object, ptrToMember ) ((object).*(ptrToMember))

#endif
