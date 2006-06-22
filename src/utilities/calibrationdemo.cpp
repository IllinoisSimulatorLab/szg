//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  ar_log().setStream(szgClient.initResponse());

  if (argc != 1 && argc != 2){
    ar_log_error() << "calibrationdemo usage: calibrationdemo [virtual_computer]\n";
    szgClient.sendInitResponse(false);
    return 1;
  }
  szgClient.sendInitResponse(true);
  ar_log().setStream(szgClient.startResponse());

  arAppLauncher launcher("calibrationdemo");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }
  launcher.setRenderProgram("PictureViewer cubecal.ppm");
  launcher.setAppType("distapp");
  if (!launcher.launchApp()){
    szgClient.sendStartResponse(false);
    return 1;
  }

  szgClient.sendStartResponse(true);
  ar_log().setStream(cout);
  launcher.waitForKill();
  return 0;
}
