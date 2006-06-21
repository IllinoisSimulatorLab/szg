//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "dkillall error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("dkillall");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }
  else if (argc > 2){
    cerr << "usage: dkillall [virtual_computer]\n";
    return 1;
  }

  return launcher.killAll() ? 0 : 1;
}
