//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSharedLib.h"
#include "arDataUtilities.h"
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include <iostream>
using namespace std;

arSharedLib::arSharedLib() :
  _h(NULL),
  _factory(NULL),
  _objectType(NULL),
  _libraryLoaded(false),
  _factoryMapped(false){
}

arSharedLib::~arSharedLib(){
}

/// Given a shared library name (WITHOUT the .dll or .so suffix that varies
/// by operating system), search the given path (which is a
/// semicolon-delimited string) for the library and attempt to load it,
/// returning true upon success and false upon failure.
bool arSharedLib::open(const string& sharedLibName, const string& path){
  string libName = sharedLibName;
  // Adds .dll or .so depending upon the platform.
  ar_addSharedLibExtension(libName);
  // Only try to produce an absolute file name if the path is specified.
  if (path != ""){
    string fileName = libName;
    // We want to use an absolute path name, if possible, to avoid the
    // unpredictability of the OS's internal shared library searching.
    libName = ar_fileFind(fileName,"",path);
    if (libName == "NULL"){
      cout << "arSharedLib error: could not find shared lib " << fileName << " on given "
	   << "path.\n";
      return false;
    }
  }
  // Next there is a Win32 way and a Unix way...
#ifdef AR_USE_WIN_32
  // The Win32 way...
  _h = LoadLibrary(libName.c_str());
  if (_h){
    _libraryLoaded = true;
  }
  return _h != NULL;
#else
  // The Unix way...
  if (_h){
    _libraryLoaded = true;
  }
  _h = dlopen(libName.c_str(), RTLD_NOW);
  return _h != NULL;
#endif
}

/// Unmap the already loaded library from the shared memory space, returning
/// true for success and false for error.
bool arSharedLib::close(){
  _libraryLoaded = false;
  _factoryMapped = false;
#ifdef AR_USE_WIN_32
  // The Win32 way..
  return FreeLibrary(_h) != 0;
#else
  // The Unix way...
  return dlclose(_h) == 0;
#endif
}

/// Try to return a pointer to the named function in the shared library.
/// Some name-mangling might occur (for instance, Mac OS X appends a "_"
/// to the front of function names. Returns NULL on error.
void* arSharedLib::sym(const string& functionName){
  if (!_libraryLoaded){
    cout << "arSharedLib error: shared library has not been loaded.\n";
    return NULL;
  }
#ifdef AR_USE_WIN_32
  // The Windows way
  return GetProcAddress(_h, functionName.c_str());
#else
  // The Unix way
  string name;
#ifdef AR_USE_DARWIN
  // Specifically for Mac OS X, we need to add a leading "_"
  name = "_" + functionName;
#else
  // Linux and Irix do not seem to have this property.
  name = functionName;
#endif
  return dlsym(_h, name.c_str());
#endif
}

/// If an error has occured, this will a string containing the last
/// error message generated by the loading mechanism.
string arSharedLib::error(){
  string result;
#ifdef AR_USE_WIN_32
  static char buf[80];
  sprintf(buf, "GetLastError() = %d", GetLastError());
  result = string(buf);
#else
  result = string(dlerror());
#endif
  return result;
}

/// This function is used in syzygy's plug-in architecture. There, each shared
/// library contains the code for a single C++ object. Information about this
/// object can be gleaned (and instances of the object created) by 2 well
/// known functions contained in each shared lib (which are accessed via sym):
///
/// extern "C"{
///   void*               factory();
///   void                baseType(char* buffer, int bufferSize);
/// }
///
/// Here, the factory returns instances of the object and baseType fills the
/// passed buffer with the base class type (i.e. arIOFilter, arInputSource,
/// etc.). The baseType function is needed for type-checking.
///
/// This function attempts to load the referenced shared library, checks
/// that each of the required functions exist, and that baseType(...)
/// claims the given type. If any of these conditions fail, then it returns
/// false and fills "error" with an informative string. Otherwise, it returns
/// true and the user can make subsequent calss to "createObject" to 
/// create instances of the object in question.
bool arSharedLib::createFactory(const string& sharedLibName,
                                const string& path,
                                const string& type,
                                string& error){
  if (_libraryLoaded){
    cout << "arSharedLib error: library already loaded.\n";
    return false;
  }
  // The arSZGClient returns "NULL" for unset parameters. Consequently,
  // if path is "NULL", we set execPath to "" (which is what open(...)
  // expects).
  stringstream message;
  string execPath;
  if (path == "NULL"){
    execPath = "";
  }
  else{
    execPath = path;
  }
  // Attempt to load the shared library.
  if (!arSharedLib::open(sharedLibName,execPath)){
    message << "arSharedLib error: could not dynamically load object "
	    << "(name=" << sharedLibName << ") on path=" << execPath << ".\n";
    error = message.str();
    return false;
  }
  _libraryLoaded = true;

  // First, make sure that we do indeed have the right type.
  _objectType = (arSharedLibObjectType) arSharedLib::sym("baseType");
  if (!_objectType){
    message << "arSharedLib error: could not map type function.\n";
    error = message.str();
    return false;
  }
  char typeBuffer[256];
  _objectType(typeBuffer, 256);
  if (strcmp(typeBuffer, type.c_str())){
    message << "DeviceServer error: dynamically loaded object "
	    << "(name=" << sharedLibName << ") declares wrong type="
	    << typeBuffer << "\n";
    error = message.str();
    return false;
  }
  // Get the factory function
  _factory = (arSharedLibFactory) arSharedLib::sym("factory");
  if (!_factory){
    message << "DeviceServer error: could not map factory function in "
	    << "object (name=" << sharedLibName << ").\n";
    error = message.str();
    return false;
  }
  _factoryMapped = true;
  return true;
}

/// Once the internal factory has been successfully created, go ahead and
/// create an object. If the factory has not been successfully created, return
/// NULL.
void* arSharedLib::createObject(){
  if (!_factoryMapped){
    cout << "arSharedLib error: factory has not been mapped.\n";
    return NULL;
  }
  return _factory();
}
