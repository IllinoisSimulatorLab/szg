//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }

  arPhleetConfigParser parser;
  if (!parser.parseConfigFile()) {
    cerr << "dhunt error: failed to parse phleet config file.\n";
    return 1;
  }

  // we send out a broadcast packet on each interface in turn
  arSZGClient client;
  // The discovery threads should be launched by us now, as opposed to
  // implicitly by the discoverSZGServer function. Thus, if we get an error,
  // then we can bail out with a reasonable error message.
  if (!client.launchDiscoveryThreads()){
    // Upon error, a complaint has already been logged.
    return 1;
  }
  const arSlashString networkList(parser.getNetworks());
  const arSlashString addressList(parser.getAddresses());
  const arSlashString maskList(parser.getMasks());
  const int numNetworks = networkList.size();
  for (int i=0; i<numNetworks; i++){
    const string address = addressList[i];
    const string mask = maskList[i];
    arSocketAddress tmpAddress;
    if (!tmpAddress.setAddress(address.c_str(), 0)){
      cout << "dhunt remark: illegal address (" << address
	   << ") in szg.conf.\n";
      continue;
    }
    string broadcast = tmpAddress.broadcastAddress(mask.c_str());
    if (broadcast == "NULL"){
      cout << "dhunt remark: illegal mask ("
	   << mask << ") for address (" << address << ").\n";
      continue;
    }
    cout << "Broadcasting on " << broadcast << "\n";
    client.printSZGServers(broadcast);
  }
  return 0;
}
