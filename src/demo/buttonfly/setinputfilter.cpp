//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arSZGClient.h"
#include <string>

int main(int argc, char** argv) {
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "setinputfilter error: failed to initialize SZGClient.\n";
    return 1;
  }

  arAppLauncher launcher("setinputfilter");
  launcher.setSZGClient(&szgClient);
  if (argc == 5){
    launcher.setVircomp(argv[1]);
  }

  if (argc != 4 && argc != 5) {
    cerr << "usage: setinputfilter [virtual_computer] driver_name "
	 << "group filter\n"
         << "  e.g. setinputfilter cube arCubeTracker SZG_INPUT "
	 << "arTrackCalFilter.\n";
    return 1;
  }

  const char* group  = argv[argc-2];
  const char* filter = argv[argc-1];

  string service(argv[2]);
  /// \todo copypaste from pathetic hack in demo/arAppLauncher.cpp
  if (service != "wandsimserver")
    service = "DeviceServer " + service;

  return launcher.updateService(service, group, "filter", filter) ? 0 : 1;
}
