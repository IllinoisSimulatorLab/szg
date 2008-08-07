// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).


%{
#include "arDataUtilities.h"
%}

string ar_versionString();

%{
#include "arSocketAddress.h"
%}

class arSocketAddress {
 public:
  arSocketAddress();
  ~arSocketAddress() {}
  bool setAddress(const char* IPaddress, int port);
  void setPort(int port);
  string mask(const char* maskAddress);
  string broadcastAddress(const char* maskAddress);
  string getRepresentation();
};

%{
#include "arDataUtilities.h"
%}

class arTimer {
  public:
    arTimer( double dur = 0. );
    virtual ~arTimer() {}
    void start( double dur = 0. );
    bool running();
    // total time since construction or start() 
    // (deprecated, calls totalTime())
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
    void setRuntime( double dur ) { _runTime = dur; }
    double duration() { return _duration; }
};

bool ar_isFile(const char* name);
bool ar_isDirectory(const char* name);
string ar_fileFind(const string& name, 
                   const string& subdirectory,
                   const string& path);
string ar_directoryFind(const string& name, 
		        const string& subdirectory,
                        const string& path);

string ar_getUser();

/*bool ar_winSockInit();*/

