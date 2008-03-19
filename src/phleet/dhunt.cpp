//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc > 2){
LUsage:
    cerr << "usage: " << argv[0] << " [-v]\n";
    return 1;
  }

  const bool fVerbose = argc == 2 && !strcmp(argv[1], "-v");
  if (argc == 2 && !fVerbose)
    goto LUsage;

  arPhleetConfig config;
  if (!config.read()) {
    cerr << "dhunt error: failed to parse config file.\n";
    return 1;
  }

  arSZGClient szgClient;
  // Launch explicitly, not implicitly via discoverSZGServer(),
  // so szgClient can display diagnostics.
  if (!szgClient.launchDiscoveryThreads())
    return 1;

  // todo: spawn threads to broadcast on all subnets (interfaces) at once
  const int numNetworks = config.getNumNetworks();
  for (int i=0; i<numNetworks; i++){
    const string broadcast = config.getBroadcast(i);
    if (broadcast == "NULL")
      continue;

    if (fVerbose)
      cout << "Hunting on network " << broadcast << ".\n";
    vector< std::string > serverVec = szgClient.findSZGServers(broadcast);
    for (std::vector< std::string >::const_iterator iter = serverVec.begin();
         iter != serverVec.end(); ++iter) {
      cout << *iter << endl;
    }
  }
  return 0;

  // If no servers are found, open firewall udp port 4620.
}
