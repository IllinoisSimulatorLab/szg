//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arDataUtilities.h"
#include <stdio.h>
#include <list>
#include <sstream>
using namespace std;
#ifdef AR_USE_WIN_32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <errno.h>
  #include <signal.h>
#endif

// A throw-away class for passing data to the exec threads
class ExecutionInfo{
public:
  ExecutionInfo(){}
  ~ExecutionInfo(){}

  string userName;
  string messageContext;
  string messageBody;
  int receivedMessageID;
};

// We want to make every trading key unique. Consequently, use a 
// "trading num" that is incremented each time.
int tradingNum = 0;
arMutex tradingNumLock;
arMutex processCreationLock;

arSZGClient* SZGClient = NULL;

// Given the specified user and argument string, contact the szgserver and
// determine the user's execution path. Next, given the arg string sent to 
// szgd and the execution path, figure out the file that should be executed.
// Finally, determine the proper command and the list of args, which are 
// returned by reference. NOTE: the proper command and arg list will be 
// constructed differently if this is a python script or a native executable.
// NOTE: the only inputs are "user" and "argSring", with "user" being the
// phleet user as determined by the message context of the "dex" message and
// "argString" being the body of the "dex" message.
// The rest of the args are really return values (by reference).
// "execPath" is simply the user's SZG_EXEC path on the computer running 
//   this szgd.
// "symbolicCommand" is the cross-platform command designation (i.e. atlantis 
//   not atlantis.exe). This WILL NOT be a long file name 
//   (i.e. not g:\foo\bar\atlantis)
//   and, in fact, the leading path will be stripped away as well.
// "command" can be one of two things:
//    1. if we are executing a python program, this will be "python".
//    2. if we are executing a native program, this will be the full path to 
//       the executable.
// "args" is a string list of the args.... but there may be MANGLING.
//    1. if we are executing a native program, this will be the arglist after
//       the exename (i.e. on unix argv[1]... argv[argc-1])
//    2. for python, this will be the full exename plus the args.
bool buildFunctionArgs(const string& user,
                       const string& argString,
                       string& execPath,
                       string& symbolicCommand,
                       string& command,
                       list<string>& args){
  // First, clear "args" and tokenize the argString, placing the first token
  // in command and the other tokens in the args list. 
  // The first element is our candidate for the execuatble, either
  // in native format, or python.
  args.clear();
  arDelimitedString tmpArgs(argString, ' ');
  if (tmpArgs.size() < 1){
    cout << "szgd error: execution attempt on empty arg string.\n";
    return false;
  }
  command = tmpArgs[0];
  int i;
  for (i=1; i<tmpArgs.size(); i++){
    // DO NOT PUSH EXTRA WHITESPACE
    if (tmpArgs[i].length() > 0){
      args.push_back(tmpArgs[i]);
    }
  }

  // Next, retrieve the exec path.
  execPath = SZGClient->getAttribute(user, "NULL", "SZG_EXEC", "path", "");
  if (execPath == "NULL"){
    cout << "szgd warning: exec path not set.\n";
  }

  // Next, determine if the first token in the argString is located in the
  // exec path. To do this:
  //   a. Strip the token (i.e. remove .exe or .EXE, for windows)
  //   b. Determine if this a python script (via the extension)
  //      i. If so, do nothing
  //      ii. If not, and we are operating on windows, add .EXE to the token.
  //   c. Go ahead and try to find the file on the path, returning false if
  //      this fails.
  command = ar_stripExeName(command);
  symbolicCommand = command;
  string executableType;
  if (command.substr(command.length()-3, 3) == ".py"){
    executableType = "python";
  }
  else{
    executableType = "native";
#ifdef AR_USE_WIN_32
    command = command + ".EXE";
#endif
  }
  string fileName = command;
  command = ar_fileFind(fileName, "", execPath);
  if (command == "NULL"){
    cout << "szgd error: could not find file " << fileName
	 << "\n  on exec path " << execPath << ".\n";
    return false;
  }

  // If we did find the file, the next step depends on what sort of executable
  // we have.
  if (executableType == "native"){
    // Nothing to do here
  }
  else if (executableType == "python"){
    // The command needs to be changed to "python" and the former command
    // needs to be push onto the front of the args list.
    args.push_front(command);
    command = "python";
  }
  else{
    cout << "szgd error: unhandled executable type.\n";
    return false;
  }
  // Go ahead and print out the information.
  cout << "szgd remark:\n"
       << "  user name=" << user << ".\n"
       << "  executable name=" << command << ".\n"
       << "  executable type=" << executableType << ".\n"
       << "  arg list=(";
  for (list<string>::iterator iter = args.begin();
       iter != args.end(); iter++){
    if (iter != args.begin()){
      cout << ",";
    }
    cout << *iter;
  }
  cout << ")\n";
  return true;
}

