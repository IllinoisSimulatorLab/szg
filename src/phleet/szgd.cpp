//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
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

// So every trading key is unique, increment tradingNum each time.
int tradingNum = 0;
arMutex tradingNumLock;
arMutex processCreationLock;

arSZGClient* SZGClient = NULL;

// By convention, we assume that python applications are installed as
// "bundles" in sub-directories on a directory on SZG_PYTHON/path.
// The application bundles can have arbitrary names. This function
// searches the path, piece by piece. In each piece, it examines
// subdirectories, looking for one containing the python script. The
// first such found is returned.
//
// python_directory1
//     my_app_directory_1
//          python_script_1.py
//     my_app_directory_2
//          python_script_2.py
// python_directory2
//     my_app_directory_3
//          python_script_2.py
//
// where SZG_PYTHON/path = python_directory1;python_directory2.
//
// In this case, if pyfile == python_script_2.py, then 
// python_directory1/my_app_directory_2 will be returned.
string getPythonPath( const string& userName, string pyfile ) {
  const string pythonScriptPath = SZGClient->getAttribute(
    userName, "NULL", "SZG_PYTHON", "path", "");
  if (pythonScriptPath == "NULL") {
    cerr << "szgd error: SZG_PYTHON/path not set.\n";
    return "NULL";
  }
  // Traverse the python data path. In each directory,
  // step through all subdirectories, looking for the named
  // file. The first one found is the directory of execution.
  arSemicolonString pythonPath(pythonScriptPath);
  string actualDirectory("NULL");
  for (int i=0; i<pythonPath.size(); ++i){
    list<string> contents = ar_listDirectory(pythonPath[i]);
    for (list<string>::iterator iter = contents.begin(); iter != contents.end(); ++iter) {
      string potentialFile = *iter;
      ar_pathAddSlash(potentialFile);
      potentialFile += pyfile;
      FILE* filePtr = ar_fileOpen(potentialFile,"","","r");
      if (filePtr){
        ar_fileClose(filePtr);
        actualDirectory = *iter;
        break;
      }
    }
    if (actualDirectory != "NULL"){
      break;
    }
  }
  return actualDirectory;
}

// Little class to pass data to exec threads.
class ExecutionInfo{
public:
  ExecutionInfo(){}
  ~ExecutionInfo(){}

  string userName;
  string messageContext;
  string messageBody;
  int receivedMessageID;
  int timeoutmsec;

  string executableType;
  string pyDirPath;
};

