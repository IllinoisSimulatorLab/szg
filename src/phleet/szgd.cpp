//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"
#include "arDataUtilities.h"

#ifdef AR_USE_WIN_32
  #include <windows.h>
  #include <cctype>
  #include "arSTLalgo.h"
#else
  #include <unistd.h>
  #include <errno.h>
  #include <signal.h>
#endif

// ar_log_remark() etc. seem to fail silently in szgd.
// Despite calling ar_log().setStream(cerr) .
// Use cerr instead, for now.

// So every trading key is unique, increment tradingNum each time.
int tradingNum = 0;
arMutex tradingNumLock;
arMutex processCreationLock;
string originalWorkingDirectory;

arSZGClient* SZGClient = NULL;

std::vector< std::string > basePathsGlobal;

/*
  By convention, we assume that python apps are installed as
  "bundles" in sub-directories on a directory on SZG_PYTHON/path.
  App bundles can have arbitrary names. This function
  searches the path, piece by piece. In each piece, it examines
  subdirectories, looking for one containing the python script.
  It returns the first such found.

  python_directory1
      my_app_directory_1
	   python_script_1.py
      my_app_directory_2
	   python_script_2.py
  python_directory2
      my_app_directory_3
	   python_script_2.py

  where SZG_PYTHON/path = python_directory1;python_directory2.

  In this case, if pyfile == python_script_2.py, then 
  python_directory1/my_app_directory_2 will be returned.

  Syzygy 1.1 adds a similar launching strategy for C++ apps.
  We search sub-directories of SZG_EXEC/path for the app.
  Also, because Python sets the current working directory
  to the directory containing the script to be executed
  (and because this lets apps read data files using paths
  relative to the application's directory), szgd does the
  same for C++ apps.
*/

// print error messages to console _and_ try to return them to dex.
void doublePrintError( ostringstream& errStream, const string& msg ) {
  // to console
  cerr << "szgd error: " << msg << endl;
  const bool fTerminated = msg[msg.size()-1] == '\n';
  if (!fTerminated)
    cerr << '\n';
  if (errStream != cerr && errStream != cout) {
    // to dex
    errStream << "szgd error on host " << SZGClient->getComputerName() << ": " << msg;
    if (!fTerminated)
      errStream << '\n';
  }
}


bool comparePathToBases( const std::string& path,
                         const std::string& groupNameString,
                         ostringstream& errStream ) {
  std::string localPath( path );
#ifdef AR_USE_WIN_32
    // convert to lowercase, for case-insensitive comparison
    std::transform( localPath.begin(), localPath.end(), localPath.begin(), (int(*)(int)) tolower );
#endif
  std::vector< std::string >::const_iterator iter;
  for (iter = basePathsGlobal.begin(); iter != basePathsGlobal.end(); ++iter) {
    if (localPath.find( *iter ) == 0) {
      return true;
    }
  }
  string errMsg = "Illegal "+groupNameString+" element '"+localPath+"'\n"
         + "All SZG_EXEC and SZG_PYTHON path elements, SZG_PYTHON/lib_path,\n"
         + "     and SZG_PYTHON/executable must begin\n"
         + "     with one of the following base paths:\n"
         + "-----------------------------------------------------\n";
  for (iter = basePathsGlobal.begin(); iter != basePathsGlobal.end(); ++iter) {
    errMsg += *iter + "\n";
  }
  errMsg += "-----------------------------------------------------\n";
  doublePrintError( errStream, errMsg );
  return false;
}


bool getBasePaths( const char* const arg ) {
  arSemicolonString pathsString( arg );
  for (int i=0; i<pathsString.size(); ++i) {
    std::string pathTmp = pathsString[i];
    bool exists;
    bool isDirectory;
    if (!ar_directoryExists( pathTmp, exists, isDirectory )) {
      cerr << "szgd internal error: ar_directoryExists() failed.\n";
      return false;
    }
    if (!exists) {
#ifdef AR_USE_WIN_32
      bool isFile;
      if (!ar_fileExists( pathTmp+".exe", exists, isFile )) {
        if (!exists ) {
          cerr << "szgd error: no directory '" << pathTmp << "'\n"
               << "            or executable '" << pathTmp+".exe.\n"
          return false;
        }
        if (!isFile ) {
          cerr << "szgd error: executable '" << pathTmp+".exe" << "' is not a file.\n";
          return false;
        }
      }
#else
      cerr << "szgd error: no directory '" << pathTmp << "'.\n";
      return false;
#endif
    }
#ifdef AR_USE_WIN_32
    // convert to lowercase, for case-insensitive comparison
    std::transform( pathTmp.begin(), pathTmp.end(), pathTmp.begin(), (int(*)(int)) tolower );
#endif
    basePathsGlobal.push_back( pathTmp );
  }
  return true;
}


