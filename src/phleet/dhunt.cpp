//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc < 1 || argc > 2){
    cerr << "usage: " << argv[0]
         << " [subnet]\n\t(default is 255.255.255)\n";
    return 1;
  }
  arPhleetConfigParser parser;
  if (!parser.parseConfigFile()) {
    cerr << "dhunt error: failed to parse phleet config file.\n";
    return 1;
  }

  /// \todo instead, parser.networkFirst(), networkNext() returning subnet strings.

  // we send out a broadcast packet on each interface in turn
  arSZGClient client;
  const arSlashString networkList(parser.getNetworks());
  const int numNetworks = networkList.size();
  for (int i=0; i<numNetworks; i++){
    const string address(parser.getAddresses(networkList[i]));
    // convert the address into a subnet
    const unsigned int position = address.find_last_of('.');
    if (position == string::npos){
      cerr << "dhunt warning: received malformed address from parser.\n"
           << "(If dconfig's addresses are wrong, correct them with daddinterface.)\n";
      continue;
    }
    const string subnet(address.substr(0, position+1));
    cout << "Searching subnet " << subnet << "\n";
    client.printSZGServers(subnet);
  }
  return 0;
}
