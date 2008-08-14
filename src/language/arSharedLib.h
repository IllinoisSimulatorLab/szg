//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHARED_LIB_H
#define AR_SHARED_LIB_H

#ifdef AR_USE_WIN_32
#ifdef AR_USE_MINGW
#include <windows.h>
#else
#include "arPrecompiled.h"
#endif
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
// dlcompat wrapper around native dylib API
#include <dlfcn.h>
#endif

#include <string>
using namespace std;

// Wrapper class for portable use of shared libraries.
// Hide platforms' different naming conventions and behaviors.
// Offer syzygy-aware functions for dll-loading, as is needed, for
// instance, in managing load paths.  Example: linux and irix require
// that dll's lacking absolute path names be either on
// LD_LIBRARY_PATH or in a standard directory,
// but win32 first checks the exe's directory.

typedef void* (*arSharedLibFactory)();
typedef void (*arSharedLibObjectType)(char*, int);

class SZG_CALL arSharedLib {
 public:
  arSharedLib();
  virtual ~arSharedLib();

  bool open(const string& sharedLibName, const string& path);
  bool close();
  void* sym(const string& functionName);
  string error();

  bool createFactory(const string& sharedLibName,
                     const string& path,
                     const string& type,
                     string& error);
  virtual void* createObject();

 private:
  LibHandle _h;
  arSharedLibFactory _factory;
  arSharedLibObjectType _objectType;
  bool _fLoaded;
};

#endif
