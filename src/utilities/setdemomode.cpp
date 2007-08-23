//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"

#include <string>

//Send a message to the master/controller to turn on demo mode immediately,
//then change the value of SZG_HEAD/fixed_head_mode on the
//master and trigger computers so that the change persists.

int main(int argc, char** argv) {
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arAppLauncher launcher("setdemomode");
  launcher.setSZGClient(&szgClient);
  if (argc == 3){
    // Explicitly set the virtual computer.
    if (!launcher.setVircomp(argv[1])) {
      ar_log_error() << "setdemomode: failed to set virtual computer.\n";
      return 1;
    }
    ar_log_debug() << "setdemomode: set virtual computer to " << argv[1] << ar_endl;
  } else {
    // Try to get the virtual computer from the "context".
    launcher.setVircomp();
    ar_log_debug() << "setdemomode: virtual computer is " << launcher.getVircomp() << ar_endl;
  }
  if (!launcher.setParameters()) {
    ar_log_error() << "setdemomode: arAppLauncher::setParameters() failed.\n";
    return 1;
  }
  ar_log_debug() << "setdemomode: arAppLauncher::setParameters() succeeded.\n";

  if (argc != 2 && argc != 3) {
Usage:    
    cerr << "usage: setdemomode [virtual_computer] true|false\n";
    return 1;
  }

  const string paramVal(argv[argc==3 ? 2 : 1]);
  if (paramVal != "true" && paramVal != "false")
    goto Usage; 

  // send message to trigger (which will relay to master if necessary).
  // copypaste from dmsg.cpp
  const string lockName = launcher.getLocation()+"/SZG_DEMO/app";
  int componentID;
  if (szgClient.getLock(lockName, componentID)) {
    // nobody else was holding the lock
    szgClient.releaseLock(lockName);
    ar_log_error() << "setdemomode error: no trigger component running.\n";
    ar_log_debug() << lockName << " lock-holder component ID = " << componentID << ar_endl;
    return 1;
  }
  const string messageBody = (paramVal == "true")? "on" : "off";
  const int match = szgClient.sendMessage( "demo", messageBody, componentID, false);
  if ( match < 0 ){
    // no need to print something here... sendMessage already does.
    return 1;
  }
  // end of copypaste from dmsg.cpp

  // set SZG_HEAD/fixed_head_mode on trigger & master.
  string triggerName = launcher.getTriggerName();
  if (triggerName == "NULL") {
    cerr << "setdemomode error: no trigger map defined.\n";
    return 1;
  }
  if (!szgClient.setAttribute( triggerName, "SZG_HEAD", "fixed_head_mode", paramVal )) {
    cerr << "setdemomode error: setAttribute failed.\n";
    return 1;
  }
  arSlashString masterMap = launcher.getMasterName();
  if (masterMap == "NULL") {
    cerr << "setdemomode error: no master map defined.\n";
    return 1;
  }
  if (masterMap.size() != 2) {
    cerr << "setdemomode error: badly-formed master map (" << masterMap << ").\n";
    return 1;
  }
  string masterName = masterMap[0];
  if (!szgClient.setAttribute( masterName, "SZG_HEAD", "fixed_head_mode", paramVal )) {
    cerr << "setdemomode error: setAttribute failed.\n";
    return 1;
  }
  
  return 0; 
}
