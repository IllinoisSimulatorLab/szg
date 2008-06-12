//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv) {
  if (argc != 3 && argc != 4) {
    cerr << "usage: " << argv[0] << " szgserver_name user_name\n"
	 << "       " << argv[0] << " szgserver_IP szgserver_port user_name\n";
    return 1;
  }

  arPhleetConfig config;
  if (!config.read()) {
    cerr << "dlogin error: failed to parse config file.\n";
    return 1;
  }

  arSZGClient szgClient;
  // Launch explicitly, not implicitly via discoverSZGServer(),
  // so szgClient can display diagnostics.
  if (!szgClient.launchDiscoveryThreads())
    return 1;

  string userName;
  if (argc == 3) {
    userName = string(argv[2]);

    // Send a broadcast packet on each interface, to find an szgserver.
    bool found = false;
    const int numNetworks = config.getNumNetworks();
    for (int i=0; i<numNetworks && !found; ++i) {
      const string broadcast = config.getBroadcast(i);
      found |= (broadcast != "NULL") && szgClient.discoverSZGServer(argv[1], broadcast);
    }

    if (!found) {
      ar_log_critical() << "dlogin found no szgserver named '" << argv[1] << "'.\n";
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
  if (!szgClient.writeLogin(userName))
    return 1;

  // Connect to the szgserver
  szgClient.init(argc, argv);
  if (!szgClient) {
    ar_log_critical() << "dlogin failed to connect to szgserver.\n";
    szgClient.logout();
    return 1;
  }

  // Write the login file completely,
  // now that the server name has been transferred.
  if (!szgClient.writeLogin(userName))
    return 1;

  // Verify that the login file can be read.
  return config.readLogin() && config.printLogin() ? 0 : 1;
}
