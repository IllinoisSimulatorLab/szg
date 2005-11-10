//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

// strip flags from the command line
void striparg(int which, int& argc, char** argv){
  for (int i=which; i<argc-1; i++){
    argv[i] = argv[i+1];
  }
  --argc;
}

// Use cout not cerr in main(), so we can build RPC scripts.

int main(int argc, char** argv){

  if (argc < 2) {
LPrintUsage:
  cerr << "Usage:\n"
       << "  To send a message to a Phleet component by ID:\n"
       << "    dmsg [-r] component_ID message_type [message_body]\n"
       << "  To send a message to a Phleet component by name:\n"
       << "    dmsg [-r] -p computer_name component_name message_type "
       << "[message_body]\n"
       << "  To send a message to the master component on a virtual "
       << "computer:\n"
       << "    dmsg [-r] -m virtual_computer message_type [message_body]\n"
       << "  To send a message to a process holding the screen resource:\n"
       << "    dmsg [-r] -g virtual_computer screen_number message_type "
       << "[message_body]\n"
       << "  To send a message to the trigger process of a virtual computer:\n"
       << "    dmsg [-r] -c location message_type [message_body]\n"
       << "  To send a message to the component hosting a given service:\n"
       << "    dmsg [-r] -s service_name message_type [message_body]\n"
       << "  To send a message to the component holding a given lock:\n"
       << "    dmsg [-r] -l lock_name message_type [message_body]\n";
    return 1;
  }

  // dmsg MAY expect a response from the receiver of the message.
  // The default is to expect no response.
  // To be able to receive a response, the first flag must be -r.

  bool responseExpected = false;
  if (!strcmp(argv[1], "-r")){
    responseExpected = true;
    striparg(1, argc, argv);
  }

  // There are several modes to dmsg.
  // No command-line flags, it sends a message to the component with that ID.
  // -p sends to the component matching the computer/component_name pair.
  // -m sends to the component operating on the master screen.
  // -s sends to the component operating on a screen.
  // -c sends to the control component (holding the "demo" lock).
  // If multiple flags are given, only the last one takes effect.

  enum { modeDefault, modeProcess, modeMaster, modeScreen, modeControl, modeService, modeLock };
  int mode = modeDefault;
  
  // parse the args
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i],"-p")){
      mode = modeProcess;
      striparg(i--,argc, argv);
      // i-- counteracts the end-of-loop increment
    }
    else if (!strcmp(argv[i],"-m")){
      mode = modeMaster;
      striparg(i--,argc, argv);
    }
    else if (!strcmp(argv[i],"-g")){
      mode = modeScreen;
      striparg(i--,argc, argv);
    }
    else if (!strcmp(argv[i],"-c")){
      mode = modeControl;
      striparg(i--,argc, argv);
    }
    else if (!strcmp(argv[i],"-s")){
      mode = modeService;
      striparg(i--,argc, argv);
    }
    else if (!strcmp(argv[i],"-l")){
      mode = modeLock;
      striparg(i--,argc, argv);
    }
  }
  
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

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
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]) : string("NULL");
    break;

  case modeProcess:
    if (argc != 4 && argc != 5)
      goto LPrintUsage;
    componentID = szgClient.getProcessID(argv[1], argv[2]);
    if (componentID == -1){
      cout << "dmsg error: no process for that computer/name pair.\n";
      return 1;
    }
    messageType = string(argv[3]);
    messageBody = argc == 5 ? string(argv[4]) : string("NULL");
    break;

  case modeMaster:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    {
      // Do the virtual computer thing.
      arAppLauncher launcher;
      launcher.setSZGClient(&szgClient);
      launcher.setVircomp(argv[1]);
      if (!launcher.setParameters()){
	cout << "dmsg error: invalid virtual computer definition.\n";
	return 1;
      }
      const string lockName = launcher.getMasterName();
      if (!szgClient.getLock(lockName, componentID)){
	// something is indeed running on the master screen
	messageType = string(argv[2]);
	messageBody = argc == 4 ? string(argv[3]) : string("NULL");
      }
      else{
	// nobody was holding the lock
	szgClient.releaseLock(lockName);
	cout << "dmsg error: no component running on master screen.\n";
	return 1;
      }
    }
    break;

  case modeScreen:
    if (argc != 4 && argc != 5)
      goto LPrintUsage;
    {
      // Do the virtual computer thing.
      arAppLauncher launcher;
      launcher.setSZGClient(&szgClient);
      launcher.setVircomp(argv[1]);
      if (!launcher.setParameters()){
	cout << "dmsg error: invalid virtual computer definition.\n";
	return 1;
      }
      const string lockName = launcher.getScreenName(atoi(argv[2]));
      if (!szgClient.getLock(lockName, componentID)){
	// something is indeed running on the screen in question
	messageType = string(argv[3]);
	messageBody = argc == 5 ? string(argv[4]) : string("NULL");
      }
      else{
	// nobody else was holding the lock
	szgClient.releaseLock(lockName);
	cout << "dmsg error: no component running on specified screen.\n";
	return 1;
      }
    }
    break;

  case modeControl:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    // Directly target the app lock, so don't mess with virtual computers.
    // Use the location instead of a virtual computer name.
    // Multiple virtual computers can share a location.
    {
      const string lockName = string(argv[1])+"/SZG_DEMO/app";
      if (!szgClient.getLock(lockName, componentID)){
	// something is indeed running on that screen
	messageType = string(argv[2]);
	messageBody = argc == 4 ? string(argv[3]): string("NULL");
      }
      else{
	// nobody else was holding the lock
	szgClient.releaseLock(lockName);
	cout << "dmsg error: no trigger component running.\n";
	return 1;
      }
    }
    break;

  case modeService:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    // Previously, arSZGClient::createComplexServiceName()
    // modified the service name given by the command line args.
    // This *seems* to do the same, less awkwardly.
    componentID = szgClient.getServiceComponentID(argv[1]);
    if (componentID < 0){
      cout << "dmsg error: no such service (" << argv[1] << ").\n";
      return 1;
    }
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]): string("NULL");
    break;

  case modeLock:
    if (argc != 3 && argc != 4)
      goto LPrintUsage;
    if (szgClient.getLock(argv[1], componentID)){
      // Nobody held the lock because we got it.
      cout << "dmsg error: no such lock is currently held (" 
	   << argv[1] << ").\n";
      // The lock will be released when we exit.
      return 1;
    }
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]) : string("NULL");
    break;
  } 

  // Finally we know what to send, and to whom!
  const int match = szgClient.sendMessage(
    messageType, messageBody, componentID, responseExpected);
  if ( match < 0 ){
    // sendMessage() already cout'ed something.
    return 1;
  }

  if (responseExpected) {
    // we will eventually be able to have a timeout here
    string responseBody;
    for (;;){
      list<int> tags;
      tags.push_back(match);
      // will be filled-in with the original match, unless there is an error.
      int remoteMatch;
      const int status = szgClient.getMessageResponse(
        tags, responseBody, remoteMatch);
      if (status == 0){
	// failure
        cout << "dmsg error: failed to get message response.\n";
	break;
      }
      else if (status == -1){
	// continuation
	cout << responseBody << "\n";
      }
      else if (status == 1){
	// final response
	cout << responseBody << "\n";
	break;
      }
      else
        cout << "dmsg error: unexpected message response status "
	     << status << ".\n";
    }
  }

  return 0;
}
