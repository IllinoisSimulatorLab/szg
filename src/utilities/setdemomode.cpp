//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"

// Tell the master/controller to turn on demo mode immediately.
// Then modify SZG_HEAD/fixed_head_mode on master and trigger,
// so the change persists.

int main(int argc, char** argv) {
  // copypaste setstereo.cpp
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc != 2 && argc != 3) {
Usage:
    ar_log_critical() << "usage: setdemomode [virtual_computer] true|false\n";
    return 1;
  }

  arAppLauncher launcher("setdemomode", &szgClient);
  if (argc == 3) {
    if (!launcher.setVircomp(argv[1])) {
      return 1;
    }
  } else {
    if (!launcher.setVircomp()) {
      return 1;
    }
  }

  const string paramVal(argv[argc-1]);
  if (paramVal != "true" && paramVal != "false")
    goto Usage;
  // end copypaste

  if (!launcher.setParameters()) {
    return 1;
  }

  // First try to send message to master, then to trigger if that fails.
  string lockName = launcher.getMasterName();
  int componentID = -1;
  if (szgClient.getLock(lockName, componentID)) {
    // nobody was holding the lock
    szgClient.releaseLock(lockName);
    ar_log_critical() << "setdemomode: no master component running.\n";

  // Try to send message to trigger.
    lockName = launcher.getLocation() + "/SZG_DEMO/app";
    if (szgClient.getLock(lockName, componentID)) {
      // Nobody held the lock.
      szgClient.releaseLock(lockName);
      ar_log_critical() << "setdemomode: no trigger component running.\n";
      ar_log_debug() << lockName << " lock-holder component ID = " << componentID << ar_endl;
      componentID = -1;
    }
  }
  if (componentID != -1) {
    const int match = szgClient.sendMessage(
      "fixedhead", (paramVal == "true")? "on" : "off", componentID, false);
    if ( match < 0 ) {
      // sendMessage() already complained.
      return 1;
    }
  }

  // set SZG_HEAD/fixed_head_mode on trigger & master.
  string triggerName = launcher.getTriggerName();
  ar_log_debug() << "Trigger = " << triggerName << ar_endl;
  if (triggerName == "NULL") {
    ar_log_critical() << "no trigger map.\n";
    return 1;
  }
  if (!szgClient.setAttribute( triggerName, "SZG_HEAD", "fixed_head_mode", paramVal )) {
    ar_log_critical() << "setAttribute failed.\n";
    return 1;
  }
  arSlashString masterMap(launcher.getMasterName());
  ar_log_debug() << "Master = " << masterMap << ar_endl;
  if (masterMap == "NULL") {
    ar_log_critical() << "no master map.\n";
    return 1;
  }
  if (masterMap.size() != 2) {
    ar_log_critical() << "expected 2 elements in master map '" << masterMap << "'.\n";
    return 1;
  }
  if (!szgClient.setAttribute( masterMap[0], "SZG_HEAD", "fixed_head_mode", paramVal )) {
    ar_log_critical() << "setAttribute failed.\n";
    return 1;
  }

  return 0;
}
