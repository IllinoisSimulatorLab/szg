//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FREEGLUT_H
#define AR_FREEGLUT_H

#include "arGraphicsHeader.h"
#include "arGraphicsCalling.h"

void SZG_CALL ar_glutWireCube( GLdouble dSize );

void SZG_CALL ar_glutSolidCube( GLdouble dSize );

void SZG_CALL ar_glutSolidSphere(GLdouble radius, GLint slices, GLint stacks);

void SZG_CALL ar_glutWireSphere(GLdouble radius, GLint slices, GLint stacks);

void SZG_CALL ar_glutSolidCone( GLdouble base, GLdouble height, GLint slices, GLint stacks );

void SZG_CALL ar_glutWireCone( GLdouble base, GLdouble height, GLint slices, GLint stacks);

void SZG_CALL ar_glutSolidCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks);

void SZG_CALL ar_glutWireCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks);

void SZG_CALL ar_glutWireTorus( GLdouble dInnerRadius, GLdouble dOuterRadius, GLint nSides, GLint nRings );

void SZG_CALL ar_glutSolidTorus( GLdouble dInnerRadius, GLdouble dOuterRadius, GLint nSides, GLint nRings );

void SZG_CALL ar_glutWireDodecahedron( void );

void SZG_CALL ar_glutSolidDodecahedron( void );

void SZG_CALL ar_glutWireOctahedron( void );

void SZG_CALL ar_glutSolidOctahedron( void );

void SZG_CALL ar_glutWireTetrahedron( void );

void SZG_CALL ar_glutSolidTetrahedron( void );

void SZG_CALL ar_glutWireIcosahedron( void );

void SZG_CALL ar_glutSolidIcosahedron( void );

void SZG_CALL ar_glutWireRhombicDodecahedron( void );

void SZG_CALL ar_glutSolidRhombicDodecahedron( void );

void SZG_CALL ar_glutWireSierpinskiSponge ( int num_levels, GLdouble offset[3], GLdouble scale );

void SZG_CALL ar_glutSolidSierpinskiSponge ( int num_levels, GLdouble offset[3], GLdouble scale );

/*
 * Renders a beautiful wired teapot...
 */
void SZG_CALL ar_glutWireTeapot( GLdouble size );

/*
 * Renders a beautiful filled teapot...
 */
void SZG_CALL ar_glutSolidTeapot( GLdouble size );

/*
 * GLUT API macro definitions -- fonts definitions
 *
 * Steve Baker suggested to make it binary compatible with GLUT:
 */
#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#   define  GLUT_STROKE_ROMAN               ((void *)0x0000)
#   define  GLUT_STROKE_MONO_ROMAN          ((void *)0x0001)
#   define  GLUT_BITMAP_9_BY_15             ((void *)0x0002)
#   define  GLUT_BITMAP_8_BY_13             ((void *)0x0003)
#   define  GLUT_BITMAP_TIMES_ROMAN_10      ((void *)0x0004)
#   define  GLUT_BITMAP_TIMES_ROMAN_24      ((void *)0x0005)
#   define  GLUT_BITMAP_HELVETICA_10        ((void *)0x0006)
#   define  GLUT_BITMAP_HELVETICA_12        ((void *)0x0007)
#   define  GLUT_BITMAP_HELVETICA_18        ((void *)0x0008)
#else
    /*
     * I don't really know if it's a good idea... But here it goes:
     */
    extern void* glutStrokeRoman;
    extern void* glutStrokeMonoRoman;
    extern void* glutBitmap9By15;
    extern void* glutBitmap8By13;
    extern void* glutBitmapTimesRoman10;
    extern void* glutBitmapTimesRoman24;
    extern void* glutBitmapHelvetica10;
    extern void* glutBitmapHelvetica12;
    extern void* glutBitmapHelvetica18;

    /*
     * Those pointers will be used by following definitions:
     */
#   define  GLUT_STROKE_ROMAN               ((void *) &glutStrokeRoman)
#   define  GLUT_STROKE_MONO_ROMAN          ((void *) &glutStrokeMonoRoman)
#   define  GLUT_BITMAP_9_BY_15             ((void *) &glutBitmap9By15)
#   define  GLUT_BITMAP_8_BY_13             ((void *) &glutBitmap8By13)
#   define  GLUT_BITMAP_TIMES_ROMAN_10      ((void *) &glutBitmapTimesRoman10)
#   define  GLUT_BITMAP_TIMES_ROMAN_24      ((void *) &glutBitmapTimesRoman24)
#   define  GLUT_BITMAP_HELVETICA_10        ((void *) &glutBitmapHelvetica10)
#   define  GLUT_BITMAP_HELVETICA_12        ((void *) &glutBitmapHelvetica12)
#   define  GLUT_BITMAP_HELVETICA_18        ((void *) &glutBitmapHelvetica18)
#endif

#include "arFreeGlutInternal.h"
SFG_StrokeFont SZG_CALL *ar_fgStrokeRoman();
SFG_StrokeFont SZG_CALL *ar_fgStrokeMonoRoman();
SFG_Font SZG_CALL *ar_fgFontFixed8x13();
SFG_Font SZG_CALL *ar_fgFontFixed9x15();
SFG_Font SZG_CALL *ar_fgFontHelvetica10();
SFG_Font SZG_CALL *ar_fgFontHelvetica12();
SFG_Font SZG_CALL *ar_fgFontHelvetica18();
SFG_Font SZG_CALL *ar_fgFontTimesRoman10();
SFG_Font SZG_CALL *ar_fgFontTimesRoman24();

void SZG_CALL ar_glutStrokeCharacter( void* fontID, int character );
void SZG_CALL ar_glutStrokeString( void* fontID, const unsigned char *string );
int SZG_CALL ar_glutStrokeWidth( void* fontID, int character );
int SZG_CALL ar_glutStrokeLength( void* fontID, const unsigned char* string );
GLfloat SZG_CALL ar_glutStrokeHeight( void* fontID );
    
#endif

