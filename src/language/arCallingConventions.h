//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This header file is unusual in that we EXPECT it to be included multiple
// times. It defines and redefines the SZG_CALL macro, as appropriate, for
// particular blocks of code.

#undef SZG_CALL

#ifdef AR_USE_SHARED

// We only need to bother with dllimport/dllexport on the windows side.
// If a function *might* be compiled into a dll for dynamic linkage then
// AR_USE_SHARED will be defined. 

// This file provides a central place to define whether or not dll linkage
// gets requested (which allows us to easily build the libraries with static
// linkage on Windows).

// For each of Syzygy's code subdirectories
// (language, math, phleet, etc.), a special include file (arLanguageCalling.h,
// arMathCalling.h, etc.) includes this one and sets whether or not
// SZG_IMPORT_LIBRARY is defined, and, thus, whether SZG_CALL resolves to
// __declspec(dllexport) [which is the default] or __declspec(dllimport).

// Making the default __declspec(dllexport) makes this backwards compatible
// with a previous incarnation of the build structure.

// PLEASE NOTE: An inclusion of one of arLanguageCalling.h, arMathCalling.h,
// etc. sets things up so that SZG_CALL is resolved correctly for the library
// at hand and its usage (part of dll (exported), a function to be imported
// from a dll, etc.) Consequently, these should be included, in some sense,
// *last* in each .h so that the functions that follow have the right
// linkage. 

// A word about the defintion process for SZG_IMPORT_LIBRARY is appropriate
// here. We only *import* if (a) the appropriate library compilation 
// flag is undefined (SZG_IMPORT_LANGUAGE, SZG_IMPORT_MATH, etc.) OR a
// special SZG_DO_NOT_EXPORT symbol is defined. The SZG_DO_NOT_EXPORT symbol
// is a useful hack which allows programs compiled along with the szg
// sub-libraries to easily only *import* functions from their associated dlls.
// (the problem is that the local compile-time rule defines the (for example)
// SZG_IMPORT_LANGUAGE symbol, but our .cpp is not part of the dll).

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

