//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"

// strip flags from the command line
void striparg(int which, int& argc, char** argv) {
  for (int i=which; i<argc-1; i++) {
    argv[i] = argv[i+1];
  }
  --argc;
}

string joinArgs( int argc, char **argv, int startIndex ) {
  ostringstream os;
  for (int i=startIndex; i<argc; ++i) {
    if (i>startIndex) {
      os << " ";
    }
    os << argv[i];
  }
  return os.str();
}

// Use cout not cerr in main(), so we can build RPC scripts.

int main(int argc, char** argv) {

  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc < 2) {
LPrintUsage:
  cerr << "Usage:\n"
       << "  Send to a Phleet component by ID:\n"
       << "    dmsg [-r] component_ID message_type [message_body]\n"
       << "  Send to a Phleet component by name:\n"
       << "    dmsg [-r] -p computer_name component_name message_type [message_body]\n"
       << "  Send to the master component on a virtual computer:\n"
       << "    dmsg [-r] -m virtual_computer message_type [message_body]\n"
       << "  Send to the process holding the screen resource:\n"
       << "    dmsg [-r] -g virtual_computer screen_number message_type "
       << "[message_body]\n"
       << "  Send to the trigger process of a virtual computer:\n"
       << "    dmsg [-r] -c location message_type [message_body]\n"
       << "  Send to the component hosting a service:\n"
       << "    dmsg [-r] -s service_name message_type [message_body]\n"
       << "  Send to the component holding a lock:\n"
       << "    dmsg [-r] -l lock_name message_type [message_body]\n\n";
    return 1;
  }

  // For dmsg to receive a response, the first flag must be -r.
  bool responseExpected = false;
  if (!strcmp(argv[1], "-r")) {
    responseExpected = true;
    striparg(1, argc, argv);
  }

  // Modes:
  // default: send a message to the component with that ID.
  // -p: send to the component matching the computer/component_name Pair.
  // -m: send to the component operating on the Master screen.
  // -g: send to the component operating on a Display.
  // -c: send to the Control component (holding the "demo" lock).
  // -l: send to the component holding the named lock.
  // -s: send to the component providing the named service.

  // Parse the args.
  enum { modeDefault, modeProcess, modeMaster, modeDisplay, modeControl, modeService, modeLock };
  int mode = modeDefault;
  if (!strcmp(argv[1], "-p")) {
    mode = modeProcess;
    striparg(1, argc, argv);
    // i-- counteracts the end-of-loop increment
  }
  else if (!strcmp(argv[1], "-m")) {
    mode = modeMaster;
    striparg(1, argc, argv);
  }
  else if (!strcmp(argv[1], "-g")) {
    mode = modeDisplay;
    striparg(1, argc, argv);
  }
  else if (!strcmp(argv[1], "-c")) {
    mode = modeControl;
    striparg(1, argc, argv);
  }
  else if (!strcmp(argv[1], "-s")) {
    mode = modeService;
    striparg(1, argc, argv);
  }
  else if (!strcmp(argv[1], "-l")) {
    mode = modeLock;
    striparg(1, argc, argv);
  }

  string messageType;
  string messageBody;
  int typeIndex;
  int componentID = -1;

  switch (mode) {
  default:
    goto LPrintUsage;

  case modeDefault:
    typeIndex = 2;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    componentID = atoi(argv[1]);
    goto LLocked;

  case modeProcess:
    typeIndex = 3;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    componentID = szgClient.getProcessID(argv[1], argv[2]);
    if (componentID == -1) {
      cout << "dmsg error: no process for that computer/name pair.\n";
      return 1;
    }
LProcess:
    messageType = string(argv[typeIndex]);
    messageBody = joinArgs( argc, argv, typeIndex+1 );
    cout << "TYPE: '" << messageType << "', BODY: '" << messageBody << "'\n";
    break;

  case modeMaster:
    typeIndex = 2;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    {
      // Virtual computer
      arAppLauncher launcher("dmsg", &szgClient);
      if (!launcher.setVircomp(argv[1])) {
        return 1;
      }
      if (!launcher.setParameters()) {
        cout << "dmsg error: invalid virtual computer definition.\n";
        return 1;
      }
      const string lockName = launcher.getMasterName();
      if (szgClient.getLock(lockName, componentID)) {
        // nobody was holding the lock
        szgClient.releaseLock(lockName);
        cout << "dmsg error: no component running on master screen.\n";
        return 1;
      }
      // Master screen is running something.
      goto LLocked;
    }

  case modeDisplay:
    typeIndex = 3;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    {
      // Virtual computer
      arAppLauncher launcher("dmsg", &szgClient);
      if (!launcher.setVircomp(argv[1])) {
        return 1;
      }
      if (!launcher.setParameters()) {
        cout << "dmsg error: invalid virtual computer definition '"
             << argv[1] << "'.\n";
        return 1;
      }
      const string lockName = launcher.getDisplayName(atoi(argv[2]));
      if (szgClient.getLock(lockName, componentID)) {
        // nobody else was holding the lock
        szgClient.releaseLock(lockName);
        cout << "dmsg error: no component running on display #"
             << atoi(argv[2]) << " of virtual computer '"
             << argv[1] << "'.\n";
        return 1;
      }
      // That screen is running something.
      goto LProcess;
    }

  case modeControl:
    typeIndex = 2;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    // Directly target the app lock, so don't mess with virtual computers.
    // Use the location instead of a virtual computer name.
    // Multiple virtual computers can share a location.
    {
      const string lockName = string(argv[1])+"/SZG_DEMO/app";
      if (szgClient.getLock(lockName, componentID)) {
        // nobody was holding the lock
        szgClient.releaseLock(lockName);
        cout << "dmsg error: no trigger component running in location '"
             << argv[1] <<"'.\n";
        return 1;
      }
      goto LLocked;
    }

  case modeService:
    typeIndex = 2;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    // Previously, arSZGClient::createComplexServiceName()
    // modified the service name given by the command line args.
    // This *seems* to do the same, less awkwardly.
    componentID = szgClient.getServiceComponentID(argv[1]);
    if (componentID < 0) {
      cout << "dmsg error: no service '" << argv[1] << "'.\n";
      return 1;
    }
    goto LLocked;

  case modeLock:
    typeIndex = 2;
    if (argc < typeIndex+1)
      goto LPrintUsage;
    if (szgClient.getLock(argv[1], componentID)) {
      // Nobody held the lock, because we got it.
      cout << "dmsg error: no held lock '" << argv[1] << "'.\n";
      // The lock will be released when we exit.
      return 1;
    }
LLocked:
    messageType = string(argv[typeIndex]);
    messageBody = joinArgs( argc, argv, typeIndex+1 );
    cout << "TYPE: '" << messageType << "', BODY: '" << messageBody << "'\n";
    break;
  }

  // We know what to send, and to whom.
  const int match = szgClient.sendMessage(
    messageType, messageBody, componentID, responseExpected);
  if ( match < 0 ) {
    // sendMessage() already complained.
    return 1;
  }

  if (responseExpected) {
    // we will eventually be able to have a timeout here
    string responseBody;
    for (;;) {
      int originalMatch = -1; // getMessageResponse sets this.
      const int status =
        szgClient.getMessageResponse(list<int>(1, match), responseBody, originalMatch);
      if (status == 0) {
        // failure
        cout << "dmsg error: no message response.\n";
        break;
      }
      if (status == 1) {
        // final response
        cout << responseBody << "\n";
        break;
      }
      if (status == -1) {
        // continuation
        cout << responseBody << "\n";
      }
      else
        cout << "dmsg error: unexpected message response status " << status << ".\n";
    }
  }

  return 0;
}