string getAppPath( const string& userName, const string& groupName, const string& appFile, 
    ostringstream& errStream ) {
  const string appPath = SZGClient->getAttribute( userName, "NULL", groupName, "path", "");
  string errMsg;
  if (appPath == "NULL") {
    doublePrintError( errStream, groupName+"/path not set." );
    return "NULL";
  }

  // Traverse the path. In each directory, step through all subdirectories,
  // looking for the named file. Choose the first one found.

  arSemicolonString appSearchPath(appPath);
  string actualDirectory("NULL");
  list<string> dirsToSearch;
  list<string>::iterator dirIter;
  // Construct a list of directories to search depth-first.
  for (int i=0; i<appSearchPath.size(); ++i) {
    string currPathElement = appSearchPath[i];
    if (!comparePathToBases( currPathElement, groupName+"/path", errStream )) {
      return "NULL";
    }
    bool dirExists;
    bool isDirectory;
    // If return value is false, 2nd & 3rd args are invalid.
    // If item does not exist (2nd arg == false), 3rd is invalid
    // If item exists, 3rd arg indicates whether or not it is a directory
    if (!ar_directoryExists( currPathElement, dirExists, isDirectory )) {
      errMsg = "error composing " + groupName + "/path: ar_directoryExists() internal error for directory "+currPathElement + ".\n";
      doublePrintError( errStream, errMsg );
      return "NULL";
    }

    if (!dirExists) {
      errMsg = "error composing "+groupName+"/path:\n"
                 + "  directory " +currPathElement+" does not exist.";
      doublePrintError( errStream, errMsg );
      return "NULL";
    }

    if (!isDirectory) {
      errMsg = "error composing "+groupName+"/path:\n"
                 + "  "+currPathElement+" exists, but is not a directory.";
      doublePrintError( errStream, errMsg );
      return "NULL";
    }

    dirsToSearch.push_back( currPathElement );
    list<string> contents = ar_listDirectory( currPathElement );
    for (list<string>::iterator dirIter = contents.begin(); dirIter != contents.end(); ++dirIter) {
      string itemPath = *dirIter;
      if (ar_isDirectory( itemPath.c_str() )) {
	dirsToSearch.push_back( itemPath );
      }
    }
  }
  cout << "szgd remark: scanning for " << appFile << ":\n";
  for (dirIter=dirsToSearch.begin(); dirIter != dirsToSearch.end(); ++dirIter) {
    cout << *dirIter << endl;
    string potentialFile(*dirIter);
    ar_pathAddSlash(potentialFile);
    potentialFile += appFile;
    // If return value is false, 2nd & 3rd args are invalid.
    // If item does not exist (2nd arg == false), 3rd is invalid
    // If item exists, 3rd arg indicates whether or not it is a regular file
    bool fileExists;
    bool isFile;
    if (!ar_fileExists( potentialFile, fileExists, isFile )) {
      errMsg = "error scanning "+groupName+"/path:\n"
                + "ar_FileExists() failed for file "+potentialFile
                + ".\n  This does not mean that the file "
                + "does not exist;\n  it means that a system error occurred while "
                + "checking its existence.\n";
      doublePrintError( errStream, errMsg );
      return "NULL";
    } else {
      if (fileExists && isFile) {
        cout << "szgd remark: found " << potentialFile << endl;
        actualDirectory = *dirIter;
        break;
      }
    }
//    FILE* filePtr = ar_fileOpen(potentialFile, "", "", "r");
//    if (filePtr) {
//      ar_fileClose(filePtr);
//      actualDirectory = *iter;
//      break;
//    }
//    }
    if (actualDirectory != "NULL") {
      break;
    }
  }

  if (actualDirectory == "NULL") {
    errMsg = "No file '"+appFile+"' on user "+userName+"'s "+groupName
           +"/path '"+appPath+"'.\n";
    doublePrintError( errStream, errMsg );
  } else {
    cout << "szgd remark: app dir for " << userName << "/" << groupName
         << "/path is '" << actualDirectory << "'.\n";
  }
  return actualDirectory;
}