char** buildUnixStyleArgList(const string& command, list<string>& args){
  // Two passes. First of all, figure out the length of the string list.
  int listLength = 0;
  list<string>::iterator i;
  for (i=args.begin(); i!=args.end(); i++){
    listLength++;
  }
  // Allocate storage and fill with the contents of args.
  char** result = new char*[listLength+2];
  // The first element in the list should be the command.
  result[0] = new char[command.length()+1];
  ar_stringToBuffer(command, result[0], command.length()+1);
  // Must be null terminated.
  result[listLength+1] = NULL;
  int index = 1;
  for (i=args.begin(); i!=args.end(); i++){
    result[index] = new char[(*i).length()+1];
    ar_stringToBuffer(*i, result[index], (*i).length()+1);
    index++;
  }
  return result;
}

void deleteUnixStyleArgList(char** argList){
  int index = 0;
  while (argList[index]){
    delete argList[index];
    index++;
  }
}

string buildWindowsStyleArgList(const string& command, list<string>& args){
  string result;
  result += command;
  if (!args.empty()){
    result += " ";
  }
  for (list<string>::iterator i = args.begin();
       i != args.end(); i++){
    // for every element except for the first, we want to add a space
    if (i != args.begin()){
      result += " ";
    }
    result += *i;
  }
  return result;
}

// a random delay before executing. this prevents a file server from getting
// hit *too* hard
void randomDelay(){
  // It seems that there can be bad interactions between 
  // samba and win32 when too many guys try to execute the same
  // exe at once... we take the low digits of the time...
  ar_timeval time1 = ar_time();
  int delay = 100000 * abs(time1.usec%6);
  ar_usleep(delay);
}

