/*
 * freeglut_font.c
 *
 * Bitmap and stroke fonts displaying.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 16 1999
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

#include "arPrecompiled.h"
#include "arFreeGlutInternal.h"
#include "arLogStream.h"
#include "arGraphicsHeader.h"
#include "arGlutRenderFuncs.h"

/*
 * TODO BEFORE THE STABLE RELEASE:
 *
 *  Test things out ...
 */

/* -- PRIVATE FUNCTIONS ---------------------------------------------------- */


/*
 * Matches a font ID with a SFG_Font structure pointer.
 * This was changed to match the GLUT header style.
 */
static SFG_Font* ar_fghFontByID( void* font )
{
    if( font == GLUT_BITMAP_8_BY_13        )
        return static_cast<SFG_Font*>(ar_fgFontFixed8x13());
    if( font == GLUT_BITMAP_9_BY_15        )
        return static_cast<SFG_Font*>(ar_fgFontFixed9x15());
    if( font == GLUT_BITMAP_HELVETICA_10   )
        return static_cast<SFG_Font*>(ar_fgFontHelvetica10());
    if( font == GLUT_BITMAP_HELVETICA_12   )
        return static_cast<SFG_Font*>(ar_fgFontHelvetica12());
    if( font == GLUT_BITMAP_HELVETICA_18   )
        return static_cast<SFG_Font*>(ar_fgFontHelvetica18());
    if( font == GLUT_BITMAP_TIMES_ROMAN_10 )
        return static_cast<SFG_Font*>(ar_fgFontTimesRoman10());
    if( font == GLUT_BITMAP_TIMES_ROMAN_24 )
        return static_cast<SFG_Font*>(ar_fgFontTimesRoman24());

    ar_log_error() << "font " << font << " not found." << ar_endl;
    return 0;
}


/*
 * Matches a font ID with a SFG_StrokeFont structure pointer.
 * This was changed to match the GLUT header style.
 */
static SFG_StrokeFont* ar_fghStrokeByID( void* font )
{
    if( font == GLUT_STROKE_MONO_ROMAN )
        return static_cast<SFG_StrokeFont*>(ar_fgStrokeMonoRoman());
    if( font == GLUT_STROKE_ROMAN )
        return static_cast<SFG_StrokeFont*>(ar_fgStrokeRoman());

    ar_log_error() << "stroke font " << font << " not found." << ar_endl;
    return 0;
}


/* -- INTERFACE FUNCTIONS -------------------------------------------------- */

/*
 * Draw a bitmap character
 */
void ar_glutBitmapCharacter( void* fontID, int character )
{
    const GLubyte* face;
    SFG_Font* font;
    font = ar_fghFontByID( fontID );
    freeglut_return_if_fail( ( character >= 1 )&&( character < 256 ) );
    freeglut_return_if_fail( font );

    /*
     * Find the character we want to draw (???)
     */
    face = font->Characters[ character ];

    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );
    glBitmap(
        face[ 0 ], font->Height,      /* The bitmap's width and height  */
        font->xorig, font->yorig,     /* The origin in the font glyph   */
        ( float )( face[ 0 ] ), 0.0,  /* The raster advance -- inc. x,y */
        ( face + 1 )                  /* The packed bitmap data...      */
    );
    glPopClientAttrib( );
}

void ar_glutBitmapString( void* fontID, const unsigned char *string )
{
    unsigned char c;
    float x = 0.0f ;
    SFG_Font* font;
    font = ar_fghFontByID( fontID );
    freeglut_return_if_fail( font );
    if ( !string || ! *string )
        return;

    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );

    /*
     * Step through the string, drawing each character.
     * A newline will simply translate the next character's insertion
     * point back to the start of the line and down one line.
     */
    while( ( c = *string++) )
        if( c == '\n' )
        {
            glBitmap ( 0, 0, 0, 0, -x, (float) -font->Height, NULL );
            x = 0.0f;
        }
        else  /* Not an EOL, draw the bitmap character */
        {
            const GLubyte* face = font->Characters[ c ];

            glBitmap(
                face[ 0 ], font->Height,     /* Bitmap's width and height    */
                font->xorig, font->yorig,    /* The origin in the font glyph */
                ( float )( face[ 0 ] ), 0.0, /* The raster advance; inc. x,y */
                ( face + 1 )                 /* The packed bitmap data...    */
            );

            x += ( float )( face[ 0 ] );
        }

    glPopClientAttrib( );
}

/*
 * Returns the width in pixels of a font's character
 */
int ar_glutBitmapWidth( void* fontID, int character )
{
    SFG_Font* font;
    font = ar_fghFontByID( fontID );
    freeglut_return_val_if_fail( character > 0 && character < 256, 0 );
    freeglut_return_val_if_fail( font, 0 );
    return *( font->Characters[ character ] );
}

/*
 * Return the width of a string drawn using a bitmap font
 */
