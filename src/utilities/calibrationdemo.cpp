//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "calibrationdemo error: failed to initialize SZGClient.\n";
    szgClient.sendInitResponse(false);
    return 1;
  }

  if (argc != 1 && argc != 2){
    cerr << "calibrationdemo usage: calibrationdemo [virtual_computer]\n";
    szgClient.sendInitResponse(false);
    return 1;
  }
  szgClient.sendInitResponse(true);

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
  launcher.waitForKill();
  return 0;
}
