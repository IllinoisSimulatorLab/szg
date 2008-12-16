//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"


void sendQuitMessage( const int progID, arSZGClient& szgClient ) {
  szgClient.sendMessage("quit", "0", progID);
  const int tag = szgClient.requestKillNotification(progID);
  const int msecTimeout = 5000;
  if (szgClient.getKillNotification(list<int>(1, tag), msecTimeout) < 0) {
    ar_log_error() << "timed out after " << msecTimeout << " msec.\n";
  }
}


bool killByID( const int id, const bool fForce, arSZGClient& szgClient ) {
  if (fForce) {
    if (szgClient.killProcessID( id ))
      return true;
    ar_log_critical() << "no process with id '" << id << ar_endl;
    return false;
  }
  sendQuitMessage( id, szgClient );
  return true;
}

bool killByName( string& hostName, const string& exeName, const bool fForce,
   arSZGClient& szgClient ) {
  const string hostLocal(szgClient.getComputerName());
  if (hostName != hostLocal &&
      szgClient.getAttribute(hostName, "SZG_CONF", "virtual", "") == "true") {
    // hostName is a virtual computer.
    const string trigger(szgClient.getAttribute(hostName, "SZG_TRIGGER", "map", ""));
    if (trigger == "NULL") {
      ar_log_critical() << "no SZG_TRIGGER/map for virtual computer '" <<
        hostName << "'.\n";
      return false;
    }
    hostName = trigger;
  }
  if (fForce) {
    if (szgClient.killProcessID(hostName, exeName))
      return true;
LNotFound:
    ar_log_critical() << "no process '" << exeName << "' on host '" << hostName << "'.\n";
    return false;
  }

  const int progID = szgClient.getProcessID(hostName, exeName);
  if (progID == -1) {
    goto LNotFound;
  }
  sendQuitMessage( progID, szgClient );
  return true;
}


int main(int argc, char** argv) {
  // todo: add -q "quiet" option
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient) {
    return szgClient.failStandalone(fInit);
  }

  int idNumArgs(2);
  int nameNumArgs(3);
  bool fForce(false);

  if (argc > 1) {
    fForce = !strcmp(argv[1], "-9");
    if (fForce) {
      ++idNumArgs;
      ++nameNumArgs;
    }
  }

  if (argc == idNumArgs) {
    int id;
    if (!ar_stringToIntValid( argv[idNumArgs-1], id )) {
      ar_log_error() << "conversion to int failed.\n";
      goto LUsage;
    }
    if (!killByID( id, fForce, szgClient )) {
      ar_log_error() << "killByID failed.\n";
      return 1;
    }
    return 0;
  } else if (argc == nameNumArgs) {
    string hostName(argv[nameNumArgs-2]);
    string progName(argv[nameNumArgs-1]);
    if (!killByName( hostName, progName, fForce, szgClient )) {
      ar_log_error() << "killByName failed.\n";
      return 1;
    }
    return 0;
  }

LUsage:
  ar_log_critical() << "usage: dkill [-9] hostname executable_label\n"
                    << "   or: dkill [-9] process_id\n";
  return 1;
}
