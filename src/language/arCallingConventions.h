//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// We EXPECT this file to be included multiple times.
// It (re)defines SZG_CALL for particular blocks of code.

#undef SZG_CALL

/*

This file centrally defines whether or not dll linkage
gets requested (which lets us build static libraries in Windows).

We only need dllimport/dllexport on Windows.
If a function *might* be compiled into a dll for dynamic linkage then
AR_LINKING_DYNAMIC will be defined.

For each of Syzygy's code subdirectories
(language, math, phleet, etc.), a special include file (arLanguageCalling.h,
arMathCalling.h, etc.) includes this one and (un)defines
SZG_IMPORT_LIBRARY, and thus whether SZG_CALL resolves to the default
__declspec(dllexport), or to __declspec(dllimport).

The default __declspec(dllexport) makes this backwards compatible
with a previous version of the build structure.

Including one of arLanguageCalling.h, arMathCalling.h, etc. properly
defines SZG_CALL for the library at hand and its usage (part of dll
(exported), a function to be imported from a dll, etc.)  Thus, these
should be the last syzygy .h file included.

About defining SZG_IMPORT_LIBRARY: we only import, not export,
if the appropriate library compilation flag is undefined
(SZG_IMPORT_LANGUAGE, SZG_IMPORT_MATH, etc.), or if SZG_DO_NOT_EXPORT
is defined. SZG_DO_NOT_EXPORT lets programs compiled along with the szg
sub-libraries to easily only *import* functions from their associated
dlls.  (The problem is that the local compile-time rule defines the (for
example) SZG_IMPORT_LANGUAGE symbol, but our .cpp is not part of the dll).

*/

#ifdef AR_LINKING_DYNAMIC
  #ifdef AR_USE_WIN_32
    #ifdef SZG_DO_NOT_EXPORT
      #define SZG_CALL __declspec(dllimport)
    #else
      #ifdef SZG_IMPORT_LIBRARY
        #define SZG_CALL __declspec(dllimport)
      #else
        #define SZG_CALL __declspec(dllexport)
      #endif
    #endif
  #else
    #define SZG_CALL
  #endif
#else
  #define SZG_CALL
#endif
