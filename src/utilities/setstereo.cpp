//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"
#include <string>

// Enable/disable stereo, and send reload message to, all render clients.

int main(int argc, char** argv) {
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "setstereo error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("setstereo");
  launcher.setSZGClient(&szgClient);
  if (argc == 3){
    launcher.setVircomp(argv[1]);
  }
  else{
    launcher.setVircomp();
  }

  if (argc != 2 && argc != 3) {
Usage:    
    cerr << "usage: setstereo [virtual_computer] true|false\n";
    return 1;
  }

  const string paramVal(argc == 3 ? argv[2] : argv[1]);
  if (paramVal != "true" && paramVal != "false")
    goto Usage; 

  launcher.updateRenderers("stereo", paramVal);
  return 0; 
}
