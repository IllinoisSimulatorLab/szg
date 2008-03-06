//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"

// copypaste restarttracker.cpp

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc > 2){
    ar_log_error() << "usage: screensaver [virtual_computer]\n";
    return 1;
  }

  arAppLauncher launcher("screensaver", &szgClient);
  if (argc == 2) {
    launcher.setVircomp(argv[1]);
  }

  return launcher.screenSaver() ? 0 : 1;
}