// Given the specified user and argument string, contact the szgserver and
// determine the user's execution path. Next, given the arg string sent to 
// szgd and the execution path, figure out the file that should be executed.
// Finally, determine the proper command and the list of args, which are 
// returned by reference. NOTE: the proper command and arg list will be 
// constructed differently if this is a python script or a native executable.
// NOTE: the only inputs are "userName" and "argSring", with "userName" being the
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
string buildFunctionArgs(ExecutionInfo* execInfo,
                       string& execPath,
                       string& symbolicCommand,
                       string& command,
                       list<string>& args){
  string userName = execInfo->userName;
  string argString = execInfo->messageBody;
  // First, clear "args" and tokenize the argString, placing the first token
  // in command and the other tokens in the args list. 
  // The first element is our candidate for the execuatble, either
  // in native format, or python.
  args.clear();
  arDelimitedString tmpArgs(argString, ' ');
  if (tmpArgs.size() < 1) {
    return "szgd error: execution attempt on empty arg string.\n";
  }
  command = tmpArgs[0];
  int i;
  for (i=1; i<tmpArgs.size(); ++i) {
    // Skip extra whitespace.
    if (tmpArgs[i].length() > 0) {
      args.push_back(tmpArgs[i]);
    }
  }

  execPath = SZGClient->getAttribute(userName, "NULL", "SZG_EXEC", "path", "");
  if (execPath == "NULL"){
    cout << "szgd warning: exec path not set.\n";
  }

  // Determine if the argString's first token is in the exec path:
  //   a. Strip the token (remove .EXE)
  //   b. Determine if this a python script (via the extension)
  //      If not, under win32 append .EXE.
  //   c. Try to find the file on the path.
#ifdef AR_USE_WIN_32
  const string commandRaw(command);
#endif
  command = ar_stripExeName(command);
#ifdef AR_USE_WIN_32
  const bool fHadEXE = command != commandRaw;
#endif
  symbolicCommand = command;
  string fileName;
  ostringstream errStream;
  if (command.length() > 3 && command.substr(command.length()-3, 3) == ".py") {
    execInfo->executableType = "python";
    fileName = command;
    execInfo->pyDirPath = getPythonPath( userName, fileName );
    if (execInfo->pyDirPath == "NULL") {
      const string pythonScriptPath = SZGClient->getAttribute(
        userName, "NULL", "SZG_PYTHON", "path", "");
      errStream << "szgd error: no python script '" << fileName
                << "' on user " << userName << "'s SZG_PYTHON/path '"
                << "    " << pythonScriptPath << "'\n";
      return errStream.str();
    }
    command = ar_fileFind( fileName, "", execInfo->pyDirPath);
    if (command == "NULL") {
      errStream << "szgd error: could not find file " << fileName
           << "\n  on python source path " << execInfo->pyDirPath << ".\n";
      return errStream.str();
    }
  }
  else {
    execInfo->executableType = "native";
#ifdef AR_USE_WIN_32
    command += ".EXE";
#endif
    fileName = command;
    command = ar_fileFind(fileName, "", execPath);
    if (command == "NULL") {
      errStream << "szgd error: no executable file '" << fileName
           << "' on path " << execPath << ".\n";
LAbort:
#ifdef AR_USE_WIN_32
      if (fHadEXE)
        errStream << "szgd warning: don't append .exe;  Windows does that for you.\n";
#endif
      return errStream.str();
    }
  }

  // Found the file.  What kind of exe is it?
  if (execInfo->executableType == "native") {
    // Nothing to do
  }
  else if (execInfo->executableType == "python") {
    // The command needs to be changed to either:
    // (1) The value of the Syzygy database variable SZG_PYTHON/executable; or if not set,
    // (2) the value of the environment variable SZG_PYEXE; or if not set,
    // (3) "python",
    // and the former command // (i.e. the script name) needs to be pushed
    // onto the front of the args list.
    args.push_front(command);
    string pyExeString = ar_getenv("SZG_PYEXE");
    string szgPyExe = SZGClient->getAttribute(userName, "NULL", "SZG_PYTHON", "executable", "");
    if (szgPyExe != "NULL") {
      pyExeString = szgPyExe;
    }
    if ((pyExeString == "NULL")||(pyExeString == "")) {
      command = "python";
    } else {
      // allow for /-delimited cmdline args in SZG_PYEXE or SZG_PYTHON/executable
      arSlashString pySpaceString( pyExeString );
      command = pySpaceString[0];
      for (int ind=1; ind < pySpaceString.size(); ++ind) {
        args.push_front( pySpaceString[ind] );
      }
    }
#ifdef AR_USE_WIN_32
    command += ".EXE";
#endif
    fileName = command;
    command = ar_fileFind( fileName, "", execPath );
    if (command == "NULL") {
      errStream << "szgd error: no python executable file " << fileName
           << "\n  on exec path " << execPath << ".\n";
      goto LAbort;
    }
  }
  else {
    errStream << "szgd error: unexpected exe type \""
         << execInfo->executableType << "\".\n";
    return errStream.str();
  }

  cout << "szgd remark:\n"
       << "  user name=" << userName << "\n"
       << "  exe name=" << command << "\n"
       << "  exe type=" << execInfo->executableType << "\n"
       << "  arg list=(";
  for (list<string>::iterator iter = args.begin();
       iter != args.end(); ++iter){
    if (iter != args.begin()){
      cout << ", ";
    }
    cout << *iter;
  }
  cout << ")\n";
  return string("OK");
}

