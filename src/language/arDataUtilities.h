//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_UTILITIES
#define AR_DATA_UTILITIES

#include "arDataType.h"
#include "arSocket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

#ifdef AR_USE_WIN_32
bool ar_winSockInit();
#endif

// small utility classes... for instance the STL does not have triples

/// Wrapper for 3 ints.
class arIntTriple{
 public:
  arIntTriple() { first=0; second=0; third=0; }
  arIntTriple(int x, int y, int z){ first=x; second=y; third=z; }
  ~arIntTriple() {}
  int first;
  int second;
  int third;
};

// cross-platform time-reporting structure, and appropriate headers

#ifdef AR_USE_WIN_32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

// functions for messing with time conveniently

/// Internal representation of time.

class ar_timeval {
 public:
  int sec;
  int usec;
  ar_timeval() : sec(0), usec(0) {}
  ar_timeval(int s, int u) : sec(s), usec(u) {}
};

ar_timeval ar_time();
double ar_difftime(const ar_timeval& endTime, const ar_timeval& startTime);
ar_timeval& ar_addtime(ar_timeval& result, const ar_timeval& addend); // returns "result"
string ar_currentTimeString();

class arTimer {
  public:
    arTimer( double dur = 0. );
    virtual ~arTimer() {}
    void start( double dur = 0. );
    bool running();
    // total time since construction or start() (deprecated, calls totalTime())
    double elapsed();
    // time since last restart() (or construction or start(), if none)
    double lapTime();
    // time running since construction or start()
    double runningTime();
    // total time since construction or start()
    double totalTime(); 
    bool done();
    void stop();
    void reset();
    double duration() { return _duration; }
  private:
    bool _firstStart;
    ar_timeval _startTime;
    ar_timeval _lastStart;
    double _duration;
    double _lapTime;
    double _runTime;
    bool _running;
};

// utilities for manipulating the records

/// Configuration of a data stream (only endianness at the moment).

struct arStreamConfig{
  ARchar endian;
};

#define AR_LITTLE_ENDIAN 0
#define AR_BIG_ENDIAN 1
#ifdef AR_USE_LINUX
  #define AR_ENDIAN_MODE AR_LITTLE_ENDIAN
#endif
#ifdef AR_USE_WIN_32
  #define AR_ENDIAN_MODE AR_LITTLE_ENDIAN
#endif
#ifdef AR_USE_SGI
  #define AR_ENDIAN_MODE AR_BIG_ENDIAN
#endif
#ifdef AR_USE_DARWIN
  #define AR_ENDIAN_MODE AR_BIG_ENDIAN
#endif

/// note that the pipe read/write code only works on Unix.
/// this code is used to establish a connection between szgd and 
/// the forked process in which the executable runs on the Unix side. 
/// As such, uses are found in szgd and arSZGClient.cpp
bool ar_safePipeRead(int pipeID, char* theData, int numBytes);
int ar_safePipeReadNonBlock(int pipeID, char* theData, int numBytes,
			    int timeout);
bool ar_safePipeWrite(int pipeID, const char* theData, int numBytes);

void ar_packData(ARchar*,const void*,arDataType,int);
void ar_unpackData(const ARchar*,void*,arDataType,int);
ARint ar_rawDataGetSize(ARchar*);
ARint ar_rawDataGetID(ARchar*);
ARint ar_rawDataGetFields(ARchar*);

arStreamConfig ar_getLocalStreamConfig();
ARint ar_translateInt(ARchar*,arStreamConfig);
ARint ar_fieldSize(arDataType,ARint);
ARint ar_fieldOffset(arDataType,ARint);
ARint ar_translateInt(ARchar*,ARint&,ARchar*,ARint&,arStreamConfig);
void ar_translateField(ARchar*,ARint&,ARchar*,ARint&,arDataType,ARint,
                       arStreamConfig);

/// type conversions with error checking
bool ar_stringToLongValid( const string& theString, long& theLong );
bool ar_longToIntValid( const long theLong, int& theInt );
bool ar_stringToIntValid( const string& theString, int& theIntg );
bool ar_stringToDoubleValid( const string& theString, double& theDouble );
bool ar_doubleToFloatValid( const double theDouble, float& theFloat );
bool ar_stringToFloatValid( const string& theString, float& theFloat );

