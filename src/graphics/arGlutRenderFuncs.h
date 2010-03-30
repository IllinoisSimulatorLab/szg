//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FREEGLUT_H
#define AR_FREEGLUT_H

#include "arGraphicsHeader.h"
#include "arFreeGlutInternal.h"
#include "arGraphicsCalling.h"

SZG_CALL void ar_glutWireCube ( GLdouble dSize );
SZG_CALL void ar_glutSolidCube( GLdouble dSize );

SZG_CALL void ar_glutWireSphere (GLdouble radius, GLint slices, GLint stacks);
SZG_CALL void ar_glutSolidSphere(GLdouble radius, GLint slices, GLint stacks);

SZG_CALL void ar_glutSolidCone( GLdouble base, GLdouble height, GLint slices, GLint stacks );
SZG_CALL void ar_glutWireCone ( GLdouble base, GLdouble height, GLint slices, GLint stacks);

SZG_CALL void ar_glutSolidCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks);
SZG_CALL void ar_glutWireCylinder (GLdouble radius, GLdouble height, GLint slices, GLint stacks);

SZG_CALL void ar_glutWireTorus ( GLdouble dInnerRadius, GLdouble dOuterRadius, GLint nSides, GLint nRings );
SZG_CALL void ar_glutSolidTorus( GLdouble dInnerRadius, GLdouble dOuterRadius, GLint nSides, GLint nRings );

SZG_CALL void ar_glutWireDodecahedron ( void );
SZG_CALL void ar_glutSolidDodecahedron( void );

SZG_CALL void ar_glutWireOctahedron ( void );
SZG_CALL void ar_glutSolidOctahedron( void );

SZG_CALL void ar_glutWireTetrahedron ( void );
SZG_CALL void ar_glutSolidTetrahedron( void );

SZG_CALL void ar_glutWireIcosahedron ( void );
SZG_CALL void ar_glutSolidIcosahedron( void );

SZG_CALL void ar_glutWireRhombicDodecahedron ( void );
SZG_CALL void ar_glutSolidRhombicDodecahedron( void );

SZG_CALL void ar_glutWireSierpinskiSponge  ( int num_levels, GLdouble offset[3], GLdouble scale );
SZG_CALL void ar_glutSolidSierpinskiSponge ( int num_levels, GLdouble offset[3], GLdouble scale );

SZG_CALL void ar_glutWireTeapot ( GLdouble size );
SZG_CALL void ar_glutSolidTeapot( GLdouble size );

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

SZG_CALL SFG_StrokeFont* ar_fgStrokeRoman();
SZG_CALL SFG_StrokeFont* ar_fgStrokeMonoRoman();
SZG_CALL SFG_Font* ar_fgFontFixed8x13();
SZG_CALL SFG_Font* ar_fgFontFixed9x15();
SZG_CALL SFG_Font* ar_fgFontHelvetica10();
SZG_CALL SFG_Font* ar_fgFontHelvetica12();
SZG_CALL SFG_Font* ar_fgFontHelvetica18();
SZG_CALL SFG_Font* ar_fgFontTimesRoman10();
SZG_CALL SFG_Font* ar_fgFontTimesRoman24();

SZG_CALL void ar_glutBitmapCharacter( void* fontID, int character );
SZG_CALL void ar_glutBitmapString( void* fontID, const unsigned char *string );
SZG_CALL int ar_glutBitmapWidth( void* fontID, int character );
SZG_CALL int ar_glutBitmapLength( void* fontID, const unsigned char* string );
SZG_CALL int ar_glutBitmapHeight( void* fontID );

SZG_CALL void ar_glutStrokeCharacter( void* fontID, int character );
SZG_CALL void ar_glutStrokeString( void* fontID, const unsigned char *string );
SZG_CALL int ar_glutStrokeWidth( void* fontID, int character );
SZG_CALL int ar_glutStrokeLength( void* fontID, const unsigned char* string );
SZG_CALL GLfloat ar_glutStrokeHeight( void* fontID );
    
#endif

