//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSharedLib.h"
#include "arDataUtilities.h"
#include "arLogStream.h"

#include <stdio.h>
#include <errno.h>
#include <sstream>
#ifdef AR_USE_WIN_32
  #include <iostream>
#endif

using namespace std;

arSharedLib::arSharedLib() :
  _h(NULL),
  _factory(NULL),
  _objectType(NULL),
  _fLoaded(false) {
}

arSharedLib::~arSharedLib() {
}

// Given a shared library name (WITHOUT the .dll or .so suffix that varies
// by operating system), search the given path (which is a
// semicolon-delimited string) for the library and attempt to load it,
// returning true upon success and false upon failure.
bool arSharedLib::open(const string& sharedLibName, const string& path) {
  string libName(sharedLibName);
  // Append .dll or .so
  ar_addSharedLibExtension(libName);
  if (path != "") {
    // Produce an absolute file name.
    string fileName(libName);

    // Use an absolute path name, if possible, to avoid the
    // OS's unpredictable internal shared library searching.
    libName = ar_fileFind(fileName, "", path);
    if (libName == "NULL") {
      ar_log_error() << "arSharedLib: no shared lib '" << fileName << "' on path '"
	   << path << "'.\n";
      return false;
    }
  }
  ar_log_remark() << "arSharedLib loading " << libName << ".\n";

#ifdef AR_USE_WIN_32
  _h = LoadLibrary(libName.c_str());
  if (_h)
    _fLoaded = true;
#else
  if (_h)
    _fLoaded = true;
  _h = dlopen(libName.c_str(), RTLD_NOW);
#endif
  return _h != NULL;
}

// Unmap the already loaded library from the shared memory space, returning
// true for success and false for error.
bool arSharedLib::close() {
  _fLoaded = false;
#ifdef AR_USE_WIN_32
  return FreeLibrary(_h) != 0;
#else
  return dlclose(_h) == 0;
#endif
}

// Return a pointer to the (name-mangled) function in the shared library.
// (Mac OS X prepends a "_".)
void* arSharedLib::sym(const string& functionName) {
  if (!_fLoaded) {
    ar_log_error() << "arSharedLib: no library loaded.\n";
    return NULL;
  }
#ifdef AR_USE_WIN_32
  return (void*)GetProcAddress(_h, functionName.c_str());
#else
  const string name =
#ifdef AR_USE_DARWIN
  // Linux and Irix don't need leading "_"
    "_" +
#endif
          functionName;
  return dlsym(_h, name.c_str());
#endif
}

// Return the last error message due to loading.
string arSharedLib::error() {
#ifdef AR_USE_WIN_32
  return "GetLastError() = " + ar_intToString(GetLastError());
#else
  return string(dlerror());
#endif
}

// Each shared library contains code for a single C++ class.
// The class can be examined and instantiated via two
// functions (accessed via sym()):
//
//   extern "C" void* factory();
//   extern "C" void  baseType(char* buffer, int bufferSize);
//
// factory() returns instances of the object. baseType() returns
// in "buffer" the class's name (e.g., "arIOFilter"), for type checking.
//
// createFactory() loads a shared library, checks
// that these two functions exist, and that baseType() matches
// the given type. On error, "error" contains the reason and it returns false.
// On success, call createObect() to make instances of the class.
bool arSharedLib::createFactory(const string& sharedLibName,
                                const string& path,
                                const string& type,
                                string& error) {
  if (_fLoaded) {
    error = "arSharedLib: library already loaded.\n";
LAbort:
    return false;
  }

  // The arSZGClient returns "NULL" for unset parameters. Consequently,
  // if path is "NULL", we set execPath to "" (which is what open(...)
  // expects).
  const string execPath = (path == "NULL") ? "" : path;

  // Attempt to load the shared library.
  if (!arSharedLib::open(sharedLibName,execPath)) {
    error = "arSharedLib failed to load '" + sharedLibName + "' on path '" + execPath + "'.\n";
    goto LAbort;
  }
  _fLoaded = true;

  // make sure that we have the right type.
  _objectType = (arSharedLibObjectType) arSharedLib::sym("baseType");
  if (!_objectType) {
    error = "arSharedLib: no baseType function in '" + sharedLibName + "'.\n";
    goto LAbort;
  }
  char typeBuffer[256];
  _objectType(typeBuffer, 256);
  if (strcmp(typeBuffer, type.c_str())) {
    error = "arSharedLib: wrong type '" + string(typeBuffer) + "' in '" + sharedLibName + "'.\n";
    goto LAbort;
  }

  _factory = (arSharedLibFactory) arSharedLib::sym("factory");
  if (!_factory) {
    // Calls to createObject() will fail.
    error = "arSharedLib: no factory function in '" + sharedLibName + "'.\n";
    goto LAbort;
  }
  return true;
}

// Once the internal factory has been successfully created, create an object.
void* arSharedLib::createObject() {
  if (!_factory) {
    // createFactory() had failed.
    ar_log_error() << "arSharedLib: unmapped factory.\n";
    return NULL;
  }

  return _factory();
}
