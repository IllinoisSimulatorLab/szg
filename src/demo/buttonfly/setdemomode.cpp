//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"
#include <string>

// Set demo mode for each rendering machine,
// and send reload message to any running render programs.
// TODO TODO TODO TODO TODO TODO TODO TODO
// THIS DOES NOT HANDSHAKE PROPERLY WITH SZGD!

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
    launcher.setVircomp(argv[1]);
  }

  if (argc != 2 && argc != 3) {
Usage:    
    cerr << "usage: setdemomode [virtual_computer] true|false\n";
    return 1;
  }

  const string paramVal(argc == 3 ? argv[2] : argv[1]);
  if (paramVal != "true" && paramVal != "false")
    goto Usage; 

  launcher.updateRenderers("fixed_head", paramVal);
  return 0; 
}
