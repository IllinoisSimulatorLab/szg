//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"
#include <string>

//Send a message to the master/controller to turn on demo mode immediately,
//then change the value of SZG_HEAD/fixed_head_mode on the
//master and trigger computers so that the change persists.

int main(int argc, char** argv) {
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "setdemomode error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("setdemomode");
  launcher.setSZGClient(&szgClient);
  if (argc == 3){
    // We must explicitly set the virtual computer.
    launcher.setVircomp(argv[1]);
  }
  else{
    // We will try to get the virtual computer from the "context".
    launcher.setVircomp();
  }

  if (argc != 2 && argc != 3) {
Usage:    
    cerr << "usage: setdemomode [virtual_computer] true|false\n";
    return 1;
  }

  const string paramVal(argc == 3 ? argv[2] : argv[1]);
  if (paramVal != "true" && paramVal != "false")
    goto Usage; 

  // send message to trigger (which will relay to master if necessary).
  // copied from dmsg.cpp
  int    componentID;
  string lockName = launcher.getVircomp()+"/SZG_DEMO/app";
  if (szgClient.getLock(lockName, componentID)){
    // nobody else was holding the lock
    szgClient.releaseLock(lockName);
    cerr << "setdemomode error: no trigger component running.\n";
    return 1;
  }
  string messageBody = (paramVal == "true")?("on"):("off");
  int match = szgClient.sendMessage( "demo", messageBody, 
                                    componentID, false);
  if ( match < 0 ){
    // no need to print something here... sendMessage already does.
    return 1;
  }
  // end dmsg block

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
