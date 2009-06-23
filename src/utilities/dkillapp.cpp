//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"

// strip flags from the command line
// Use cout not cerr in main(), so we can build RPC scripts.

int main(int argc, char** argv) {

  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc < 2) {
    cout << "Usage: dkillapp <location>\n";
    return 1;
  }

  string messageType("quit");
  string messageBody;
  int componentID = -1;
  const string lockName = string(argv[1])+"/SZG_DEMO/app";
  if (szgClient.getLock(lockName, componentID)) {
    // nobody else was holding the lock
    szgClient.releaseLock(lockName);
    cout << "dmsg error: no trigger running in location '"
         << argv[1] << "'.\n";
    return 1;
  }

  // We know what to send, and to whom.
  return szgClient.sendMessage("quit", "", componentID) < 0 ? 1 : 0;
}
