//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
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
    cerr << "dlogin error: failed to parse phleet config file.\n";
    return 1;
  }
  arSZGClient client;
  string userName;
  bool success = false;
  if (argc == 3){
    userName = string(argv[2]);
    // broadcast on the network to find the szgserver

    /// \todo instead, parser.networkFirst(), networkNext() returning subnet strings.

    // we send out a broadcast packet on each interface in turn
    const arSlashString networkList(parser.getNetworks());
    const int numNetworks = networkList.size();
    for (int i=0; i<numNetworks; ++i){
      const string address(parser.getAddresses(networkList[i]));
      // convert the address into a subnet
      const unsigned int position = address.find_last_of('.');
      if (position == string::npos){
        cerr << "dlogin warning: parser returned malformed address.\n"
             << "(If dconfig's addresses are wrong, correct them with daddinterface.)\n";
        continue;
      }
      const string subnet(address.substr(0, position+1));
      if (client.discoverSZGServer(argv[1], "*", subnet)){
        // found something on this subnet, stop looking
        success = true;
        break;
      }
    }
  }
  else{
    // connect explicitly
    // BUG: the szgserver name is not set! though this doesn't affect it
    success = true; // we assume success
    const int port = atoi(argv[2]);
    client.setServerLocation(argv[1], port);
    userName = string(argv[3]);
  }

  if (!success){
    cerr << "dlogin error: failed to find named szgserver.\n";
    return 1;
  }

  // write the *partial* login file, since this is what the 
  // arSZGClient will use to connect. We'll write the login file again
  // after connecting to the szgserver (when the server name will have
  // transfered over)
  if (!client.writeLoginFile(userName)) {
    cout << "dlogin error: failed to write login file.\n";
    return 1;
  }
  
  // make sure we can really connect to the szgserver
  client.init(argc, argv);
  if (!client) {
    cerr << "dlogin error: failed to login to the szgserver.\n";
    client.logout();
    return 1;
  }

  // Write the login info to disk
  if (!client.writeLoginFile(userName))
    cout << "dlogin error: failed to write login file.\n";
  
  // verfiy we can, in fact, read the file.
  parser.parseLoginFile();
  parser.printLogin();
  return 0;
}
