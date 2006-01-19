//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_LIB_H
#define AR_SHARED_LIB_H

#ifdef AR_USE_WIN_32
// DO NOT INCLUDE <windows.h> here! INSTEAD DO AS BELOW...
#include "arPrecompiled.h"
typedef HMODULE LibHandle;
#endif

#include "arLanguageCalling.h"

#ifdef AR_USE_LINUX
typedef void* LibHandle;
#include <dlfcn.h>
#endif

#ifdef AR_USE_SGI
typedef void* LibHandle;
#include <dlfcn.h>
#endif

#ifdef AR_USE_DARWIN
typedef void* LibHandle;
// NOTE: This is the dlcompat wrapper over Mac OS X's native dylib API
#include <dlfcn.h>
#endif

#include <string>
using namespace std;

/// A wrapper class that allows portable use of shared libraries.
/// It handles the different naming conventions and behaviors for
/// different platforms as automatically as is possible. It also 
/// offers syzygy-aware functions for dll-loading, as is needed, for
/// instance, in managing load paths across different architectures.
/// A case in point: linux and irix require, essentially, that dll's
/// loaded without absolute path names either be on the LD_LIBRARY_PATH
/// or in a standard directory, whereas Win32 will look first for dll's
/// in the executable's directory.

typedef void* (*arSharedLibFactory)();
typedef void (*arSharedLibObjectType)(char*, int);

class SZG_CALL arSharedLib {
 public:
  arSharedLib();
  ~arSharedLib();

  bool open(const string& sharedLibName, const string& path);
  bool close();
  void* sym(const string& functionName);
  string error();

  bool createFactory(const string& sharedLibName,
                     const string& path,
                     const string& type,
                     string& error);
  void* createObject();

 private:
  LibHandle _h;
  arSharedLibFactory    _factory;
  arSharedLibObjectType _objectType;
  bool _libraryLoaded;
  bool _factoryMapped;
};

#endif
