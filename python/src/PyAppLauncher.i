// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

// The arAppLauncher is used to interact with the Syzygy "virtual computer"
// that the application is running on, to retrieve configuration information.
// Not intended for everyday use.
//
// Example: youre running a master/slave application and you need to send a
// message to the master from a slave. You do that with the arAppLauncher
// and the arSZGClient, as shown here (fw is an arMasterSlaveFramework).
//
//  cl = fw.getSZGClient()
//  al = fw.getAppLauncher()
//  mn = al.getMasterPipeNumber()
//  rp = al.getRenderProgram( mn )
//  if rp == 'NULL':
//    return
//  rpl = rp.split('/')
//  id = cl.getProcessID(rpl[0],rpl[1])
//  m = cl.sendMessage( 'user', 'connected', id, False )



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
    int getMasterPipeNumber();
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

