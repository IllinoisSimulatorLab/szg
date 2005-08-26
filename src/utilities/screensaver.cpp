//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
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
