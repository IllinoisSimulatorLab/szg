//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

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