char** buildUnixStyleArgList(const string& command, list<string>& args) {
  // Allocate storage and fill with the contents of args.
  const int listLength = args.size();
  char** result = new char*[listLength+2];
  // The first element in the list should be the command.
  result[0] = new char[command.length()+1];
  ar_stringToBuffer(command, result[0], command.length()+1);
  // Null-terminate.
  result[listLength+1] = NULL;
  int index = 1;
  for (list<string>::iterator i=args.begin(); i!=args.end(); ++i) {
    result[index] = new char[i->length()+1];
    ar_stringToBuffer(*i, result[index], i->length()+1);
    ++index;
  }
  return result;
}

void deleteUnixStyleArgList(char** argList) {
  for (int index = 0; argList[index]; ++index) {
    delete argList[index];
  }
}

string buildWindowsStyleArgList(const string& command, list<string>& args) {
  string result(command);
  if (!args.empty()) {
    result += " ";
  }
  for (list<string>::iterator i = args.begin(); i != args.end(); ++i) {
    // for every element except for the first, we want to add a space
    if (i != args.begin()) {
      result += " ";
    }
    result += *i;
  }
  return result;
}

// Use this to reduce samba win32 fileserver load when many
// hosts run one exe at the same time.
void randomDelay() {
  const ar_timeval time1 = ar_time();
  ar_usleep(100000 * abs(time1.usec % 6));
}

static void TweakPath(string& path) {
  // Point slashes in the right direction.
  ar_scrubPath(path);
#ifndef AR_USE_WIN_32
  // szg paths use Win32 ';' separator.  In Unix, change to ':'
  unsigned int pos;
  while ((pos = path.find(";")) != string::npos) {
    path.replace( pos, 1, ":" );
  }
#endif
}

