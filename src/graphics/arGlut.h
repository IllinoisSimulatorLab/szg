//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GLUT_H
#define AR_GLUT_H

#ifdef AR_USE_WIN_32
  #include <GL\glut.h>
#else
  #ifdef AR_USE_DARWIN
    #include <GLUT/glut.h>
  #else
    // Linux/SGI
    #include <GL/glut.h>
  #endif
#endif

#endif
