//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc > 2){
    cerr << "usage: screensaver [virtual_computer]\n";
    return 1;
  }

  arAppLauncher launcher("screensaver");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }

  return launcher.screenSaver() ? 0 : 1;
}
