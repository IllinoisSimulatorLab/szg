//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This can be included multiple times.

// Recall that SZG_IMPORT_LIBRARY is defined or undefined by each of the
// sublibraries that make up SZG.Consequently, we have to manually undefine
// it here.

#undef SZG_IMPORT_LIBRARY

#ifndef SZG_COMPILING_GRAPHICS
#define SZG_IMPORT_LIBRARY
#endif

#include "arCallingConventions.h"
