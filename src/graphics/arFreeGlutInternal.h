/*
 * freeglut_internal.h
 *
 * The freeglut library private include file.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 2 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef  FREEGLUT_INTERNAL_H
#define  FREEGLUT_INTERNAL_H

/* XXX Update these for each release! */
#define  VERSION_MAJOR 2
#define  VERSION_MINOR 4
#define  VERSION_PATCH 0

/* Freeglut is meant to be available under all Unix/X11 and Win32 platforms. */
#if defined(_WIN32_WCE)
#   define  TARGET_HOST_UNIX_X11    0
#   define  TARGET_HOST_WIN32       0
#   define  TARGET_HOST_WINCE       1
#elif defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#   define  TARGET_HOST_UNIX_X11    0
#   define  TARGET_HOST_WIN32       1
#   define  TARGET_HOST_WINCE       0
#else
#   define  TARGET_HOST_UNIX_X11    1
#   define  TARGET_HOST_WIN32       0
#   define  TARGET_HOST_WINCE       0
#endif

#define  FREEGLUT_MAX_MENUS         3

/* Somehow all Win32 include headers depend on this one: */
#if TARGET_HOST_WIN32
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <TCHAR.H>
#endif

#if defined(_MSC_VER)
#define strdup   _strdup
#endif

/* Those files should be available on every platform. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#if HAVE_SYS_TYPES_H
#    include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#    include <unistd.h>
#endif
#if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#    include <time.h>
#else
#    if HAVE_SYS_TIME_H
#        include <sys/time.h>
#    else
#        include <time.h>
#    endif
#endif

/* The system-dependant include files should go here: */
#if TARGET_HOST_UNIX_X11
    #include <GL/glx.h>
    #include <X11/Xlib.h>
    #include <X11/Xatom.h>
    #include <X11/keysym.h>

    #ifdef HAVE_X11_EXTENSIONS_XF86VMODE_H
    #include <X11/extensions/xf86vmode.h>
    #endif
#endif

/* Microsoft VisualC++ 5.0's <math.h> does not define the PI */
#ifndef M_PI
#    define  M_PI  (3.14159265358979323846)
#endif

#ifndef TRUE
#    define  TRUE  1
#endif

#ifndef FALSE
#    define  FALSE  0
#endif

/* -- GLOBAL TYPE DEFINITIONS ---------------------------------------------- */

/*
 * A generic function pointer.  We should really use the GLUTproc type
 * defined in freeglut_ext.h, but if we include that header in this file
 * a bunch of other stuff (font-related) blows up!
 */
typedef void (*SFG_Proc)();

/* The bitmap font structure */
typedef struct tagSFG_Font SFG_Font;
struct tagSFG_Font
{
    const char*           Name;   /* The source font name             */
    int             Quantity;     /* Number of chars in font          */
    int             Height;       /* Height of the characters         */
    const GLubyte** Characters;   /* The characters mapping           */

    float           xorig, yorig; /* Relative origin of the character */
};


/* The stroke font structures */

typedef struct tagSFG_StrokeVertex SFG_StrokeVertex;
struct tagSFG_StrokeVertex
{
    GLfloat         X, Y;
};

typedef struct tagSFG_StrokeStrip SFG_StrokeStrip;
struct tagSFG_StrokeStrip
{
    int             Number;
    const SFG_StrokeVertex* Vertices;
};

typedef struct tagSFG_StrokeChar SFG_StrokeChar;
struct tagSFG_StrokeChar
{
    GLfloat         Right;
    int             Number;
    const SFG_StrokeStrip* Strips;
};

typedef struct tagSFG_StrokeFont SFG_StrokeFont;
struct tagSFG_StrokeFont
{
    const char*           Name;                       /* The source font name      */
    int             Quantity;                   /* Number of chars in font   */
    GLfloat         Height;                     /* Height of the characters  */
    const SFG_StrokeChar** Characters;          /* The characters mapping    */
};

/* -- GLOBAL VARIABLES EXPORTS --------------------------------------------- */

/* -- PRIVATE FUNCTION DECLARATIONS ---------------------------------------- */


#define  FREEGLUT_INTERNAL_ERROR_EXIT( cond, string, function )  \
  if ( ! ( cond ) )                                              \
  {                                                              \
    fgError ( " ERROR:  Internal error <%s> in function %s",     \
              (string), (function) ) ;                           \
  }

/*
 * Following definitions are somewhat similiar to GLib's,
 * but do not generate any log messages:
 */
#define  freeglut_return_if_fail( expr ) \
    if( !(expr) )                        \
        return;
#define  freeglut_return_val_if_fail( expr, val ) \
    if( !(expr) )                                 \
        return val ;

/*
 * A call to those macros assures us that there is a current
 * window set, respectively:
 */
#define  FREEGLUT_EXIT_IF_NO_WINDOW( string )                   \
  if ( ! fgStructure.CurrentWindow )                            \
  {                                                             \
    fgError ( " ERROR:  Function <%s> called"                   \
              " with no current window defined.", (string) ) ;  \
  }

/* Error Message functions */
void fgError( const char *fmt, ... );
void fgWarning( const char *fmt, ... );

#endif /* FREEGLUT_INTERNAL_H */

/*** END OF FILE ***/
