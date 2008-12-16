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
  arLock lockSpawn;
#else
  #include <unistd.h>
  #include <errno.h>
  #include <signal.h>
#endif

arIntAtom tradingNum(-1);
string originalWorkingDirectory;
arSZGClient* SZGClient = NULL;
std::vector< std::string > basePathsGlobal;

arIntAtom fConnect(0);

/*****************************************************************************/

// Print warnings to console AND return them to dex.
void warnTwice( ostream& errStream, const string& msg ) {
  // to console
  ar_log_error() << msg << "\n";
  const bool fTerminated = msg[msg.size()-1] == '\n';
  if (!fTerminated)
    ar_log_error() << '\n';

  if (errStream != cerr && errStream != cout) {
    // to dex
    errStream << "szgd on " << SZGClient->getComputerName() << ": " << msg;
    if (!fTerminated)
      errStream << '\n';
  }
}

/*****************************************************************************/
/*
  By convention, we assume that python apps are installed as
  "bundles" in sub-directories on a directory on SZG_PYTHON/path.
  App bundles can have arbitrary names. For each member of the
  path, this function looks for a subdirectory containing the
  python script.  It returns the first such found.

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
  We search sub-directories of SZG_EXEC/path for the app.  Also,
  because Python sets the current working directory to the directory
  containing the script to be executed (and because this lets
  apps read data files using paths relative to the application's
  directory), szgd does the same for C++ apps.
*/

/*****************************************************************************/
// All SZG_<foo>/path variables must begin with one of the items
// passed to szg in the base_paths command-line argument. This
// function checks a path for this condition.
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
  warnTwice( errStream, errMsg );
  return false;
}
/*****************************************************************************/


