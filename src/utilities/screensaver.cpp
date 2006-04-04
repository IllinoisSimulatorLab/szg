//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv); // Parse "-szg" args first.
  if (!szgClient) {
    cerr << "screensaver error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("screensaver");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }
  if (argc > 2){
    cerr << "usage: screensaver [virtual_computer]\n";
    return 1;
  }

  return launcher.screenSaver() ? 0 : 1;
}
