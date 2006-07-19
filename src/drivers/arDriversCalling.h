//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This can be included multiple times.

// Each sublibrary of Syzygy defines or undefines SZG_IMPORT_LIBRARY.
// So undefine it here.
#undef SZG_IMPORT_LIBRARY
#ifndef SZG_COMPILING_DRIVERS
#define SZG_IMPORT_LIBRARY
#endif

#ifndef DriverFactory
#ifdef NOFACTORY
#define DriverFactory(theClass, theName)
#else
#define DriverFactory(theClass, theName) \
  extern "C" { \
    SZG_CALL void* factory() \
      { return new theClass(); } \
    SZG_CALL void baseType(char* buffer, int size) \
      { ar_stringToBuffer(theName, buffer, size); } \
  }
#endif
#endif

#include "arCallingConventions.h"