/*****************************************************************************/
// Un-pack the base paths commandline argument and verify that each
// element of it actually exists.
bool getBasePaths( const char* const arg ) {
  arSemicolonString pathsString( arg );
  for (int i=0; i<pathsString.size(); ++i) {
    std::string pathTmp(pathsString[i]);
    bool dirExists = false;
    bool isDir = false;
    if (!ar_directoryExists( pathTmp, dirExists, isDir )) {
      ar_log_error() << "ar_directoryExists() failed.\n";
      return false;
    }
    if (!dirExists) {
#ifdef AR_USE_WIN_32
      bool isFile = false;
      if (!ar_fileExists( pathTmp+".exe", dirExists, isFile )) {
        if (!dirExists) {
          ar_log_error() << "no directory '" << pathTmp << "' or executable '" << pathTmp+".exe.\n";
          return false;
        }
        if (!isFile) {
          ar_log_error() << "executable '" << pathTmp << ".exe' is not a file.\n";
          return false;
        }
      }
#else
      ar_log_error() << "no directory '" << pathTmp << "'.\n";
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
/*****************************************************************************/


/*****************************************************************************/
// Find the 'program', i.e. the executable name passed to dex. This could be either
// an executable or a .py file. The absolute path is returned.
string getAppPath( const string& userName, const string& groupName, const string& appFile,
    ostringstream& errStream ) {
  const string appPath = SZGClient->getAttribute( userName, "NULL", groupName, "path", "");
  if (appPath == "NULL") {
    warnTwice( errStream, "no " + groupName + "/path." );
    return "NULL";
  }

  // Traverse the path. In each directory, step through all subdirectories,
  // looking for the named file. Choose the first one found.

  arSemicolonString appSearchPath(appPath);
  string actualDirectory("NULL");
  list<string> dirsToSearch;
  list<string>::const_iterator dirIter;
  // Depth-first search a list of directories.
  for (int i=0; i<appSearchPath.size(); ++i) {
    string dir(appSearchPath[i]);
    if (!comparePathToBases( dir, groupName+"/path", errStream )) {
      return "NULL";
    }

    bool dirExists = false;
    bool isDir = false;

    // If return value is false, 2nd & 3rd args are invalid.
    // If item does not exist (2nd arg == false), 3rd is invalid
    // If item exists, 3rd arg indicates whether or not it is a directory
    if (!ar_directoryExists( dir, dirExists, isDir )) {
      warnTwice( errStream, "internal error composing " + groupName +
        "/path: ar_directoryExists() for directory " + dir + ".\n");
      return "NULL";
    }

    if (!dirExists) {
      warnTwice( errStream, "composing " + groupName + "/path: nonexistent " + dir + ".\n");
      continue;
    }

    if (!isDir) {
      warnTwice( errStream, "composing " + groupName + "/path: nondirectory " + dir + ".\n");
      continue;
    }

    dirsToSearch.push_back( dir );
    list<string> contents = ar_listDirectory( dir );
    for (dirIter = contents.begin(); dirIter != contents.end(); ++dirIter) {
      string itemPath = *dirIter;
      if (ar_isDirectory( itemPath.c_str() )) {
        dirsToSearch.push_back( itemPath );
      }
    }
  }
  ar_log_remark() << "scanning for " << appFile << ":\n";
  for (dirIter=dirsToSearch.begin(); dirIter != dirsToSearch.end(); ++dirIter) {
    ar_log_remark() << *dirIter << "\n";
    string potentialFile(*dirIter);
    ar_pathAddSlash(potentialFile);
    potentialFile += appFile;
    // If return value is false, 2nd & 3rd args are invalid.
    // If item does not exist (2nd arg == false), 3rd is invalid
    // If item exists, 3rd arg indicates whether or not it is a regular file
    bool fileExists;
    bool isFile;
    if (!ar_fileExists( potentialFile, fileExists, isFile )) {
      warnTwice( errStream, "while scanning " + groupName +
        "/path: ar_FileExists() problem with " + potentialFile + ".\n" );
      return "NULL";
    }
    if (fileExists && isFile) {
      ar_log_remark() << "found " << potentialFile << "\n";
      actualDirectory = *dirIter;
      break;
    }
    if (actualDirectory != "NULL") {
      break;
    }
  }

  if (actualDirectory == "NULL") {
    warnTwice( errStream, "No file '" + appFile + "' on user " + userName + "'s " +
      groupName + "/path '" + appPath+"'.\n");
  } else {
    ar_log_remark() << "app dir for " << userName << "/" << groupName <<
      "/path is '" << actualDirectory << "'.\n";
  }
  return actualDirectory;
}
/*****************************************************************************/



/*****************************************************************************/
// Helper class to pass data to exec threads.

enum {
  formatNative=0,
  formatPython,
  formatInvalid
};

class ExecInfo {
 public:
  ExecInfo(const string& u, const string& mB, const string& mC, int rMID, int tM) :
    receivedMessageID(rMID),
    timeoutmsec(tM),
    userName(u),
    messageBody(mB),
    messageContext(mC),
    _format(formatInvalid)
    {}

  int receivedMessageID;
  int timeoutmsec;
  string userName;
  string messageBody;
  string messageContext;
  string appDirPath;
  bool fNative() const
    { return _format == formatNative; }
  bool fPython() const
    { return _format == formatPython; }
  void setFormat(int f)
    { _format = f; }
  const char* formatname() const
    { return _formatnames[_format]; }
  int format() const
    { return _format; }
 private:
  int _format;
  static const char* const _formatnames[formatInvalid+1];
};

const char* const ExecInfo::_formatnames[formatInvalid+1] =
    { "native", "python", "invalid" };

/*****************************************************************************/


/*****************************************************************************/
string argsAsList(const list<string>& args) {
  string s("(");
  for (list<string>::const_iterator iter = args.begin();
       iter != args.end(); ++iter) {
    if (iter != args.begin())
      s += ", ";
    s += *iter;
  }
  return s + ")\n";
}
/*****************************************************************************/


/*****************************************************************************/
// 1. Given the user and arg string, from szgserver get the user's exe path.
// 2. From the arg string and the exe path, find the file to execute.
// 3. Determine "symbolicCommand", "command" and "args", returned by reference.
//
// NOTE: the only inputs are "userName" and "argSring", with "userName" being the
// Syzygy user as determined by the message context of the "dex" message and
// "argString" being the body of the "dex" message.
//
// "execPath" is user's SZG_EXEC/path on the host running us.
//
// "symbolicCommand" is e.g. "atlantis", from "g:\foo\bar\atlantis.exe".
//
// "command" is "python", or if native, "g:\foo\bar\atlantis.exe".
//
// "args" is a list of strings, possibly mangled.
//   If python: the full exe path + args.
//   If native: everything after the exe (argv[2 ... argc-1]).
string buildFunctionArgs(ExecInfo* execInfo,
                       string& execPath,
                       string& symbolicCommand,
                       string& command,
                       list<string>& args) {
  const string userName(execInfo->userName);
  const string argString(execInfo->messageBody);

  // Tokenize argString.
  args.clear();
  arDelimitedString tmpArgs(argString, ' ');
  if (tmpArgs.size() < 1) {
    return "no arguments.\n";
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
  if (execPath == "NULL") {
    errStream << "no SZG_EXEC/path.\n";
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
    execInfo->setFormat(formatPython);
    fileName = command;
    execInfo->appDirPath = getAppPath( userName, "SZG_PYTHON", fileName, errStream );
    if (execInfo->appDirPath == "NULL") {
      return errStream.str();
    }
    command = ar_fileFind( fileName, "", execInfo->appDirPath);
    if (command == "NULL") {
      errStream << "no file '" << fileName << "' on SZG_PYTHON/path '" <<
        execInfo->appDirPath << "'.\n";
      goto LNolaunch;
    }
  }
  else {
    execInfo->setFormat(formatNative);
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
      errStream << "no file '" << fileName << "' on SZG_EXEC/path '" <<
        execInfo->appDirPath << "'.\n";
LAbort:
#ifdef AR_USE_WIN_32
      if (fHadEXE)
        errStream << "Don't append .exe;  Windows does that for you.\n";
#endif
LNolaunch:
      ar_log_error() << errStream.str();
      return errStream.str();
    }
  }

  // Found the file.
  const int format = execInfo->format();
  switch (format) {

  case formatInvalid:
  default:
    errStream << "unexpected exe type " << format << ".\n";
    return errStream.str();

  case formatNative:
    // Nothing to do
    break;

  case formatPython:
    // Prepend the original command (the script name) to the args list.
    args.push_front(command);

    // Change the command to:
    //   the Syzygy database variable SZG_PYTHON/executable;
    //   or the environment variable SZG_PYEXE;
    //   or "python".
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
      arSemicolonString pyExePathString( command );
      if (pyExePathString.size() != 1) {  // absolute path has not been specified.
        fileName = command;
        command = ar_fileFind( fileName, "", execPath );
        if (command == "NULL") {
          errStream << "no python executable '" << fileName
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
        errStream << "no python exe '" << fileName
                  << "' on SZG_EXEC/path " << execPath << ".\n";
        goto LAbort;
      }
      if (!comparePathToBases( command, "SZG_EXEC/path for Python", errStream )) {
        goto LAbort;
      }
    }
    ar_log_remark() << "Python command: " << command << "\n";
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
    break;
  }

  ar_log_remark()
       << "  user = " << userName << "\n"
       << "  exe  = " << execInfo->formatname() << " " << command << "\n"
       << "  args = " << argsAsList(args);
  return string("OK");
}
/*****************************************************************************/

void bufFromString(char*& buf, const string& s) {
  const int cch = s.length() + 1;
  ar_stringToBuffer(s, buf = new char[cch], cch);
}

// Build an "argv".
char** buildUnixStyleArgList(const string& command, const list<string>& args) {
  const int argc = 1 + args.size();
  char** argv = new char*[argc+1];

  // The arg-less command.
  bufFromString(argv[0], command);

  // The args.
  int index = 1;
  for (list<string>::const_iterator i=args.begin(); i!=args.end(); ++i)
    bufFromString(argv[index++], *i);

  // Null-terminate argv.
  argv[argc] = NULL;

  return argv;
}

void deleteUnixStyleArgList(char** argv) {
  for (int i = 0; argv[i]; ++i)
    delete argv[i];
}

string buildWindowsStyleArgList(const string& command, list<string>& args) {
  if (args.empty())
    return command;

  string s(command);
  for (list<string>::const_iterator i = args.begin(); i != args.end(); ++i)
    s += " " + *i;
  return s;
}

// To reduce load on smb fileserver when many hosts launch an exe at once.
void randomDelay() {
  const ar_timeval time1 = ar_time();
  ar_usleep(30000 * abs(time1.usec % 6));
}

static void TweakPath(string& path) {
  // Point slashes in the right direction.
  ar_fixPathDelimiter(path);

#ifndef AR_USE_WIN_32
  // Change windows szg path's ';' delimiter to unix's ':'.
  unsigned pos;
  while ((pos = path.find(";")) != string::npos) {
    path.replace( pos, 1, ":" );
  }
#endif
}


/*****************************************************************************/
void execProcess(void* i) {
  ExecInfo* execInfo = (ExecInfo*)i;
  const string& userName = execInfo->userName;
  const string& messageContext = execInfo->messageContext;
  const int receivedMessageID = execInfo->receivedMessageID;

  // Respond to the message ourselves, if the exe doesn't launch.
  ostringstream info;
  info << "szgd on " << SZGClient->getComputerName() << " failed to launch exe.\n"
       << SZGClient->launchinfo(userName, messageContext);

  string execPath;
  string symbolicCommand;
  string newCommand;
  list<string> argsMangled;
  const string stats = buildFunctionArgs(
    execInfo, execPath, symbolicCommand, newCommand, argsMangled);
  if (stats != "OK") {
    info << stats;
    SZGClient->messageResponse(receivedMessageID, info.str());
LDone:
    delete execInfo;
    return;
  }

  // We have something to execute.
  // Invoke a message trade so the executee can respond.
  // Increment the "trading number" that makes this request unique.
  // Since many threads can execute this simultaneously, use a lock.
  // So every trading key is unique, increment tradingNum each time.
  const string tradingStr(ar_intToString(++tradingNum));
  const string tradingKey(
    SZGClient->getComputerName() + "/" + tradingStr + "/" + symbolicCommand);
  ar_log_debug() << "trading key = " << tradingKey << ".\n";
  int match = SZGClient->startMessageOwnershipTrade(receivedMessageID, tradingKey);

  // Before launching, alter dynamic search paths (stored in env vars).
  // In win32, restore them after launch.
  // Get info for the alterations (from database variables) now,
  // because arSZGClient can't be used after the fork.

  // The library search path is modified for each user, so
  // multiple users (with different ways of arranging and
  // loading dynamic libraries) can use the same szgd.
  // Libraries are searched for in this order:
  //   1. Directory on the exec path containing the exe.
  //   2. SZG_NATIVELIB/path
  //   3. SZG_EXEC/path
  //   4. The native DLL search path (i.e. LD_LIBRARY_PATH or
  //      DYLD_LIBRARY_PATH or LD_LIBRARYN32_PATH or PATH)
  //      as held by the user running szgd.
  // This path is altered for both "native" AND "python" exes.

  // The python module search path is modified for each user,
  // for similar reasons. By default, python seems to prepend
  // the directory containing the .py file.
  //   1. SZG_PYTHON/path (put python modules in the top level of
  //      your "application bundle directory")
  //   2. SZG_PYTHON/lib_path (for python modules
  //      but not "application bundles")
  //   3. SZG_EXEC/path (PySZG.py and PySZG.so, or .dll,
  //      must be in the same directory and on the PYTHONPATH).
  //   4. PYTHONPATH, as held by the user running szgd.

  const string envDLLPath =
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

  const string szgExecPath = SZGClient->getAttribute(userName, "NULL", "SZG_EXEC", "path", "");
  const string nativeLibPath = SZGClient->getAttribute(userName, "NULL", "SZG_NATIVELIB", "path", "");

  // Construct the new dynamic library path.
#ifdef AR_USE_WIN_32
  // guard around ar_getenv/ar_setenv/ar_setenv(DLLPathPrev),
  // lest, when spawning several things at once,
  // one spawn thread get another's already-ar_setenv'd path.
  lockSpawn.lock("szgd execProcess");
#endif
  const string DLLPathPrev = ar_getenv(envDLLPath.c_str());
  const string appPath = ar_exePath(newCommand);
  string DLLPath;
  if (appPath != "") {
    DLLPath = appPath;
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
          DLLPath += ";" + parDirString;
          break;
        }
      }
    }
  }
  if (nativeLibPath != "NULL") {
    DLLPath += ";" + nativeLibPath;
  }
  if (DLLPathPrev != "" && DLLPathPrev != "NULL") {
    DLLPath += ";" + DLLPathPrev;
  }
  TweakPath(DLLPath);

  // Deal with python
  string pythonPath;
  string oldPythonPath;
  if (execInfo->fPython()) {
    oldPythonPath = ar_getenv( "PYTHONPATH" );
    // If no SZG_PYTHON/path, warning was already displayed.
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

  // Spawn a new process on Linux, OS X, Irix).

  // Set the current directory to that containing the app
  if (!ar_setWorkingDirectory( execInfo->appDirPath )) {
    ar_log_error() << "pre-launch failed to cd to '" <<
         execInfo->appDirPath << "'.\n";
  } else {
    ar_log_remark() << "pre-launch dir for " << execInfo->messageBody
         << "\n    is '" << execInfo->appDirPath << "'.\n";
  }

  int pipeDescriptors[2] = {0};
  if (pipe(pipeDescriptors) < 0) {
    ar_log_error() << "failed to create pipe.\n";
    goto LDone;
  }

  // Make the read pipe nonblocking, to avoid szgd hanging
  fcntl(pipeDescriptors[0], F_SETFL,
    fcntl(pipeDescriptors[0], F_GETFL, 0) | O_NONBLOCK);

  char numberBuffer[8] = {0};
  const int PID = fork();
  if (PID < 0) {
    ar_log_error() << "failed to fork.\n";
    goto LDone;
  }

  if (PID > 0) {
    // parent process

    // Block until the child reports if it launched an executable or not.
    // If launch fails, the szgd part of the child reports.
    // If launch succeeds, the launchee reports.
    numberBuffer[0] = 0;
    // Twenty second timeout, e.g. for Python on a busy CPU.
    if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer, 1, 20000)) {
      info << "szgd launchee returned no success code\n"
           << "  (it failed to load a dll, crashed before framework init, or took too long to load).\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
      goto LDone;
    }
    if (numberBuffer[0] == 0) {
      // The launch failed. we will get error messages from
      // the szgd side of the fork.
      // Revoke the "message trade" since the other end of this code
      // would be invoked in the arSZGClient of the exec'ed process.
      SZGClient->revokeMessageOwnershipTrade(tradingKey);

      // Read error info from the pipe.  Short timeout's ok
      // since this info doesn't depend on slow dll-loading, etc.
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0], numberBuffer, sizeof(int), 1000)) {
        info << "szgd internal error: pipe-based handshake failed.\n";
        SZGClient->messageResponse(receivedMessageID, info.str());
        goto LDone;
      }
      // At least one character of text but at most 10000.
      if (*(int*)numberBuffer < 0 || *(int*)numberBuffer > 10000) {
        ar_log_error() << "internal error: ignoring bogus numberBuffer value " <<
          *(int*)numberBuffer << ".\n";
      }
      char* textBuffer = new char[*((int*)numberBuffer)+1];
      // The timeout can be small.
      // Read the error message from the exec call in the forked process.
      if (!ar_safePipeReadNonBlock(pipeDescriptors[0], textBuffer, *((int*)numberBuffer), 1000)) {
        info << "szgd internal error: pipe-based handshake failed, text phase.\n";
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
      timeout = 30000;
    }
    if (!SZGClient->finishMessageOwnershipTrade(match, timeout)) {
      info << "szgd message ownership trade timed out.\n";
      SZGClient->revokeMessageOwnershipTrade(tradingKey);
      SZGClient->messageResponse(receivedMessageID, info.str());
    }

    // Close the pipes.
    // todo: better to allocate the pipes once, not each time? less secure?
    close(pipeDescriptors[0]);
    close(pipeDescriptors[1]);
    goto LDone;
  }

  // Child process
  ar_setenv("SZGUSER", userName);
  ar_setenv("SZGCONTEXT", messageContext);
  ar_setenv("SZGPIPEID", pipeDescriptors[1]);
  ar_setenv("SZGTRADINGNUM", tradingStr);

  ar_log_remark() << "libpath =\n  " << DLLPath << "\n";
  ar_setenv(envDLLPath, DLLPath);
  if (execInfo->fPython()) {
    ar_log_remark() << "python path =\n  " << pythonPath << "\n";
    ar_setenv("PYTHONPATH", pythonPath);
  }
  info << "szgd running " << symbolicCommand << " on path\n" << execPath << ".\n";

  const unsigned iLog = messageContext.find("log=");
  if (iLog != string::npos) {
    argsMangled.push_back("-szg");
    // messageContext is ;-delimited
    const unsigned iSemi = messageContext.find(";");
    if (iSemi == string::npos) {
      // "log=FOO" was last
      argsMangled.push_back(messageContext.substr(iLog));
    } else {
      // "log=FOO" was in the middle
      argsMangled.push_back(messageContext.substr(iLog, iSemi - iLog));
    }
  }

  char** argv = buildUnixStyleArgList(newCommand, argsMangled);
  ar_log_remark() << "cmd  = " << newCommand
                << "\nargs = " << argsAsList(argsMangled);
  // Stagger launches so the cluster's file server isn't hit so hard.
  randomDelay();
  (void)execv(newCommand.c_str(), argv);
  // If execv returns at all, it returns -1 because the spawn failed.

  perror("szgd execv");
  deleteUnixStyleArgList(argv);

  info << "szgd failed to launch '" << symbolicCommand << "': ";
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
  numberBuffer[0] = 0; // magic number for "launch failed"
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, 1)) {
    ar_log_remark() << "failed to send failure code over pipe.\n";
  }
  *((int*)numberBuffer) = terminalOutput.length();
  if (!ar_safePipeWrite(pipeDescriptors[1], numberBuffer, sizeof(int))) {
     ar_log_remark() << "failed to complete pipe-based handshake.\n";
  }
  if (!ar_safePipeWrite( pipeDescriptors[1],
                         terminalOutput.c_str(),
                         terminalOutput.length())) {
    ar_log_remark() << "failed to complete pipe-based handshake, text stage.\n";
  }

  // Kill the child, so the orphaned process doesn't restart on the message loop.
  exit(0);