void execProcess(void* i){
  ExecutionInfo* execInfo = (ExecutionInfo*) i;
  const string userName = execInfo->userName;
  const string messageContext = execInfo->messageContext;
  const string messageBody = execInfo->messageBody;
  int receivedMessageID = execInfo->receivedMessageID; 

  // Start out with the part of executable launching that is the same
  // between Windows and Unix (not much)
  stringstream info;
  // We might have to respond ourselves if the executable won't launch.
  info << "*user=" << userName
       << ", context=" << messageContext
       << ", *computer=" << SZGClient->getComputerName() << endl;

  string execPath;
  string symbolicCommand;
  string newCommand;
  list<string> mangledArgList;
  const string statString = buildFunctionArgs( execInfo, execPath, 
                         symbolicCommand, newCommand, mangledArgList);
  if (statString != "OK") {
    info << statString;
    // we respond to the message ourselves
    SZGClient->messageResponse(receivedMessageID, info.str());
LDone:
    delete execInfo;
    return;
  }

  // We've got something to try to execute. We'll invoke a message
  // trade so the executed program can respond.
  // Don't forget to increment the "trading number" that makes this
  // request unique. Note that this can be executed simultaneously in
  // different threads. Consequently, must use a lock.
  ar_mutex_lock(&tradingNumLock);
    stringstream tradingNumStream;
    tradingNumStream << ++tradingNum;
  ar_mutex_unlock(&tradingNumLock);
  const string tradingKey(SZGClient->getComputerName() + "/" 
                          + tradingNumStream.str() + "/"
	                  + symbolicCommand);
  cout << "szgd remark: trading key = " << tradingKey << endl;
  int match = SZGClient->startMessageOwnershipTrade(receivedMessageID, tradingKey);

  // Two dynamic search paths (as embodied in environment variables) need
  // to be altered before launching the new executable (and in the case of
  // Win32 altered back after launch). The info to do the alterations comes
  // from szg database variables. We go ahead and get this info here since
  // the arSZGClient cannot be used after the Unix fork.
  // Also, a description of the dynamic search paths follows:

  // The library search path must be altered for each user. This is
  // necessary to let multiple users (with different ways of arranging and
  // loading dynamic libraries) to use szgd at once from another user's 
  // account.
  // By convention, we make the system search for libraries in the
  // following order:
  //   1. Directory on the exec path where the executable resides.
  //   2. SZG_NATIVELIB/path
  //   3. SZG_EXEC/path
  //   4. The native DLL search path (i.e. LD_LIBRARY_PATH or
  //      DYLD_LIBRARY_PATH or LD_LIBRARYN32_PATH or PATH) as held by the
  //      user running the szgd.
  // NOTE: This path must be altered both in the case of "native" AND "python"
  // executables. 

  // The python module search path must also be modified for each user, for
  // similar reasons. It seems like python, by default, will prepend the
  // directory in which the .py file lives.
  //   1. SZG_PYTHON/path (python modules can be put in the top level of
  //      your "application bundle directory")
  //   2. SZG_PYTHON/lib_path (conceived of as where python modules might go,
  //      but not "application bundles")
  //   3. SZG_EXEC/path (PySZG.py and PySZG.so (or .dll depending on platform)
  //      must be in the same directory and on the PYTHONPATH).
  //   4. PYTHONPATH, as held by the user running the szgd.

  string dynamicLibraryPathVar =
#ifdef AR_USE_LINUX
    "LD_LIBRARY_PATH";
#endif
#ifdef AR_USE_SGI
    "LD_LIBRARYN32_PATH";
#endif
#ifdef AR_USE_WIN_32
    "PATH";
#endif
#ifdef AR_USE_DARWIN
    "DYLD_LIBRARY_PATH";
#endif

  // Do not warn again here if SZG_EXEC/path is NULL. Said warning has 
  // already occured.
  const string szgExecPath =
                 SZGClient->getAttribute(userName, "NULL", "SZG_EXEC", "path","");
  const string oldDynamicLibraryPath = ar_getenv(dynamicLibraryPathVar.c_str());
  const string nativeLibPath =
                 SZGClient->getAttribute(userName, "NULL", "SZG_NATIVELIB","path", "");
  // Construct the new dynamic library path.
  string dynamicLibraryPath;
  if (ar_exePath(newCommand) != "") {
    dynamicLibraryPath += ar_exePath(newCommand);
  }
  if (nativeLibPath != "NULL") {
    dynamicLibraryPath += ";" + nativeLibPath;
  }
  if (szgExecPath != "NULL") {
    dynamicLibraryPath += ";" + szgExecPath;
  }
  if (oldDynamicLibraryPath != "" && oldDynamicLibraryPath != "NULL") {
    dynamicLibraryPath += ";" + oldDynamicLibraryPath;
  }
  TweakPath(dynamicLibraryPath);

  // Deal with python
  string pythonPath;
  string oldPythonPath;
  if (execInfo->executableType == "python") {
    oldPythonPath = ar_getenv( "PYTHONPATH" );
    // if SZG_PYTHON/path not set, warning was already displayed.
    const string szgPythonPath =
                   SZGClient->getAttribute(userName, "NULL", "SZG_PYTHON", "path", "");
    if (szgPythonPath != "NULL") {
      pythonPath += szgPythonPath;
    }
    // Do not warn if this is unset.
    const string szgPythonLibPath =
               SZGClient->getAttribute(userName, "NULL", "SZG_PYTHON", "lib_path", "");
    if (szgPythonLibPath != "NULL") {
      pythonPath += ";" + szgPythonLibPath;
    }
    if (szgExecPath != "NULL") {
      pythonPath += ";" + szgExecPath;
    }
    if (oldPythonPath != "" && oldPythonPath != "NULL") {
      pythonPath += ';' + oldPythonPath;
    }
    TweakPath(pythonPath);
  }

#ifndef AR_USE_WIN_32

  //*******************************************************************
  // spawn a new process on Unix (i.e. Linux, OS X, Irix)
  //*******************************************************************
  int pipeDescriptors[2] = {0};
  if (pipe(pipeDescriptors) < 0) {
    cerr << "szgd warning: failed to create pipe.\n";
    goto LDone;
  }
  // Make the read pipe nonblocking, to avoid szgd hanging
  
  fcntl(pipeDescriptors[0], F_SETFL,
        fcntl(pipeDescriptors[0], F_GETFL, 0) | O_NONBLOCK);

  char numberBuffer[8] = {0};
  const int PID = fork();
  if (PID < 0) {
    cerr << "szgd error: fork failed.\n";
    goto LDone;
  }

  if (PID > 0) {
    // parent process

    // We block here waiting on the child process to send
    // information regarding whether or not it has successfully launched
    // an executable. If it fails to launch an executable, the
    // szgd part of the child will do so... otherwise the launched
    // executable will do so
    numberBuffer[0] = 0;
    // Twenty second time-out for pipe. What if it takes a VERY long time
    // to start the program (like a Python program on a heavily loaded CPU)?
    if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer, 1, 20000)) {
      info << "szgd remark: got no success/failure code via pipe.\n"
	   << "  The launched executable either failed to load a shared library,"
	   << "crashed before reaching the framework init, or took a VERY long"
	   << "time to load.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
      goto LDone;
    }
    if (numberBuffer[0] == 0) {
      // The launch has failed. we will be receiving error messages from
      // the szgd side of the fork. First, we need to revoke the 
      // "message trade" since the other end of this code would be invoked in 
      // the arSZGClient of the exec'ed process.
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      // Next, we read in the error information from the pipe. We don't need
      // a very long time out since the this info should, for sure, be quickly
      // forthcoming (we aren't depending on lots of dlls being loaded, for
      // instance, instead it's all in the szg library code).
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0],
                                   numberBuffer, sizeof(int), 1000)) {
	info << "szgd remark: pipe-based handshake failed. Likely an internal library error.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
        goto LDone;
      }
      // At least one character of text but at most 10000.
      if (*(int*)numberBuffer < 0 || *(int*)numberBuffer > 10000) {
	cout << "szgd warning: ignoring bogus numberBuffer value "
	     << *(int*)numberBuffer << ". Likely an internal library error.\n";
      }
      char* textBuffer = new char[*((int*)numberBuffer)+1];
      // Again, the time out need not be very large. Essentially, we are
      // reading in the error message from the exec call in the forked process
      // here.
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0],
                                   textBuffer, *((int*)numberBuffer), 1000)) {
	info << "szgd remark: pipe-based handshake failed, text phase. Likely an internal library error.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
	delete [] textBuffer;
        goto LDone;
      }
      textBuffer[*((int*)numberBuffer)] = '\0';
      // Note how we respond to the message ourselves.
      SZGClient->messageResponse(receivedMessageID, string(textBuffer));
      delete [] textBuffer;
      goto LDone;
    }

    // numberBuffer[0] = 1 and the launch worked.
    // We do not receive the info via the pipe
    // in this case... instead the launched client
    // sends that to the "dex" directly.
    // Wait for the "message trade". A long time-out should not, in fact,
    // be necessary here. Since the szg code will be immediately communicating
    // with the szgserver, telling it that it wants the message ownership.
    int timeout = execInfo->timeoutmsec;
    if (timeout == -1) {
      timeout = 20000;
    }
    if (!SZGClient->finishMessageOwnershipTrade(match,timeout)) {
      info << "szgd remark: message ownership trade timed out.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
    }

    // close the pipes, since they are allocated each
    // time... would it be better to allocate the pipes once??
    // or would that be less secure?
    close(pipeDescriptors[0]);
    close(pipeDescriptors[1]);
    goto LDone;
  }

  // Child process
  // Set a few env vars for the child process.
  ar_setenv("SZGUSER",userName);
  ar_setenv("SZGCONTEXT",messageContext);
  ar_setenv("SZGPIPEID", pipeDescriptors[1]);
  ar_setenv("SZGTRADINGNUM", tradingNumStream.str());
  
  cout << "szgd remark: dynamic library path =\n  "
       << dynamicLibraryPath << "\n";
  ar_setenv(dynamicLibraryPathVar, dynamicLibraryPath);
  if (execInfo->executableType == "python") {
    cout << "szgd remark: python path =\n  " << pythonPath << "\n";
    ar_setenv("PYTHONPATH", pythonPath);
  }
  info << "szgd remark: running " << symbolicCommand << " on path\n"
       << execPath << ".\n";

  char** theArgs = buildUnixStyleArgList(newCommand, mangledArgList);
  // Stagger launches so the cluster's file server isn't hit so hard.
  randomDelay();
  if (execv(newCommand.c_str(), theArgs) >= 0) {
    // The child spawned okay, so remove parent's side of the fork().
    // (Actually, if the child spawned, exec() doesn't return
    // so the exit(0) isn't reached.)
    exit(0);
  }
  deleteUnixStyleArgList(theArgs);

  info << "szgd error: failed to exec \"" 
       << symbolicCommand << "\":\n\treason:  ";
  switch (errno) {
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
  case EFAULT: info << "invalid command or argv pointer:\n";
    break;
  default: info << "errno is " << errno << endl;
    break;
  }
	
  // handshake back to the parent
  const string terminalOutput = info.str();
  // we failed to launch the exe
  numberBuffer[0] = 0;
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, 1)) {
    cout << "szgd remark: failed to send failure code over pipe.\n";
  }
  *((int*)numberBuffer) = terminalOutput.length();
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, sizeof(int))) {
     cout << "szgd remark: incomplete pipe-based handshake.\n";
  }   
  if (!ar_safePipeWrite( pipeDescriptors[1],
                         terminalOutput.c_str(),
			 terminalOutput.length())) {
    cout << "szgd remark: incomplete pipe-based handshake, text stage.\n";
  }

  // Kill the child, so the orphaned process doesn't start up again 
  // on the message loop.
  exit(0);

