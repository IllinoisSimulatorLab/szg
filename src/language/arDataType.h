//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_TYPE
#define AR_DATA_TYPE

#include "arLanguageCalling.h"

////////// Platform-DEPENDENT ///////////////////////

// Implementation of the standard data types on local machine

typedef char ARchar;
typedef int ARint;
typedef long ARlong;
typedef float ARfloat;
typedef double ARdouble;
#ifdef AR_USE_WIN_32
typedef LONGLONG ARint64;
#else
typedef long long ARint64;
#endif

////////// Platform-INDEPENDENT ///////////////////////

// Sizes of the standard data types.
/// \bug The Syzygy core silently assumes sizeof(float) == sizeof(ARfloat).
/// \bug The Syzygy core silently assumes sizeof(int) == sizeof(ARint).
const int AR_GARBAGE_SIZE = 0;
const int AR_CHAR_SIZE = 1;
const int AR_INT_SIZE = 4;
const int AR_LONG_SIZE = 4;
const int AR_FLOAT_SIZE = 4;
const int AR_DOUBLE_SIZE = 8;
const int AR_INT64_SIZE = 8;

// IDs of data types.

// These type codes are encoded in binary records and must NEVER change!
// ALWAYS ADD NEW DATA TYPES TO THE REAR (i.e. USING NEW CODES)!
enum arDataType {
  AR_GARBAGE=0, AR_CHAR=1, AR_INT=2, AR_LONG=3, AR_FLOAT=4, AR_DOUBLE=5, AR_INT64=6
};

// Get the size and name of a data type from its ID.

SZG_CALL int arDataTypeSize(arDataType theType);
SZG_CALL const char* arDataTypeName(arDataType theType);
SZG_CALL arDataType arDataNameType(const char* const theName) ;

#endif
