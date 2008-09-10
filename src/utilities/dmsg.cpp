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
       << "    dmsg [-r] -l lock_name message_type [message_body]\n";
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
  // If multiple flags are given, only the last one takes effect.

  // Parse the args.
  enum { modeDefault, modeProcess, modeMaster, modeDisplay, modeControl, modeService, modeLock };
  int mode = modeDefault;
  for (int i=0; i<argc; i++) {
    if (!strcmp(argv[i], "-p")) {
      mode = modeProcess;
      striparg(i--, argc, argv);
      // i-- counteracts the end-of-loop increment
    }
    else if (!strcmp(argv[i], "-m")) {
      mode = modeMaster;
      striparg(i--, argc, argv);
    }
    else if (!strcmp(argv[i], "-g")) {
      mode = modeDisplay;
      striparg(i--, argc, argv);
    }
    else if (!strcmp(argv[i], "-c")) {
      mode = modeControl;
      striparg(i--, argc, argv);
    }
    else if (!strcmp(argv[i], "-s")) {
      mode = modeService;
      striparg(i--, argc, argv);
    }
    else if (!strcmp(argv[i], "-l")) {
      mode = modeLock;
      striparg(i--, argc, argv);
    }
  }

  string messageType;
  string messageBody;
  int componentID = -1;

  switch (mode) {
  default:
    goto LPrintUsage;

  case modeDefault:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    componentID = atoi(argv[1]);
    goto LLocked;

  case modeProcess:
    if (argc != 4 && argc != 5)
      goto LPrintUsage;
    componentID = szgClient.getProcessID(argv[1], argv[2]);
    if (componentID == -1) {
      cout << "dmsg error: no process for that computer/name pair.\n";
      return 1;
    }
LProcess:
    messageType = string(argv[3]);
    messageBody = string(argc == 5 ? argv[4]: "NULL");
    break;

  case modeMaster:
    if (argc != 3 && argc != 4)
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
    if (argc != 4 && argc != 5)
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
      const string lockName = launcher.getDisplayName(atoi(argv[2]));
      if (szgClient.getLock(lockName, componentID)) {
        // nobody else was holding the lock
        szgClient.releaseLock(lockName);
        cout << "dmsg error: no component running on specified screen.\n";
        return 1;
      }
      // That screen is running something.
      goto LProcess;
    }

  case modeControl:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    // Directly target the app lock, so don't mess with virtual computers.
    // Use the location instead of a virtual computer name.
    // Multiple virtual computers can share a location.
    {
      const string lockName = string(argv[1])+"/SZG_DEMO/app";
      if (szgClient.getLock(lockName, componentID)) {
        // nobody was holding the lock
        szgClient.releaseLock(lockName);
        cout << "dmsg error: no trigger component running.\n";
        return 1;
      }
      goto LLocked;
    }

  case modeService:
    if (argc != 3 && argc != 4)
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
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    if (szgClient.getLock(argv[1], componentID)) {
      // Nobody held the lock, because we got it.
      cout << "dmsg error: no held lock '" << argv[1] << "'.\n";
      // The lock will be released when we exit.
      return 1;
    }
LLocked:
    messageType = string(argv[2]);
    messageBody = string(argc == 4 ? argv[3]: "NULL");
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