#else // Win32

  //*******************************************************************
  // spawn a new process on Win32
  //*******************************************************************
  PROCESS_INFORMATION theInfo;
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);

  // NOTE: THIS IS REALLY ANNOYING. We pass stuff to
  // the child via environment variables.
  // On Unix this isn't so bad: we've already inside a fork and,
  // consequently, can change the environment with impunity. On Windows,
  // process creation doesn't work in the
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
  
  cout << "szgd remark: dynamic library path =\n  "
       << dynamicLibraryPath << "\n";
  ar_setenv(dynamicLibraryPathVar, dynamicLibraryPath);
  if (execInfo->executableType == "python") {
    cout << "szgd remark: python path =\n  "
         << pythonPath << "\n";
    ar_setenv("PYTHONPATH", pythonPath);
  }

  // Stagger launches so the cluster's file server isn't hit so hard.
  randomDelay();
  const string theArgs = buildWindowsStyleArgList(newCommand, mangledArgList);
  const bool fArgs = theArgs != "";
  // We need to pack these buffers... can't just use my_string.c_str().
  char* command = new char[newCommand.length()+1];
  ar_stringToBuffer(newCommand, command, newCommand.length()+1);
  char* argsBuffer = new char[theArgs.length()+1];
  ar_stringToBuffer(theArgs, argsBuffer, theArgs.length()+1);
  // The process might fail to run after being created, e.g. if a DLL is missing.
  const bool fCreated = CreateProcess(command, fArgs?argsBuffer:NULL, 
		     NULL, NULL, false,
		     NORMAL_PRIORITY_CLASS, NULL, NULL, 
		     &si, &theInfo);

  // Restore the variables before unlocking the mutex.
  ar_setenv( dynamicLibraryPathVar, oldDynamicLibraryPath);
  if (execInfo->executableType == "python")
    ar_setenv( "PYTHONPATH", oldPythonPath );
  ar_mutex_unlock(&processCreationLock);

  if (!fCreated) {
    info << "szgd warning: failed to exec \"" << command
	 << " with args " << argsBuffer
	 << "\";\n\twin32 GetLastError() returned " 
	 << GetLastError() << ".\n";
    // We will be responding directly, not the spawned process.
    SZGClient->revokeMessageOwnershipTrade(tradingKey);
    SZGClient->messageResponse(receivedMessageID, info.str());
  } else {
    // Wait for the spawnee to reach its main() and respond.
    // It could take even 20 seconds (default, unless a dex command-line arg
    // overrides it).
    int timeoutMsec = execInfo->timeoutmsec;
    if (timeoutMsec == -1)
      timeoutMsec = 20000;
    if (!SZGClient->finishMessageOwnershipTrade(match, timeoutMsec)) {
      info << "szgd warning: ownership trade timed out.\n"
	   << "  The launched executable either failed to load a dll,"
	   << "crashed before initing the framework, or took too long to load.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
    }
  }
  delete [] command;
  delete [] argsBuffer;
