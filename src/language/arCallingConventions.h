//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CALLING_CONVENTIONS_H
#define AR_CALLING_CONVENTIONS_H

#ifdef AR_USE_SHARED

#ifdef AR_USE_WIN_32
#define SZG_CALL __declspec(dllexport)
#else
#define SZG_CALL
#endif

#else

#define SZG_CALL

#endif

#endif
