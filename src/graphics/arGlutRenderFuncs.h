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
#   define  GLUT_STROKE_MONO_ROMAN          ((void *)0x0001)
#else
    /*
     * I don't really know if it's a good idea... But here it goes:
     */
    extern void* glutStrokeMonoRoman;

    /*
     * Those pointers will be used by following definitions:
     */
#   define  GLUT_STROKE_MONO_ROMAN          ((void *) &glutStrokeMonoRoman)
#endif

void* SZG_CALL ar_fgStrokeMonoRoman();
    
void SZG_CALL ar_glutStrokeCharacter( void* fontID, int character );
void SZG_CALL ar_glutStrokeString( void* fontID, const unsigned char *string );
int SZG_CALL ar_glutStrokeWidth( void* fontID, int character );
int SZG_CALL ar_glutStrokeLength( void* fontID, const unsigned char* string );
GLfloat SZG_CALL ar_glutStrokeHeight( void* fontID );
    
#endif

