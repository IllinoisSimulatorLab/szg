//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  /// \todo add a -q "quiet" option
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient){
    return 1;
  }

  if (argc < 2 || argc > 4){
LUsage:
    cerr << "usage: " << argv[0] << " [-9] [hostname] executable_label\n";
    return 1;
  }

  string hostName;
  string exeName;
  string messageContext("NULL");
  bool fForce = false;

  switch (argc)
    {
  case 2:
    // executable
    hostName = szgClient.getComputerName();
    exeName = argv[1];
    break;
  case 3:
    if (!strcmp(argv[1], "-9"))
      {
      // -9 executable
      fForce = true;
      hostName = szgClient.getComputerName();
      exeName = argv[2];
      }
    else
      {
      // hostname executable
      hostName = argv[1];
      exeName = argv[2];
      }
    break;
  case 4:
    // -9 hostname executable
    if (strcmp(argv[1], "-9"))
      goto LUsage;
    fForce = true;
    hostName = argv[2];
    exeName = argv[3];
    break;
    }

  if (hostName != szgClient.getComputerName() &&
      szgClient.getAttribute(hostName,"SZG_CONF","virtual", "") == "true"){
    // hostName is a virtual computer.
    const string trigger(
      szgClient.getAttribute(hostName,"SZG_TRIGGER","map", ""));
    if (trigger == "NULL"){
      cerr << "dkill error: undefined SZG_TRIGGER/map on virtual computer \""
	   << hostName << "\".\n";
      return 1;
    }
    hostName = trigger;
  }

  if (fForce)
    {
    if (!szgClient.killProcessID(hostName, exeName)){
      cerr << "dkill -9: no process \"" << exeName
	   << "\" on host " << hostName << ".\n";
      return 1;
    }
    return 0;
    }

  const int progID = szgClient.getProcessID(hostName, exeName);
  if (progID == -1){
    cerr << "dkill: no process \"" << exeName
	 << "\" on host " << hostName << ".\n";
    return 1;
  }

  szgClient.sendMessage("quit", "0", progID);
  return 0;
}
