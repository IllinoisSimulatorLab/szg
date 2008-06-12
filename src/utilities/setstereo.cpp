//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"

// Enable/disable stereo on, and send reload message to, all render clients.

int main(int argc, char** argv) {
  // copypaste setdemomode.cpp
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc != 2 && argc != 3) {
Usage:
    ar_log_critical() << "usage: setstereo [virtual_computer] true|false\n";
    return 1;
  }

  arAppLauncher launcher("setstereo", &szgClient);
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

  launcher.updateRenderers("stereo", paramVal);
  return 0;
}