// Little class to pass data to exec threads.
class ExecutionInfo{
public:
  ExecutionInfo(){}
  ~ExecutionInfo(){}

  int receivedMessageID;
  int timeoutmsec;
  string userName;
  string messageContext;
  string messageBody;

  string executableType;
  string appDirPath;
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
//       the exename (i.e. on unix argv[2]... argv[argc-1])
//    2. for python, this will be the full exename plus the args.
string buildFunctionArgs(ExecutionInfo* execInfo,
                       string& execPath,
                       string& symbolicCommand,
                       string& command,
                       list<string>& args){
  const string userName(execInfo->userName);
  const string argString(execInfo->messageBody);

  // Tokenize the argString.
  // "command" gets the first token, the list "args" gets the rest.
  // command is our candidate for the exe, either native or python.
  args.clear();
  arDelimitedString tmpArgs(argString, ' ');
  if (tmpArgs.size() < 1) {
    return "szgd error: no arguments.\n";
  }

  command = tmpArgs[0];
  int i;
  for (i=1; i<tmpArgs.size(); ++i) {
    // Skip extra whitespace.
    if (tmpArgs[i].length() > 0) {
      args.push_back(tmpArgs[i]);
    }
  }

  ostringstream errStream;
  execPath = SZGClient->getAttribute(userName, "NULL", "SZG_EXEC", "path", "");
  if (execPath == "NULL"){
    errStream << "szgd warning: SZG_EXEC/path not set.\n";
  }

  // Determine if the argString's first token is in the exec path:
  //   a. Strip the token (remove .EXE).
  //   b. If this not a .py python script, under win32 append .EXE.
  //   c. Find the file on the path.
#ifdef AR_USE_WIN_32
  const string commandRaw(command);
#endif
  command = ar_stripExeName(command);
#ifdef AR_USE_WIN_32
  const bool fHadEXE = command != commandRaw;
#endif
  symbolicCommand = command;
  string fileName;
  string failedAppPath;
  if (command.length() > 3 && command.substr(command.length()-3, 3) == ".py") {
    execInfo->executableType = "python";
    fileName = command;
    execInfo->appDirPath = getAppPath( userName, "SZG_PYTHON", fileName, errStream );
    if (execInfo->appDirPath == "NULL") {
      return errStream.str();
    }
    command = ar_fileFind( fileName, "", execInfo->appDirPath);
    if (command == "NULL") {
      errStream << "szgd error: no file '" << fileName <<
        "' on SZG_PYTHON/path '" << execInfo->appDirPath << "'.\n";
      cerr << errStream.str();
      return errStream.str();
    }
  }
  else {
    execInfo->executableType = "native";
#ifdef AR_USE_WIN_32
    command += ".EXE";
#endif
    fileName = command;
    execInfo->appDirPath = getAppPath( userName, "SZG_EXEC", fileName, errStream );
    if (execInfo->appDirPath == "NULL") {
      return errStream.str();
    }
    command = ar_fileFind( fileName, "", execInfo->appDirPath);
    if (command == "NULL") {
      errStream << "szgd error: no file '" << fileName
           << "' on SZG_EXEC/path '" << execInfo->appDirPath << "'.\n";
LAbort:
#ifdef AR_USE_WIN_32
      if (fHadEXE)
        errStream << "szgd warning: don't append .exe;  Windows does that for you.\n";
#endif
      cerr << errStream.str();
      return errStream.str();
    }
  }

  // Found the file.  What kind of exe is it?
  if (execInfo->executableType == "native") {
    // Nothing to do
  }
  else if (execInfo->executableType == "python") {
    // Prepend the original command (the script name) to the args list.
    args.push_front(command);

    // Change the command to:
    //   the Syzygy database variable SZG_PYTHON/executable; or if not set,
    //   the environment variable SZG_PYEXE; or if not set,
    //   "python".
    const string szgPyExe = SZGClient->getAttribute(userName, "NULL", "SZG_PYTHON", "executable", "");
    const string pyExeString = (szgPyExe!="NULL") ? szgPyExe : ar_getenv("SZG_PYTHON_executable");
    if (pyExeString != "NULL" && pyExeString != "") {
      if (!comparePathToBases( szgPyExe, "SZG_PYTHON/executable", errStream )) {
        goto LAbort;
      }
      // Handle bar-delimited cmdline args in SZG_PYEXE or SZG_PYTHON/executable
      arDelimitedString pyArgsString( pyExeString, '|' );
      command = pyArgsString[0];
#ifdef AR_USE_WIN_32
      command += ".EXE";
#endif
      for (int ind=1; ind < pyArgsString.size(); ++ind) {
        args.push_front( pyArgsString[ind] );
      }
      arPathString pyExePathString( command );
      if (pyExePathString.size() != 1) {  // absolute path has not been specified.
        fileName = command;
        command = ar_fileFind( fileName, "", execPath );
        if (command == "NULL") {
          errStream << "szgd error: no python exe '" << fileName
               << "' on SZG_EXEC/path " << execPath << ".\n";
          goto LAbort;
        }
      }
    } else {
      command = "python";
#ifdef AR_USE_WIN_32
      command += ".EXE";
#endif
      fileName = command;
      command = ar_fileFind( fileName, "", execPath );
      if (command == "NULL") {
        errStream << "szgd error: no python exe '" << fileName
             << "' on SZG_EXEC/path " << execPath << ".\n";
        goto LAbort;
      }
      if (!comparePathToBases( command, "SZG_EXEC/path for Python", errStream )) {
        goto LAbort;
      }
    }
    cerr << "Python command: " << command << endl;
    const string szgPythonLibPath =
               SZGClient->getAttribute(userName, "NULL", "SZG_PYTHON", "lib_path", "");
    if (szgPythonLibPath != "NULL") {
      arSemicolonString pyLibs( szgPythonLibPath );
      for (int q=0; q<pyLibs.size(); ++q) {
        if (!comparePathToBases( pyLibs[q], "SZG_PYTHON/lib_path", errStream )) {
          goto LAbort;
        }
      }
    }

  } else {
    errStream << "szgd error: unexpected exe type '" << execInfo->executableType << "'.\n";
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
  ExecutionInfo* execInfo = (ExecutionInfo*)i;
  const string userName(execInfo->userName);
  const string messageContext(execInfo->messageContext);
  const string messageBody(execInfo->messageBody);
  int receivedMessageID = execInfo->receivedMessageID; 

  ostringstream info;
  info << endl << "The program failed to launch!" << endl;
  // Respond to the message ourselves if the exe doesn't launch.
  info << "user=" << userName
       << ", context=" << messageContext
       << ", computer=" << SZGClient->getComputerName() << endl << endl;

  string execPath;
  string symbolicCommand;
  string newCommand;
  list<string> mangledArgList;
  const string stats = buildFunctionArgs(
    execInfo, execPath, symbolicCommand, newCommand, mangledArgList);
  if (stats != "OK") {
    info << stats;
    SZGClient->messageResponse(receivedMessageID, info.str());
LDone:
    delete execInfo;
    if (ar_setWorkingDirectory( originalWorkingDirectory )) {
      cerr << "szgd remark: post-launch current directory is '"
           << originalWorkingDirectory << "'.\n";
    } else {
      cerr << "szgd error: post-launch failed to set current directory to "
           << originalWorkingDirectory << ".\n";
    }
    return;
  }

  // We've got something to execute.
  // Invoke a message trade so the executee can respond.
  // Increment the "trading number" that makes this request unique.
  // Since many threads can execute this simultaneously, use a lock.
  ar_mutex_lock(&tradingNumLock);
    stringstream tradingNumStream;
    tradingNumStream << ++tradingNum;
  ar_mutex_unlock(&tradingNumLock);
  const string tradingKey(SZGClient->getComputerName() + "/" 
                          + tradingNumStream.str() + "/"
	                  + symbolicCommand);
  cerr << "szgd remark: trading key = " << tradingKey << endl;
  int match = SZGClient->startMessageOwnershipTrade(receivedMessageID, tradingKey);

  // Two dynamic search paths (as embodied in environment variables) need
  // to be altered before launching the new executable (and in the case of
  // Win32 altered back after launch). The info to do the alterations comes
  // from szg database variables. Get this info here since
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
  string appPath = ar_exePath(newCommand);
  if (appPath != "") {
    dynamicLibraryPath = appPath;
  }
  // If parent directory is in SZG_EXEC/path, append it.
  if (szgExecPath != "NULL") {
    int j;
    arPathString appPathString( appPath );
    arPathString parDirString;
    if (appPathString.size() > 1) {
      for (j=0; j<(appPathString.size()-1); ++j) {
        parDirString /= appPathString[j];
      }
      arSemicolonString szgExecPathString( szgExecPath );
      for (j=0; j<szgExecPathString.size(); ++j) {
        if (parDirString == szgExecPathString[j]) {
          dynamicLibraryPath += ";" + parDirString;
          break;
        }
      }
    }
  }
  if (nativeLibPath != "NULL") {
    dynamicLibraryPath += ";" + nativeLibPath;
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

  // Set the current directory to that containing the app
  if (!ar_setWorkingDirectory( execInfo->appDirPath )) {
    cerr << "szgd error: pre-launch failed to set current directory to '"
         << execInfo->appDirPath << "'.\n";
  } else {
    cerr << "szgd remark: pre-launch current directory is '"
         << execInfo->appDirPath << "'.\n";
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

    // Block until the child reports if it launched an executable or not.
    // If launch fails, the szgd part of the child reports.
    // If launch succeeds, the launchee reports.
    numberBuffer[0] = 0;
    // Twenty second timeout for pipe. What if it takes a VERY long time
    // to start the program (like a Python program on a heavily loaded CPU)?
    if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer, 1, 20000)) {
      info << "szgd remark: launchee returned no success/failure code\n"
	   << "  (it failed to load a dll, crashed before framework init, or took too long to load).\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
      goto LDone;
    }
    if (numberBuffer[0] == 0) {
      // The launch failed. we will be receiving error messages from
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
        cerr << "szgd warning: ignoring bogus numberBuffer value "
	           << *(int*)numberBuffer << ". Likely an internal library error.\n";
      }
      char* textBuffer = new char[*((int*)numberBuffer)+1];
      // The timeout can be small.
      // Read the error message from the exec call in the forked process.
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0], textBuffer, *((int*)numberBuffer), 1000)) {
        info << "szgd remark: pipe-based handshake failed, text phase. Likely an internal library error.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
        delete [] textBuffer;
        goto LDone;
      }

      textBuffer[*((int*)numberBuffer)] = '\0';
      // Respond to the message ourselves.
      SZGClient->messageResponse(receivedMessageID, string(textBuffer));
      delete [] textBuffer;
      goto LDone;
    }

    // numberBuffer[0] = 1 and the launch worked.
    // We do not receive the info via the pipe;
    // the launched client sends that to the "dex" directly.
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
  
  cerr << "szgd remark: dynamic library path =\n  "
       << dynamicLibraryPath << "\n";
  ar_setenv(dynamicLibraryPathVar, dynamicLibraryPath);
  if (execInfo->executableType == "python") {
    cerr << "szgd remark: python path =\n  " << pythonPath << "\n";
    ar_setenv("PYTHONPATH", pythonPath);
  }
  info << "szgd remark: running " << symbolicCommand << " on path\n"
       << execPath << ".\n";

  char** theArgs = buildUnixStyleArgList(newCommand, mangledArgList);
  // Stagger launches so the cluster's file server isn't hit so hard.
  cerr << "szgd remark: command = " << newCommand << endl
       << "             args    = " << theArgs << endl;
  randomDelay();
  if (execv(newCommand.c_str(), theArgs) >= 0) {
    // The child spawned okay, so remove parent's side of the fork().
    // (Actually, if the child spawned, exec() doesn't return
    // so the exit(0) isn't reached.)
    exit(0);
  }
  deleteUnixStyleArgList(theArgs);

  info << "szgd error: failed to exec '" << symbolicCommand << "': ";
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
  default: info << "errno = " << errno << endl;
    break;
  }
	
  // Handshake back to the parent.
  const string terminalOutput = info.str();
  // Exe failed to launch.
  numberBuffer[0] = 0;
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, 1)) {
    cerr << "szgd remark: failed to send failure code over pipe.\n";
  }
  *((int*)numberBuffer) = terminalOutput.length();
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, sizeof(int))) {
     cerr << "szgd remark: incomplete pipe-based handshake.\n";
  }   
  if (!ar_safePipeWrite( pipeDescriptors[1],
                         terminalOutput.c_str(),
			 terminalOutput.length())) {
    cerr << "szgd remark: incomplete pipe-based handshake, text stage.\n";
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
  
  cerr << "szgd remark: dynamic library path =\n  "
       << dynamicLibraryPath << "\n";
  ar_setenv(dynamicLibraryPathVar, dynamicLibraryPath);
  if (execInfo->executableType == "python") {
    cerr << "szgd remark: python path =\n  "
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
  cerr << "szgd remark: command = " << command << endl
       << "             args    = " << argsBuffer << endl;
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
           << "  Launchee failed to load a dll, crashed before framework init, or took too long to load.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
    }
  }
  delete [] command;
  delete [] argsBuffer;
