// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

// The arAppLauncher is used to interact with the Syzygy "virtual computer"
// that the application is running on, to retrieve configuration information.
// Not intended for everyday use.
//



%{
#include "arAppLauncher.h"
%}

class arAppLauncher {
  public:
    arAppLauncher(const char* exeName = NULL);
    ~arAppLauncher();

    bool setVircomp();
    bool setVircomp(const string& vircomp);     

    bool connectSZG(int&, char** argv);
    bool setSZGClient(arSZGClient*);

    bool setParameters();

    int getNumberScreens();
    string getMasterName();
    string getTriggerName();
    string getScreenName(int num);
%extend{
    string getRenderProgram( int num ) {
      string computerName;
      string renderName;
      if (!self->getRenderProgram( num, computerName, renderName )) {
        return "NULL";
      } else {
        return computerName + "/" + renderName;
      }
    }
}
};

