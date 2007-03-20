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

  if (argc > 2) {
    ar_log_error() << "usage: dkillall [virtual_computer]\n";
    (void)szgClient.sendInitResponse(false);
    return 1;
  }

  arAppLauncher launcher("dkillall");
  if (argc == 2) {
    launcher.setSZGClient(&szgClient);
    if (!launcher.setVircomp(argv[1])) {
      // arAppLauncher already printed msg.
      return 1;
    }
  }

  return launcher.killAll() ? 0 : 1;
}