int ar_glutBitmapLength( void* fontID, const unsigned char* string )
{
    unsigned char c;
    int length = 0, this_line_length = 0;
    SFG_Font* font;
    font = ar_fghFontByID( fontID );
    freeglut_return_val_if_fail( font, 0 );
    if ( !string || ! *string )
        return 0;

    while( ( c = *string++) )
    {
        if( c != '\n' )/* Not an EOL, increment length of line */
            this_line_length += *( font->Characters[ c ]);
        else  /* EOL; reset the length of this line */
        {
            if( length < this_line_length )
                length = this_line_length;
            this_line_length = 0;
        }
    }
    if ( length < this_line_length )
        length = this_line_length;

    return length;
}

/*
 * Returns the height of a bitmap font
 */
int ar_glutBitmapHeight( void* fontID )
{
    SFG_Font* font;
    font = ar_fghFontByID( fontID );
    freeglut_return_val_if_fail( font, 0 );
    return font->Height;
}


/*
 * Draw a stroke character
 */
void ar_glutStrokeCharacter( void* fontID, int character )
{
    const SFG_StrokeChar *schar;
    const SFG_StrokeStrip *strip;
    int i, j;
    SFG_StrokeFont* font;
    font = ar_fghStrokeByID( fontID );
    freeglut_return_if_fail( character >= 0 );
    freeglut_return_if_fail( character < font->Quantity );
    freeglut_return_if_fail( font );

    schar = font->Characters[ character ];
    freeglut_return_if_fail( schar );
    strip = schar->Strips;

    for( i = 0; i < schar->Number; i++, strip++ )
    {
        glBegin( GL_LINE_STRIP );
        for( j = 0; j < strip->Number; j++ )
            glVertex2f( strip->Vertices[ j ].X, strip->Vertices[ j ].Y );
        glEnd( );
    }
    glTranslatef( schar->Right, 0.0, 0.0 );
}

void ar_glutStrokeString( void* fontID, const unsigned char *string )
{
    unsigned char c;
    int i, j;
    float length = 0.0;
    SFG_StrokeFont* font;
    font = ar_fghStrokeByID( fontID );
    freeglut_return_if_fail( font );
    if ( !string || ! *string )
        return;

    /*
     * Step through the string, drawing each character.
     * A newline will simply translate the next character's insertion
     * point back to the start of the line and down one line.
     */
    while( ( c = *string++) )
        if( c < font->Quantity )
        {
            if( c == '\n' )
            {
                glTranslatef ( -length, -( float )( font->Height ), 0.0 );
                length = 0.0;
            }
            else  /* Not an EOL, draw the bitmap character */
            {
                const SFG_StrokeChar *schar = font->Characters[ c ];
                if( schar )
                {
                    const SFG_StrokeStrip *strip = schar->Strips;

                    for( i = 0; i < schar->Number; i++, strip++ )
                    {
                        glBegin( GL_LINE_STRIP );
                        for( j = 0; j < strip->Number; j++ )
                            glVertex2f( strip->Vertices[ j ].X,
                                        strip->Vertices[ j ].Y);

                        glEnd( );
                    }

                    length += schar->Right;
                    glTranslatef( schar->Right, 0.0, 0.0 );
                }
            }
        }
}

/*
 * Return the width in pixels of a stroke character
 */
int ar_glutStrokeWidth( void* fontID, int character )
{
    const SFG_StrokeChar *schar;
    SFG_StrokeFont* font;
    font = ar_fghStrokeByID( fontID );
    freeglut_return_val_if_fail( ( character >= 0 ) &&
                                 ( character < font->Quantity ),
                                 0
    );
    freeglut_return_val_if_fail( font, 0 );
    schar = font->Characters[ character ];
    freeglut_return_val_if_fail( schar, 0 );

    return ( int )( schar->Right + 0.5 );
}

/*
 * Return the width of a string drawn using a stroke font
 */
int ar_glutStrokeLength( void* fontID, const unsigned char* string )
{
    unsigned char c;
    float length = 0.0;
    float this_line_length = 0.0;
    SFG_StrokeFont* font;
    font = ar_fghStrokeByID( fontID );
    freeglut_return_val_if_fail( font, 0 );
    if ( !string || ! *string )
        return 0;

    while( ( c = *string++) )
        if( c < font->Quantity )
        {
            if( c == '\n' ) /* EOL; reset the length of this line */
            {
                if( length < this_line_length )
                    length = this_line_length;
                this_line_length = 0.0;
            }
            else  /* Not an EOL, increment the length of this line */
            {
                const SFG_StrokeChar *schar = font->Characters[ c ];
                if( schar )
                    this_line_length += schar->Right;
            }
        }
    if( length < this_line_length )
        length = this_line_length;
    return( int )( length + 0.5 );
}

/*
 * Returns the height of a stroke font
 */
GLfloat ar_glutStrokeHeight( void* fontID )
{
    SFG_StrokeFont* font;
    font = ar_fghStrokeByID( fontID );
    freeglut_return_val_if_fail( font, 0.0 );
    return font->Height;
}

/*** END OF FILE ***/