#endif

  goto LDone;
}

int main(int argc, char** argv) {
#ifndef AR_USE_WIN_32
  // If $DISPLAY is not 0:0, szgd will create a window on an
  // unexpected screen.  Should we test for this?

  // Kill zombie processes.
  signal(SIGCHLD,SIG_IGN);
#endif

  ar_mutex_init(&tradingNumLock);
  ar_mutex_init(&processCreationLock);

  const int argcOriginal = argc;
  // We don't need a copy of argv because it doesn't get modified.

LRetry:
  argc = argcOriginal;
  SZGClient = new arSZGClient;
  // Force the component's name, because win98 can't provide it.
  SZGClient->init(argc, argv, "szgd");
  bool fRetry = argc > 1 && !strcmp(argv[1], "-r");
  if (!*SZGClient) {
    if (fRetry) {
      delete SZGClient;
      ar_usleep(5000000);
      goto LRetry;
    }
    return 1;
  }
  
  // Only one szgd per host.
  int ownerID = -1;
  if (!SZGClient->getLock(SZGClient->getComputerName()+"/szgd", ownerID)) {
    cerr << "szgd error: szgserver thinks another copy is already running on host (pid = " 
         << ownerID << ").\n";
    // todo: if we can't communicate with that pid, then
    // assume szgserver has obsolete info, "dkill -9" that pid,
    // and start up anyways.
    return 1;
  }


  string userName, messageType, messageBody, messageContext;
  while (true) {
    const int receivedMessageID = SZGClient->receiveMessage(
      &userName, &messageType, &messageBody, &messageContext);

    if (receivedMessageID == 0) {
      // szgserver disconnected
      if (fRetry) {
        delete SZGClient;
	ar_usleep(5000000);
        goto LRetry;
      }
      exit(0);
    }
    
    if (messageType=="quit") {
      // Just in case exit() misses SZGClient's destructor.
      SZGClient->closeConnection();
      // will return(0) be gentler, yet kill the child processes too?
      exit(0);
    }
    
    if (messageType=="exec") {
      // Hack - extract timeoutMsec from end of body, if it's there.
      int timeoutMsec = -1;
      string::size_type pos = messageBody.find( "||||" );
      if (pos != string::npos) {
        pos += 4;
        string timeoutString = messageBody.substr( pos, messageBody.size()-pos );
        int temp;
        bool stat = ar_stringToIntValid( timeoutString, temp );
        cerr << "szgd remark: checking timeout info:\n";
        cerr << "     Timeout: " << temp << endl;
        messageBody.replace( pos-4, timeoutString.size()+4, "" );
        cerr << "     messageBody: " << messageBody << endl;
        if (!stat) {
          cerr << "szgd warning: failed to convert timeout string.\n";
        } else {
          timeoutMsec = temp;
        }
      }
      
      ExecutionInfo* info = new ExecutionInfo();
      // todo: all this should be args to the constructor
	info->userName = userName;
	info->messageBody = messageBody;
	info->messageContext = messageContext;
	info->receivedMessageID = receivedMessageID;
	info->timeoutmsec = timeoutMsec;
      // The long-blocking exec call gets its own thread.
      // So the various arSZGClient methods must be thread-safe.
      arThread dummy;
      dummy.beginThread(execProcess, info); // execProcess() deletes info.
    }
  }
  return 1;
}
