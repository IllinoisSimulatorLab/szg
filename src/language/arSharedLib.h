//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SHAREDLIB_H
#define AR_SHAREDLIB_H

#ifdef AR_USE_WIN_32
#include <windows.h>
typedef HMODULE Handle;
#endif

#ifdef AR_USE_LINUX
typedef void* Handle;
#include <dlfcn.h>
#endif

#ifdef AR_USE_SGI
typedef void* Handle;
#include <dlfcn.h>
#endif

#ifdef AR_USE_DARWIN
typedef void* Handle;
#include <dlfcn.h>
#endif

/// For shared libraries (.DLL's, .so's).

class arSharedLib {
 public:
  arSharedLib();
  ~arSharedLib();

  bool open(const char *pathname); // pass "libfoo" for libfoo.dll
  bool close();
  void* sym(const char *name); // returns NULL on error
  const char* error(); // returns detailed error info
  bool test();

 private:
  Handle _h;
};

#endif
