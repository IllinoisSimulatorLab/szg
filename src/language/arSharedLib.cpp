//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSharedLib.h"
#include <stdio.h>
#include <errno.h>
#include <iostream>
using namespace std;

class spooch { public: int x; };

arSharedLib::arSharedLib() :
  _h(NULL)
{
}

arSharedLib::~arSharedLib(){
}

#ifdef AR_USE_WIN_32

bool arSharedLib::test(){
  typedef signed char (*myfn)(int, int, unsigned int);
  // try to load foo.dll
  if (!open("I:/augr/fmod")) {
    cout << "found no foo.dll\n";
    return 42;
  }
  myfn bar = (myfn)sym("_FSOUND_Init@12");
  if (!bar) {
    cout << "found no myFunction() in foo library.\n";
    return 420;
  }
  bool f = (*bar)(44100, 20, 0);
  cout << "bar() returned " << f << endl;
  return f;
  // it crashes during the return.  hmm.
}


bool arSharedLib::open(const char *pathname) {
  _h = LoadLibrary(pathname);
  return _h != NULL;
}

bool arSharedLib::close() {
  return FreeLibrary(_h) != 0;
}

void* arSharedLib::sym(const char *name) {
  return GetProcAddress(_h, name);
}

const char* arSharedLib::error() {
  static char buf[80];
  sprintf(buf, "GetLastError() = %d", GetLastError());
  return buf;
}

#else

bool arSharedLib::test(){
  typedef spooch* (*myfn)(int, char*);
  // try to load foo.dll
  // "/lib/libfmod-3.4.so"
  if (!open("/usr/lib/libspooge.so")) {
    cout << "failed to load shared library.\n";
    cout << error() << endl;
    return 42;
  }
  myfn bar = (myfn)sym("testEntryPoint");
  if (!bar) {
    cout << "found no myFunction() in shared library.\n";
    cout << error() << endl;
    return 420;
  }
  spooch* p = (*bar)(19, "fuzzy wuzzy");
  cout << "bar() returned " << p->x << endl;
  return 4200;
}

bool arSharedLib::open(const char *pathname) {
  _h = dlopen(pathname, RTLD_NOW);
  return _h != NULL;
}

bool arSharedLib::close() {
  return dlclose(_h) == 0;
}

void* arSharedLib::sym(const char *name) {
  return dlsym(_h, name);
}

const char* arSharedLib::error() {
  return dlerror();
}

#endif
