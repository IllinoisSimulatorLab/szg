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
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "calibrationdemo error: failed to initialize SZGClient.\n";
    return 1;
  }

  if (argc != 1 && argc != 2){
    cerr << "calibrationdemo usage: calibrationdemo [virtual_computer]\n";
    return 1;
  }

  arAppLauncher launcher("calibrationdemo");
  launcher.setSZGClient(&szgClient);
  if (argc == 2){
    launcher.setVircomp(argv[1]);
  }

  return (
    launcher.setRenderProgram("PictureViewer cubecal.ppm") &&
    launcher.setAppType("distapp") &&
    launcher.launchApp() &&
    launcher.waitForKill()) ?
    0 : 1;
}