#endif

  goto LDone;
}


void printUsage() {
  cerr << "Usage: szgd <basePaths> [-r]\n"
       << "  basePaths is a semicolon-delimited list of paths that all\n"
       << "    SZG_EXEC and SZG_PYTHON path elements, SZG_PYTHON/lib_path, and\n"
       << "    SZG_PYTHON/executable must be equal to or beneath in the directory tree.\n"
       << "    These paths may be directories or executables (if executables,\n"
       << "    OMIT '.exe' on Windows).\n"
       << "  -r: repeatedly try to reconnect to the Syzygy server on failure.\n";
}

int main(int argc, char** argv) {
#ifndef AR_USE_WIN_32
  // If $DISPLAY is not 0:0, szgd creates a window on an unexpected screen.
  // Should we test for this?

  // Kill zombie processes.
  signal(SIGCHLD,SIG_IGN);
#endif

  ar_mutex_init(&tradingNumLock);
  ar_mutex_init(&processCreationLock);
#ifdef UNUSED
  // dex didn't spawn an szgd - that's pointless(?).
  ar_log().setStream(cerr);
#endif

  if (argc < 2) {
    printUsage();
    return 1;
  }
  
  if (!getBasePaths( argv[1] )) {
    printUsage();
    return 1;
  }

  const int argcOriginal = argc;
  // We don't need an original copy of argv, because it doesn't get modified.

LRetry:
  argc = argcOriginal;
  SZGClient = new arSZGClient;
  // Force the component's name, because win98 can't provide it.
  const bool fInit = SZGClient->init(argc, argv, "szgd");
  bool fRetry = argc > 2 && !strcmp(argv[2], "-r");
  if (!*SZGClient) {
    if (fRetry) {
      delete SZGClient;
      ar_usleep(5000000);
      goto LRetry;
    }
    return SZGClient->failStandalone(fInit);
  }

  ar_getWorkingDirectory( originalWorkingDirectory );

  // Only one instance per host.
  int ownerID = -1;
  if (!SZGClient->getLock(SZGClient->getComputerName() + "/szgd", ownerID)) {
    cerr << "szgd error: another copy is already running (pid = " << ownerID << ").\n";
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
        const bool ok = ar_stringToIntValid( timeoutString, temp );
        messageBody.replace( pos-4, timeoutString.size()+4, "" );
        if (ok) {
	  cout << "szgd remark: timeout is " << temp <<
	    " msec, msg body is '" << messageBody << "'.\n";
          timeoutMsec = temp;
        } else {
          cerr << "szgd warning: ignoring invalid timeout string '" << timeoutString << "'.\n";
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
      arThread dummy(execProcess, info); // execProcess() deletes info.
    }
  }
  return 1;
}