void execProcess(void* i){
  ExecutionInfo* execInfo = (ExecutionInfo*) i;
  string userName = execInfo->userName;
  string messageContext = execInfo->messageContext;
  string messageBody = execInfo->messageBody;
  int receivedMessageID = execInfo->receivedMessageID; 

  // Start out with the part of executable launching that is the same
  // between Windows and Unix (not much)
  stringstream info;
  // We might have to respond ourselves if the executable won't launch.
  info << "*user=" << userName
       << ", context=" << messageContext
       << ", *computer=" << SZGClient->getComputerName() << "\n";

  string execPath;
  string symbolicCommand;
  string newCommand;
  list<string> mangledArgList;
  if (!buildFunctionArgs(userName, messageBody, execPath, 
                         symbolicCommand,
                         newCommand, mangledArgList)){
    info << "szgd error: failed to find file " << symbolicCommand << "\n"
	  << "on path " << execPath << ".\n";
    // note how we respond to the message ourselves
    SZGClient->messageResponse(receivedMessageID, info.str());
    return;
  }

  // We've got something to try to execute. We'll invoke a message
  // trade so the executed program can respond.
  // Don't forget to increment the "trading number" that makes this
  // request unique. Note that this can be executed simultaneously in
  // different threads. Consequently, must use a lock.
  ar_mutex_lock(&tradingNumLock);
  tradingNum++;
  stringstream tradingNumStream;
  tradingNumStream << tradingNum;
  ar_mutex_unlock(&tradingNumLock);
  const string tradingKey(SZGClient->getComputerName() + "/" 
                          + tradingNumStream.str() + "/"
	                  + symbolicCommand);
  cout << "szgd remark: using trading key = " << tradingKey << "\n";
  int match = SZGClient->startMessageOwnershipTrade(receivedMessageID,
					            tradingKey);
#ifndef AR_USE_WIN_32
  //*******************************************************************
  //*******************************************************************
  // Code for spawning a new process on Unix (i.e. Linux, OS X, Irix)
  //*******************************************************************
  //*******************************************************************
  int pipeDescriptors[2];
  if (pipe(pipeDescriptors) < 0){
    cerr << "szgd warning: exec failed (pipe creation failed).\n";
    return;
  }
  // NOTE: to make it impossible for szgd to JAM (which is very bad behavior)
  // it is necessary to make the read pipe NONBLOCKING
  int propertyValue = fcntl(pipeDescriptors[0], F_GETFL, 0);
  fcntl(pipeDescriptors[0], F_SETFL, propertyValue | O_NONBLOCK);

  char numberBuffer[8];
  int PID = fork();
  if (PID < 0){
    cerr << "szgd error: fork failed.\n";
    return;
  }
  else if (PID > 0){
    // We are in the parent process still

    // we block here waiting on the child process to send
    // information regarding whether or not it has successfully launched
    // an executable. If it fails to launch an executable, the
    // szgd part of the child will do so... otherwise the launched
    // executable will do so
    numberBuffer[0]= 0;
    // Ten second time-out for pipe.
    if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer,
			         1, 10000)){
      info << "szgd remark: did not get success/failure "
           << "code via pipe.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
      return;
    }
    // we read in 0 on failure to launch and 1 on launch success
    if (numberBuffer[0] == 0){
      // the launch has failed. we will be receiving error messages from
      // the szgd side of the fork
      // first, we need to revoke the "message trade" since the
      // other end of this code would be invoked in the arSZGClient
      // of the exec'ed process
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      // next, we read in the error information from the pipe
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer, 
                                   sizeof(int), 1000)){
	info << "szgd remark: pipe-based handshake failed.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
        return;
      }
      if (*(int*)numberBuffer < 0 || *(int*)numberBuffer > 10000){
	cout << "szgd warning: numberBuffer value ("
	     << *(int*)numberBuffer << ") makes no sense. Continuing "
	     << "anyway.\n";
      }
      char* textBuffer = new char[*((int*)numberBuffer)+1];
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0], textBuffer, 
                                   *((int*)numberBuffer), 1000)){
	info << "szgd remark: pipe-based handshake failed, text phase.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
	delete [] textBuffer;
        return;
      }
      textBuffer[*((int*)numberBuffer)] = '\0';
      // note how we respond to the message ourselves
      SZGClient->messageResponse(receivedMessageID, string(textBuffer));
      delete [] textBuffer;
      return;
    }
    else{
      // in this case, numberBuffer[0] = 1 and the launch must have
      // succeeded. we do not receive the info via the pipe
      // in this case... instead the launched client goes ahead
      // and sends that to the "dex" directly. We must, however,
      // wait for the "message trade" to have occured
      if (!SZGClient->finishMessageOwnershipTrade(match,10000)){
	info << "szgd remark: message ownership trade timed out.\n";
        SZGClient->revokeMessageOwnershipTrade(tradingKey);
        SZGClient->messageResponse(receivedMessageID, info.str());
      }
    }
    // don't forget to close the pipes, since they are allocated each
    // time... would it be better to allocate the pipes once??
    // or would that be less secure?
    close(pipeDescriptors[0]);
    close(pipeDescriptors[1]);
    return;
  }
  else{
    // We are in the child process.
    // Set a few env vars for the child process.
    ar_setenv("SZGUSER",userName);
    ar_setenv("SZGCONTEXT",messageContext);
    ar_setenv("SZGPIPEID", pipeDescriptors[1]);
    ar_setenv("SZGTRADINGNUM", tradingNumStream.str());

    info << "szgd remark: running " << symbolicCommand << " on path\n"
         << execPath << ".\n";

    char** theArgs = buildUnixStyleArgList(newCommand, mangledArgList);
    // Stagger launches so the cluster's file server isn't hit so hard.
    randomDelay();
    if (execv(newCommand.c_str(), theArgs) >= 0){
      // The child spawned okay, so remove parent's side of the fork().
      // (Actually, if the child spawned, exec() doesn't return
      // so the exit(0) isn't reached.)
      exit(0);
    }
    deleteUnixStyleArgList(theArgs);

    info << "szgd error: failed to exec \"" 
         << symbolicCommand << "\":\n\treason:  ";
    switch (errno){
    case E2BIG: info << "args + env too large\n";
      break;
    case EACCES: info << "locking or sharing violation\n";
      break;
    case ENOENT: info << "file not found\n";
      break;
    case ENOEXEC: info << "file format not executable\n";
      break;
    case ENOMEM: info << "out of memory, possibly\n";
      break;
    case EFAULT: info << "command or argv pointer was invalid:\n";
      break;
    default: info << "errno is " << errno << endl;
      break;
    }
	  
    // must do the handshake back to the parent process
    string terminalOutput = info.str();
    // we failed to launch the exe
    numberBuffer[0] = 0;
    if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, 1)){
      cout << "szgd remark: failed to send failure code over pipe.\n";
    }
    *((int*)numberBuffer) = terminalOutput.length();
    if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, 
                          sizeof(int))){
       cout << "szgd remark: incomplete pipe-based handshake.\n";
    }   
    if (!ar_safePipeWrite(pipeDescriptors[1], terminalOutput.c_str(),
		          terminalOutput.length())){
      cout << "szgd remark: incomplete pipe-based handshake, "
	   << "text stage.\n";
    }
    // Kill the child process here,
    // because we don't want the orphaned process to start up again 
    // on the message loop, causing no end of confusion!!!
    exit(0);
  }
