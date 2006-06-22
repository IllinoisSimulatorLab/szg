//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSZGClient.h"
#include "arPhleetConfigParser.h"
#include "arDataUtilities.h"
#include <iostream>

int main(int argc, char** argv){
  if (argc != 3 && argc != 4){
    cerr << "usage: " << argv[0] << " szgserver_name user_name\n"
	 << "       " << argv[0] << " szgserver_IP szgserver_port user_name\n";
    return 1;
  }
 
  arPhleetConfigParser parser;
  if (!parser.parseConfigFile()) {
    ar_log_error() << "dlogin failed to parse phleet config file.\n";
    return 1;
  }

  arSZGClient szgClient;
  // Launch explicitly, not implicitly via discoverSZGServer(),
  // so szgClient can display diagnostics.
  if (!szgClient.launchDiscoveryThreads())
    return 1;

  string userName;
  if (argc == 3){
    userName = string(argv[2]);
    const arSlashString networkList(parser.getNetworks());
    const arSlashString addressList(parser.getAddresses());
    const arSlashString maskList(parser.getMasks());
    const int numNetworks = networkList.size();
    bool found = false;

    // Send a broadcast packet on each interface, to find an szgserver.
    for (int i=0; i<numNetworks && !found; ++i){
      const string address = addressList[i];
      const string mask = maskList[i];
      // Compute the broadcast address for this network.
      arSocketAddress tmpAddress;
      if (!tmpAddress.setAddress(address.c_str(), 0)){
	cout << "dlogin remark: illegal address (" << address
	     << ") in szg.conf.\n";
	continue;
      }
      const string broadcast = tmpAddress.broadcastAddress(mask.c_str());
      if (broadcast == "NULL"){
	cout << "dlogin remark: illegal mask '"
	     << mask << "' for address '" << address << "'.\n";
	continue;
      }
      found = szgClient.discoverSZGServer(argv[1], broadcast);
    }

    if (!found){
      ar_log_error() << "dlogin found no szgserver named '" << argv[1] << "'.\n";
      return 1;
    }
  }
  else{
    // Connect explicitly.
    // BUG: the szgserver name is not set! though this doesn't affect it
    szgClient.setServerLocation(argv[1], atoi(argv[2]));
    userName = string(argv[3]);
  }

  // Write the partial login file,
  // which is what the arSZGClient will use to connect.
  if (!szgClient.writeLoginFile(userName))
    return 1;

  // Connect to the szgserver
  szgClient.init(argc, argv);
  if (!szgClient) {
    ar_log_error() << "dlogin failed to connect to szgserver.\n";
    szgClient.logout();
    return 1;
  }

  // Write the login file completely,
  // now that the server name has been transferred.
  if (!szgClient.writeLoginFile(userName))
    return 1;

  // Verify that the login file can be read.
  parser.parseLoginFile();
  parser.printLogin();
  return 0;
}
