//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv){
  // todo: add -q "quiet" option
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc < 2 || argc > 4){
LUsage:
    ar_log_critical() << "usage: " << argv[0] << " [-9] [hostname] executable_label\n";
    return 1;
  }

  const string hostLocal(szgClient.getComputerName());
  const string exeName(argv[argc-1]);
  const bool fForce = !strcmp(argv[1], "-9");
  string hostName;
  switch (argc)
    {
  case 2:
    if (fForce)
      goto LUsage;
    // executable
    hostName = hostLocal;
    break;
  case 3:
    // -9 executable, or hostname executable
    hostName = fForce ? hostLocal : argv[1];
    if (fForce && exeName == hostName)
      ar_log_error() << "(Did you forget to name the exe to kill?)\n";
    break;
  case 4:
    // -9 hostname executable
    if (!fForce)
      goto LUsage;
    hostName = argv[2];
    break;
    }

  if (hostName != hostLocal &&
      szgClient.getAttribute(hostName,"SZG_CONF","virtual", "") == "true"){
    // hostName is a virtual computer.
    const string trigger(szgClient.getAttribute(hostName,"SZG_TRIGGER","map", ""));
    if (trigger == "NULL"){
      ar_log_critical() << "no SZG_TRIGGER/map for virtual computer '" <<
	hostName << "'.\n";
      return 1;
    }
    hostName = trigger;
  }

  if (fForce)
    {
    if (szgClient.killProcessID(hostName, exeName))
      return 0;

LNotFound:
    ar_log_critical() << "no process '" << exeName << "' on host '" << hostName << "'.\n";
    return 1;
    }

  const int progID = szgClient.getProcessID(hostName, exeName);
  if (progID == -1){
    goto LNotFound;
  }

  szgClient.sendMessage("quit", "0", progID);
  const int tag = szgClient.requestKillNotification(progID);
  const int msecTimeout = 5000;
  if (szgClient.getKillNotification(list<int>(1,tag), msecTimeout) < 0){
    ar_log_error() << "timed out after " << msecTimeout << " msec.\n";
  }
  return 0;
}