#else
  //*******************************************************************
  //*******************************************************************
  // Code for spawning a new process on Win32
  //*******************************************************************
  //*******************************************************************
  PROCESS_INFORMATION theInfo;
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);

  // NOTE: THIS IS REALLY ANNOYING. We are passing certain pieces of 
  // information into the child process via environment variables.
  // On Unix this isn't so bad: we've already inside a fork and,
  // consequently, can change the environment with impunity. On Windows, the
  // situation is different. Here, process creation doesn't work in the
  // tree-like unix fashion. Consequently, a lock is needed.
  // (another solution would be to pass in an altered environment block)
  // (another solution would be to figure out a way to send the spawned process
  // a message)
  ar_mutex_lock(&processCreationLock);
  // Set a few env vars for the child process.
  ar_setenv("SZGUSER",userName);
  ar_setenv("SZGCONTEXT",messageContext);
  // even though we only use the pipe to communicate with the child
  // process on the Unix side, we still need to be able to tell the
  // child that it was launched by szgd instead of at the command line
  // on this side... the -1 guarantees that we aren't on the Unix side
  // (where this would be a file descriptor)
  ar_setenv("SZGPIPEID", -1);
  ar_setenv("SZGTRADINGNUM", tradingNumStream.str());

  // Stagger launches so the cluster's file server isn't hit so hard.
  randomDelay();
  string theArgs = buildWindowsStyleArgList(newCommand, mangledArgList);
  bool fArgs;
  if (theArgs == ""){
    fArgs = false;
  }
  else{
    fArgs = true;
  }
  // Unfortunately, we actually need to pack these buffers... can't just use
  // my_string.c_str().
  char* command = new char[newCommand.length()+1];
  ar_stringToBuffer(newCommand, command, newCommand.length()+1);
  char* argsBuffer = new char[theArgs.length()+1];
  ar_stringToBuffer(theArgs, argsBuffer, theArgs.length()+1);
  // NOTE: The process might fail to run for various reasons after
  // successfully being created. For instance, a needed DLL might not exist.
  if (!CreateProcess(command, fArgs?argsBuffer:NULL, 
                     NULL, NULL, false,
                     NORMAL_PRIORITY_CLASS, NULL, NULL, 
                     &si, &theInfo)){
    ar_mutex_unlock(&processCreationLock);
    info << "szgd warning: failed to exec \"" << newCommand
         << "\";\n\twin32 GetLastError() returned " 
         << GetLastError() << ".\n";
    // in this case, we will be responding directly, not the spawned
    // process
    SZGClient->revokeMessageOwnershipTrade(tradingKey);
    SZGClient->messageResponse(receivedMessageID, info.str());
  }
  else{
    ar_mutex_unlock(&processCreationLock);
    // the spawned process will be responding
    if (!SZGClient->finishMessageOwnershipTrade(match, 10000)){
      info << "szgd warning: ownership trade timed-out.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
    }
  }
  delete [] command;
  delete [] argsBuffer;
  return;
#endif
}

int main(int argc, char** argv){
#ifndef AR_USE_WIN_32
  // If $DISPLAY is not 0:0, it will throw up a window on a
  // screen you do NOT expect.  Should we test for this?

  // Kill zombie processes in unix.
  signal(SIGCHLD,SIG_IGN);
#endif

  ar_mutex_init(&tradingNumLock);
  ar_mutex_init(&processCreationLock);

  SZGClient = new arSZGClient;
  // note how we force the name of the component. This is because it is
  // impossible to get the name automatically on Win98 and we want to run
  // szgd on Win98
  SZGClient->init(argc, argv, "szgd");
  if (!*SZGClient)
    return 1;
  
  // only a single szgd should be running on a given computer
  int ownerID = -1;
  if (!SZGClient->getLock(SZGClient->getComputerName()+"/szgd", ownerID)){
    cerr << argv[0] << " error: another copy is already running (pid = " 
         << ownerID << ").\n";
    return 1;
  }
  
  string userName, messageType, messageBody, messageContext;
  while (true){
    int receivedMessageID = 
      SZGClient->receiveMessage(&userName, &messageType, 
                                &messageBody, &messageContext);
    
    if (messageType=="quit"){
      // Just in case exit() misses SZGClient's destructor.
      SZGClient->closeConnection();
      // will return(0) be gentler, yet kill the child processes too?
      exit(0);
    }
    else if (messageType=="exec"){
      // BUG BUG BUG BUG BUG
      // THERE IS A SMALL MEMORY LEAK HERE IN THAT THE EXECUTION INFO IS
      // NEVER DELETED
      ExecutionInfo* info = new ExecutionInfo();
      info->userName = userName;
      info->messageBody = messageBody;
      info->messageContext = messageContext;
      info->receivedMessageID = receivedMessageID;
      // The exec call is long-blocking. Consequently, it is handled in its
      // own thread. For this to be possible, the various arSZGClient
      // methods must be thread-safe.
      arThread execThread;
      execThread.beginThread(execProcess, info);
    }
  }
  return 1;
}
