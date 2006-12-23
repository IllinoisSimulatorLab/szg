//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

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
  arSZGClient szgClient;
  // Launch the discovery threads here, as opposed to
  // implicitly by discoverSZGServer(), so that on error
  // we can abort with a reasonable error message.
  if (!szgClient.launchDiscoveryThreads()){
    // Already warned.
    return 1;
  }

  const arSlashString networkList(parser.getNetworks());
  const arSlashString addressList(parser.getAddresses());
  const arSlashString maskList(parser.getMasks());
  const int numNetworks = networkList.size();
  //;;;; Could we spawn threads to broadcast on all subnets at once?
  for (int i=0; i<numNetworks; i++){
    const string address(addressList[i]);
    const string mask(maskList[i]);
    arSocketAddress tmpAddress;
    if (!tmpAddress.setAddress(address.c_str(), 0)){
      cout << "dhunt remark: illegal address (" << address
	   << ") in szg.conf.\n";
      continue;
    }
    const string broadcast(tmpAddress.broadcastAddress(mask.c_str()));
    if (broadcast == "NULL"){
      cout << "dhunt remark: illegal mask ("
	   << mask << ") for address (" << address << ").\n";
      continue;
    }
    //;;;; only say Broadcasting on, if -v flag
    cout << "Hunting via network interface " << broadcast << "\n";
    std::vector< std::string > serverVec = szgClient.findSZGServers(broadcast);
    std::vector< std::string >::const_iterator iter;
    for (iter = serverVec.begin(); iter != serverVec.end(); ++iter) {
      cout << *iter << endl;
    }
    serverVec.clear();
  }
  return 0;
}