#else // Win32

  // Spawn a new process.

  PROCESS_INFORMATION theInfo;
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);

  // Hack: pass stuff to the child via environment variables.
  // On Unix this isn't so bad: we've already inside a fork and,
  // consequently, can change the environment with impunity. On Windows,
  // Windows processes aren't like unix's trees, so we need a lock.
  // (another solution: pass in an altered environment block)
  // (another solution: send the spawned process a message)

  // Set the current directory to that containing the app
  if (!ar_setWorkingDirectory( execInfo->appDirPath )) {
    ar_log_error() << "pre-launch failed to cd to '" <<
         execInfo->appDirPath << "'.\n";
  } else {
    ar_log_remark() << "pre-launch dir for " << execInfo->messageBody
         << "\n    is '" << execInfo->appDirPath << "'.\n";
  }

  // Set a few env vars for the child process.
  ar_setenv("SZGUSER", userName);
  ar_setenv("SZGCONTEXT", messageContext);

  // Although the pipe *communicates* with the child only in unix, still
  // tell the child that it was launched by szgd, not from the command line.
  // -1 (invalid file descriptor) says this isn't unix.
  ar_setenv("SZGPIPEID", -1);
  ar_setenv("SZGTRADINGNUM", tradingStr);
  ar_log_remark() << "libpath =\n  " << DLLPath << "\n";
  ar_setenv(envDLLPath, DLLPath);
  if (execInfo->fPython()) {
    ar_log_remark() << "python path =\n  " << pythonPath << "\n";
    ar_setenv("PYTHONPATH", pythonPath);
  }

  // Don't randomDelay(), because lockSpawn already serializes launches.

  const unsigned iLog = messageContext.find("log=");
  if (iLog != string::npos) {
    argsMangled.push_back("-szg");
    // messageContext is ;-delimited
    const unsigned iSemi = messageContext.find(";");
    if (iSemi == string::npos) {
      // "log=FOO" was last
      argsMangled.push_back(messageContext.substr(iLog));
    } else {
      // "log=FOO" was in the middle
      argsMangled.push_back(messageContext.substr(iLog, iSemi - iLog));
    }
  }

  const string theArgs = buildWindowsStyleArgList(newCommand, argsMangled);
  const string s = argsAsList(argsMangled);
  cout << "szgd executing " << newCommand << ((s=="()\n") ? "\n" : " " + s);

  const bool fArgs = theArgs != "";

  // Pack these buffers... can't just use my_string.c_str().
  char* command = new char[newCommand.length()+1];
  ar_stringToBuffer(newCommand, command, newCommand.length()+1);
  char* argsBuffer = new char[theArgs.length()+1];
  ar_stringToBuffer(theArgs, argsBuffer, theArgs.length()+1);
  ar_log_remark() << "cmd = " << command << ", args = " << argsBuffer << "\n";
  // The child might fail after being created, e.g. if a DLL is missing.
  const bool fCreated = CreateProcess(command, fArgs?argsBuffer:NULL,
                     NULL, NULL, false,
                     NORMAL_PRIORITY_CLASS, NULL, NULL,
                     &si, &theInfo);

  // Restore variables before unlocking.
  ar_setenv(envDLLPath, DLLPathPrev);
  if (execInfo->fPython())
    ar_setenv( "PYTHONPATH", oldPythonPath );
  lockSpawn.unlock();

  if (!fCreated) {
    info << "szgd failed to exec '" << command << "' with args '" << argsBuffer
         << "';\n\tGetLastError() = " << GetLastError() << ".\n";
    // szgd, not the child, will respond.
    SZGClient->revokeMessageOwnershipTrade(tradingKey);
    SZGClient->messageResponse(receivedMessageID, info.str());
  } else {
    // Wait for the child to reach its main() and respond.
    // Wait up to 20 seconds (default, unless a dex command-line arg overrides it).
    int timeoutMsec = execInfo->timeoutmsec;
    if (timeoutMsec == -1)
      timeoutMsec = 20000;
    if (!SZGClient->finishMessageOwnershipTrade(match, timeoutMsec)) {
      info << "szgd ownership trade timed out.\n"
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
/*****************************************************************************/


void messageLoop( void* /*d*/ ) {
  string userName, messageType, messageBody, messageContext;
  while (fConnect==0) {
    const int receivedMessageID = SZGClient->receiveMessage(
      &userName, &messageType, &messageBody, &messageContext);

    if (receivedMessageID == 0) {
      // szgserver disconnected
      fConnect.set(1);
      return;
    }

    if (messageType=="quit") {
      fConnect.set(2);
    }

    if (messageType=="exec") {
      // Hack - extract timeoutMsec from end of body, if it's there.
      int timeoutMsec = -1;
      string::size_type pos = messageBody.find( "||||" );
      if (pos != string::npos) {
        pos += 4;
        string timeoutString = messageBody.substr( pos, messageBody.size()-pos );
        int temp = -1;
        const bool ok = ar_stringToIntValid( timeoutString, temp );
        messageBody.replace( pos-4, timeoutString.size()+4, "" );
        if (ok) {
          ar_log_remark() << "timeout is " << temp <<
            " msec, msg body is '" << messageBody << "'.\n";
          timeoutMsec = temp;
        } else {
          ar_log_error() << "ignoring invalid timeout string '" << timeoutString << "'.\n";
        }
      }

      // Since the long-blocking exec call gets its own thread,
      // various arSZGClient methods must be thread-safe.
      arThread dummy(execProcess, new ExecInfo(
        userName, messageBody, messageContext, receivedMessageID, timeoutMsec));
      // new is matched by execProcess()'s delete.
    }

    else if (messageType=="log") {
      (void)ar_setLogLevel( messageBody );
    }

    else {
      SZGClient->messageResponse( receivedMessageID, "szgd: unknown message type '"+messageType+"'"  );
    }
  }
}

void printUsage() {
  ar_log_critical() << "Usage: szgd <basePaths> [-r]\n"
    << "  basePaths is a semicolon-delimited list of paths that prefix all\n"
    << "      SZG_EXEC and SZG_PYTHON path elements, SZG_PYTHON/lib_path, and\n"
    << "      SZG_PYTHON/executables.  They may be directories or executables\n"
    << "      (omit '.exe' on Windows).\n"
    << "  -r: repeatedly try to reconnect to the Syzygy server on failure.\n";
}

int main(int argc, char** argv) {
#ifndef AR_USE_WIN_32
  // If $DISPLAY is not 0:0, szgd creates a window on an unexpected screen.
  // Should we test for this?

  // Kill zombie processes.
  signal(SIGCHLD, SIG_IGN);
#endif

  if (argc < 2) {
    printUsage();
    return 1;
  }

  if (!getBasePaths( argv[1] )) {
    printUsage();
    return 1;
  }

  bool fRetry(false);
  if (argc > 2) {
    for (int i=2; i<argc; ++i) {
      if (!strcmp(argv[i], "-r")) {
        fRetry = true;
      }
    }
  }
  ar_log().setTimestamp(true);
  ar_log().setHeader("szgd"); // No need to append getProcessID: only 1 szgd per host.
  const int argcOriginal = argc;
  // No argvOriginal, because argv isn't modified.

LRetry:
  argc = argcOriginal;
  SZGClient = new arSZGClient;
  // Force the component's name, because win98 can't provide it.
  const bool fInit = SZGClient->init(argc, argv, "szgd");
  ar_log_debug() << "szgd version: " << ar_versionString() << ar_endl;
  if (!SZGClient->connected()) {
    if (fRetry) {
LGonnaRetry:
      delete SZGClient;
      ar_usleep(5000000);
      goto LRetry;
    }
    return SZGClient->failStandalone(fInit);
  }

  fConnect.set(0);

  ar_getWorkingDirectory( originalWorkingDirectory );

  // Only one instance per host.
  int ownerID = -1;
  if (!SZGClient->getLock(SZGClient->getComputerName() + "/szgd", ownerID)) {
    ar_log_critical() << "already running (pid = " << ownerID << ").\n";
    // todo: if we can't communicate with that pid,
    // then assume szgserver has obsolete info, "dkill -9" that pid,
    // and start up anyways.
    return 1;
  }

  arThread dummy(messageLoop, NULL);

  int pingCount(10);
  while (true) {
    ar_usleep(1000000); // szgd may take up to one second to quit.
    if (--pingCount <= 0) {
      if (!SZGClient->setAttribute( SZGClient->getComputerName(),
                      "SZG_SERVER", "szgserver_ping", "true" )) {
        ar_log_error() << "szgserver ping failed.\n";
        fConnect.set(1);
      }
      pingCount = 10;
    }
    if (fConnect == 1) {
      // szgserver disconnected
      if (fRetry) {
        goto LGonnaRetry;
      }
      cout << "exiting...\n";
      exit(0);
    }

    if (fConnect == 2) {
      // In case exit() misses ~arSZGClient().
      SZGClient->closeConnection();
      // Will return(0) be gentler, yet kill the child processes too?
      exit(0);
    }
  }
  return 1;
}