/// string manipulation
int ar_parseFloatString(const string& theString, float* outArray, int len);
int ar_parseIntString(const string& theString, int* outArray, int len);
int ar_parseLongString(const string& theString, long* outArray, int len);
void ar_stringToBuffer(const string& theString, char* theBuffer, int len);
int ar_stringToInt(const string& theString);
/// parse semicolon-delimited paths
string ar_pathToken(const string& theString, int& start);
/// Deal with the idiosyncracies of paths across Unix and Win32
char ar_pathDelimiter();
char ar_oppositePathDelimiter();
string& ar_scrubPath(string& s);
string& ar_pathAddSlash(string& s); // append a / or a \ if necessary

/// for slash-delimited string lists, "string1/string2/.../stringn"
class arDelimitedString : public string {
public:
  arDelimitedString(const char delim = '/') : string(), _delimiter(delim) {}
  arDelimitedString(const char* sz, const char delim) : string(sz), _delimiter(delim) {}
  arDelimitedString(const string& s, const char delim) : string(s), _delimiter(delim) {}
  /// Return number of substrings.
  int size() const;
  bool empty() const { return size() == 0; }
  /// Return i'th substring, NOT i'th char.
  string operator[](int i) const;
  void appendDelimiter();
  /// Append another substring, inserting _delimiter if needed.  (Bad pun.)
  arDelimitedString& operator/=(const string&);
private:
  char _delimiter;  
};

class arSlashString : public arDelimitedString {
public:
  arSlashString() : arDelimitedString('/') {}
  arSlashString(const char* sz) : arDelimitedString(sz,'/') {}
  arSlashString(const string& s) : arDelimitedString(s,'/') {}
  /// Return number of substrings.
  arSlashString& operator/=(const string&);
};

/// for semicolon-delimited string lists, "string1;string2;...;stringn"
class arSemicolonString : public arDelimitedString {
public:
  arSemicolonString() : arDelimitedString(';') {}
  arSemicolonString(const char* sz) : arDelimitedString(sz,';') {}
  arSemicolonString(const string& s) : arDelimitedString(s,';') {}
  /// Append another substring, inserting ';' if needed.
  arSemicolonString& operator/=(const string&);
};

class arPathString : public arDelimitedString {
public:
  arPathString() : arDelimitedString(ar_pathDelimiter()) {}
  arPathString(const char* sz) : arDelimitedString(sz,ar_pathDelimiter()) {}
  arPathString(const string& s) : arDelimitedString(s,ar_pathDelimiter()) {}
  /// Append another substring, inserting appropriate delimiter if needed.
  arPathString& operator/=(const string&);
};


bool ar_getTokenList( const std::string& inString,
                       std::vector<std::string>& outList,
                       const char delim = ' ' );
/// pack a string with the parameters
string ar_packParameters(int,char**);

/// strip pathname and .EXE from win32 exe's
string ar_stripExeName(const string&);
/// find the extension of a particular file name (i.e. jpg or ppm or obj)
string ar_getExtension(const string&);
/// add the right shared library extension for the system (.dll or .so)
void ar_addSharedLibExtension(string& name);

/// manipulating system characteristics
string ar_getenv(const string&);
void   ar_setenv(const string&, const string&);
void   ar_setenv(const string& variableName, int variableValue);
string ar_getUser();

/// file access
bool ar_fileExists( const string name, bool& exists, bool& isFile );
bool ar_directoryExists( const string name, bool& exists, bool& isDirectory );
bool ar_fileItemExists( const string name, bool& exists );
bool ar_getWorkingDirectory( string& name );
bool ar_setWorkingDirectory( const string name );
bool ar_isFile(const char* name);
bool ar_isDirectory(const char* name);
FILE* ar_fileOpen(const string& name, 
                  const string& subdirectory,
                  const string& path,
                  const string& operation);
FILE* ar_fileOpen(const string& name, 
                  const string& path,
                  const string& operation);
/*
ifstream ar_istreamOpen(const string& name, 
                  const string& subdirectory,
                  const string& path,
                  int mode );
ifstream ar_istreamOpen(const string& name, 
                  const string& path,
                  const string& operation);
                  */
/// Looks for the named file in the given subdirectory of the path
string ar_fileFind(const string& name, 
		   const string& subdirectory,
		   const string& path);
int ar_fileClose(FILE* pf);

/// ARchar buffer management
bool ar_growBuffer(ARchar*& buf, int& size, int newSize);

/// wrapper for usleep (compatibility with Win32)
void ar_usleep(int);

void* ar_allocateBuffer( arDataType theType, unsigned int size );
void ar_deallocateBuffer( void* ptr );
// note order below is to match memcpy, which this is an extension of
void ar_copyBuffer( void* const outBuf, const void* const inBuf,
    arDataType theType, unsigned int size );

#define CALL_MEMBER_FUNCTION( object, ptrToMember ) ((object).*(ptrToMember))

#endif
