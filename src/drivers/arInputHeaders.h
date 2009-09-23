//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_HEADERS
#define AR_INPUT_HEADERS

#ifdef AR_USE_LINUX
#include <linux/joystick.h>
#include <fcntl.h>
#include <stdio.h> // for perror()
#include <errno.h> // for perror()
#endif

#ifdef AR_USE_WIN_32
#include <mmsystem.h>
#endif

#endif
