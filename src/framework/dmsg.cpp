//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"

// used to strip the flags from the command line
void striparg(int which, int& argc, char** argv){
  for (int i=which; i<argc-1; i++){
    argv[i] = argv[i+1];
  }
  argc--;
}

// the stuff you need to type in to make this work
void printusage(){
  cerr << "dmsg usage:\n";
  cerr << "  dmsg [-r] component_ID message_type [message_body] (process by ID)\n";
  cerr << "  dmsg [-r] -p computer_name component_name message_type "
       << "[message_body] (process by name)\n";
  cerr << "  dmsg [-r] -m virtual_computer message_type [message_body] (master)\n";
  cerr << "  dmsg [-r] -g virtual_computer screen_number message_type "
       << "[message_body] (screen)\n";
  cerr << "  dmsg [-r] -c location message_type [message_body] (trigger/controller/master)\n";
  cerr << "  dmsg [-r] -s service_name message_type [message_body] (service)\n";
  cerr << "  dmsg [-r] -l lock_name message_type [message_body] (lock)\n";
}

int main(int argc, char** argv){

  if (argc < 2) {
    printusage();
    return 1;
  }
  // dmsg can either expect a response or not from the receiver of the
  // message. The default is to expect no response (for historical reasons,
  // since messages originally were just kinda thrown out into the blue,
  // since that was easier to implement). To be able to receive a response,
  // the first flag must be -r

  bool responseExpected = false;

  if (!strcmp(argv[1], "-r")){
    // we should expect a response
    responseExpected = true;
    striparg(1, argc, argv);
  }

  // there are several modes to dmsg.
  // If you pass it no flags, it will send a message to the component
  // with the given ID
  // Use the flag -p and it will try to find a component matching the
  // given computer/component_name pair and send the message to it.
  // Use the flag -m and it will attempt to send the message to the
  // component operating on the master screen of the given virtual computer.
  // Use the flag -s and it will attempt to send the message to the
  // component operating on the given screen of the particular virtual
  // computer.
  // Use the flag -c and it will attempt to send the message to the control
  // component operating on the particular virtual computer (i.e. the
  // component that holds the "demo" lock

  string mode = "default";
  // first thing to do is to scan for the args
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i],"-p")){
      mode = "process";
      striparg(i,argc, argv);
      // must decrement to conteract the increment that will occur at
      // the end of the loop
      i--;
    }
    else if(!strcmp(argv[i],"-m")){
      mode = "master";
      striparg(i,argc, argv);
      // must decrement to conteract the increment that will occur at
      // the end of the loop
      i--;
    }
    else if (!strcmp(argv[i],"-g")){
      mode = "screen";
      striparg(i,argc, argv);
      // must decrement to conteract the increment that will occur at
      // the end of the loop
      i--;
    }
    else if (!strcmp(argv[i],"-c")){
      mode = "control";
      striparg(i,argc, argv);
      // must decrement to conteract the increment that will occur at
      // the end of the loop
      i--;
    }
    else if (!strcmp(argv[i],"-s")){
      mode = "service";
      striparg(i, argc, argv);
      // must decrement to conteract the increment that will occur at
      // the end of the loop
      i--;
    }
    else if (!strcmp(argv[i],"-l")){
      mode = "lock";
      striparg(i, argc, argv);
      // must decrement counter
      i--;
    }
  }
  
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  string messageType;
  string messageBody;
  int    componentID;
  
  if (mode == "default"){
    if (argc != 3 && argc != 4){
      printusage();
      return 1;
    }
    componentID = atoi(argv[1]);
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]) : string("NULL");
  }
  else if (mode == "process"){
    if (argc != 4 && argc != 5){
      printusage();
      return 1;
    }
    componentID = szgClient.getProcessID(argv[1], argv[2]);
    if (componentID == -1){
      cerr << "dmsg error: no process corresponds to that computer/name "
	   << "pair.\n";
      return 1;
    }
    messageType = string(argv[3]);
    messageBody = argc == 5 ? string(argv[4]) : string("NULL");
  }
  else if (mode == "master"){
    if (argc != 3 && argc != 4){
      printusage();
      return 1;
    }
    // we actually need to do the virtual computer thing
    arAppLauncher launcher;
    launcher.setSZGClient(&szgClient);
    launcher.setVircomp(argv[1]);
    if (!launcher.setParameters()){
      cerr << "dmsg error: invalid virtual computer definition.\n";
      return 1;
    }
    string lockName = launcher.getMasterName();
    if (!szgClient.getLock(lockName, componentID)){
      // something is indeed running on the master screen
      messageType = string(argv[2]);
      messageBody = argc == 4 ? string(argv[3]) : string("NULL");
    }
    else{
      // nobody was holding the lock
      szgClient.releaseLock(lockName);
      cerr << "dmsg error: no component running on master screen.\n";
      return 1;
    }
  }
  else if (mode == "screen"){
    if (argc != 4 && argc != 5){
      printusage();
      return 1;
    }
    // we actually need to do the virtual computer thing
    arAppLauncher launcher;
    launcher.setSZGClient(&szgClient);
    launcher.setVircomp(argv[1]);
    if (!launcher.setParameters()){
      cerr << "dmsg error: invalid virtual computer definition.\n";
      return 1;
    }
    string lockName = launcher.getScreenName(atoi(argv[2]));
    if (!szgClient.getLock(lockName, componentID)){
      // something is indeed running on the screen in question
      messageType = string(argv[3]);
      messageBody = argc == 5 ? string(argv[4]) : string("NULL");
    }
    else{
      // nobody else was holding the lock
      szgClient.releaseLock(lockName);
      cerr << "dmsg error: no component running on specified screen.\n";
      return 1;
    }
  }
  else if (mode == "control"){
    if (argc != 3 && argc != 4){
      printusage();
      return 1;
    }
    // WE DO NOT NEED TO MESS WITH VIRTUAL COMPUTERS IN THIS CASE!
    // (WE ARE DIRECTLY TARGETING THE "APPLICATION LOCK")

    // NOTE: here we use the location instead of a virtual computer name.
    // Multiple virtual computers can share a location.
    string lockName = string(argv[1])+"/SZG_DEMO/app";
    if (!szgClient.getLock(lockName, componentID)){
      // something is indeed running on the screen in question
      messageType = string(argv[2]);
      messageBody = argc == 4 ? string(argv[3]): string("NULL");
    }
    else{
      // nobody else was holding the lock
      szgClient.releaseLock(lockName);
      cerr << "dmsg error: no trigger component running.\n";
      return 1;
    }
  }
  else if (mode == "service"){
    if (argc != 3 && argc != 4){
      printusage();
      return 1;
    }
    componentID = szgClient.getServiceComponentID(
                              szgClient.createComplexServiceName(argv[1]));
    if (componentID < 0){
      cerr << "dmsg error: no such service exists (" << argv[1] << ").\n";
      return 1;
    }
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]): string("NULL");
  }
  else if (mode == "lock"){
    if (argc != 3 && argc != 4){
      printusage();
      return 1;
    }
    if (szgClient.getLock(argv[1], componentID)){
      // nobody is holding the lock because we were able to get it
      cerr << "dmsg error: no such lock is currently held (" 
	   << argv[1] << ").\n";
      // The lock will be released when we exit.
      return 1;
    }
    messageType = string(argv[2]);
    messageBody = argc == 4 ? string(argv[3]) : string("NULL");
  } 

  int match = szgClient.sendMessage(messageType, messageBody, 
                                    componentID, responseExpected);
  if ( match < 0 ){
    // no need to print something here... sendMessage already does.
    return 1;
  }
  if (responseExpected){
    // with the new szg, we will eventually be able to have a timeout here
    int messageID;
    string responseBody;
    // getMessageResponse returns -1 upon receiving a "continuation",
    // 1 upon receiving a final response, and 0 upon failure.
    bool done = false;
    while (!done){
      list<int> tags;
      tags.push_back(match);
      // will be filled-in with the original match, unless there is an error.
      int remoteMatch;
      int status = szgClient.getMessageResponse(tags, responseBody, 
                                                remoteMatch);
      if (status == 0){
        cerr << "dmsg error: problem in receiving message response.\n";
	done = true;
      }
      if (status == -1){
	cout << responseBody << "\n";
      }
      if (status == 1){
	cout << responseBody << "\n";
	done = true;
      }
    }
  }
  return 0;
}
