//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
// TODO TODO TODO TODO TODO TODO TODO TODO
// THIS DOES NOT HANDSHAKE PROPERLY WITH SZGD!

int main(int argc, char** argv){
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "setstereo error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("restarttracker");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }

  if (argc != 1 && argc != 2){
    cerr << "usage: restarttracker [virtual_computer]\n";
    return 1;
  }

  return launcher.restartServices() ? 0 : 1;
}
