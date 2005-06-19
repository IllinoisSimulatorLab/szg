
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#include "arPrecompiled.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <fstream>

#include "arMath.h"
#include "arTexFont.h"

static int useLuminanceAlpha = 1;

static arTexFont::TexGlyphVertexInfo nullGlyph = {
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  0
};

#define CLAMP( x, a, b ) x = ( x > (b) ) ? (b) : ( ( x < (a) ) ? (a) : x );

// byte swap a 32-bit value
#define SWAPL(x, n) { \
  n = ((char*) (x))[0];\
  ((char*) (x))[0] = ((char*) (x))[3];\
  ((char*) (x))[3] = n;\
  n = ((char *) (x))[1];\
  ((char*) (x))[1] = ((char*) (x))[2];\
  ((char*) (x))[2] = n; }

// byte swap a short
#define SWAPS(x, n) { \
  n = ((char*) (x))[0];\
  ((char*) (x))[0] = ((char*) (x))[1];\
  ((char*) (x))[1] = n; }

arTexFont::TexGlyphVertexInfo* getTCVI( const arTexFont::TexFont* txf, int c )
{
  arTexFont::TexGlyphVertexInfo* tgvi = NULL;

  /* Automatically substitute uppercase letters with lowercase if not
     uppercase available (and vice versa). */
  if( ( c >= txf->min_glyph ) && ( c < txf->min_glyph + txf->range ) ) {
    tgvi = txf->lut[ c - txf->min_glyph ];

    if( tgvi ) {
      return tgvi;
    }

    if( islower( c ) ) {
      c = toupper( c );

      if( ( c >= txf->min_glyph ) && ( c < txf->min_glyph + txf->range ) ) {
        return txf->lut[ c - txf->min_glyph ];
      }
    }

    if( isupper( c ) ) {
      c = tolower( c );

      if( ( c >= txf->min_glyph ) && ( c < txf->min_glyph + txf->range ) ) {
        return txf->lut[ c - txf->min_glyph ];
      }
    }
  }

  return &nullGlyph;

  #if 0
  fprintf(stderr, "texfont: tried to access unavailable font character \"%c\" (%d)\n",
    isprint(c) ? c : ' ', c);
  abort();
  #endif
}

arTexFont::arTexFont( const std::string& font, int rows, int cols ) :
  _currentFont( NULL ),
  _rows( rows ),
  _cols( cols )
{
  if( loadFont( font ) < 0 ) {
    // print warning/error?
  }
}

arTexFont::~arTexFont( void )
{
  std::map<std::string, arTexFont::TexFont*>::iterator itr;

  for( itr = _fonts.begin(); itr != _fonts.end(); itr++ ) {
    unloadFont( itr->first );
  }
}

int arTexFont::loadFont( const std::string& font )
{
  arTexFont::TexFont* txf = NULL;
  FILE* file = NULL;
  GLfloat w, h, xstep, ystep;
  char fileid[ 4 ], tmp;
  unsigned char* texbitmap = NULL;
  int min_glyph, max_glyph;
  int endianness, swap, format, stride, width, height;
  unsigned long got;
  int i, j;

  if( !font.length() ) {
    return -1;
  }

  file = fopen( font.c_str(), "rb" );
  if( file == NULL ) {
    return -1;
  }

  txf = (arTexFont::TexFont*) malloc( sizeof( TexFont ) );
  if( txf == NULL ) {
    fclose( file );
    return -1;
  }

  #define FAILURE_RETURN \
    fclose( file ); \
    unloadFont( txf ); \
    return -1;

  // For easy cleanup in error case.
  txf->tgi = NULL;
  txf->tgvi = NULL;
  txf->lut = NULL;
  txf->teximage = NULL;
  txf->texobj = 0;

  got = fread( fileid, 1, 4, file );
  if( got != 4 || strncmp( fileid, "\377txf", 4 ) ) {
    FAILURE_RETURN;
  }

  assert( sizeof( int ) == 4 );  // Ensure external file format size.
  got = fread( &endianness, sizeof( int ), 1, file );
  if( got == 1 && endianness == 0x12345678 ) {
    swap = 0;
  }
  else if( got == 1 && endianness == 0x78563412 ) {
    swap = 1;
  }
  else {
    FAILURE_RETURN;
  }

  #define EXPECT( n ) if ( got != ( n ) ) { FAILURE_RETURN; }
  got = fread( &format, sizeof( int ), 1, file);
  EXPECT( 1 );
  got = fread( &txf->tex_width, sizeof( int ), 1, file );
  EXPECT( 1 );
  got = fread( &txf->tex_height, sizeof( int ), 1, file );
  EXPECT( 1 );
  got = fread( &txf->max_ascent, sizeof( int ), 1, file );
  EXPECT( 1 );
  got = fread( &txf->max_descent, sizeof( int ), 1, file );
  EXPECT( 1 );
  got = fread( &txf->num_glyphs, sizeof( int ), 1, file );
  EXPECT( 1 );

  if( swap ) {
    SWAPL( &format, tmp );
    SWAPL( &txf->tex_width, tmp );
    SWAPL( &txf->tex_height, tmp );
    SWAPL( &txf->max_ascent, tmp );
    SWAPL( &txf->max_descent, tmp );
    SWAPL( &txf->num_glyphs, tmp );
  }

  txf->tgi = (arTexFont::TexGlyphInfo*) malloc( txf->num_glyphs * sizeof( arTexFont::TexGlyphInfo ) );
  if( txf->tgi == NULL ) {
    FAILURE_RETURN;
  }

  assert( sizeof( arTexFont::TexGlyphInfo ) == 12 );  // Ensure external file format size.
  got = fread( txf->tgi, sizeof( arTexFont::TexGlyphInfo ), txf->num_glyphs, file );
  EXPECT( txf->num_glyphs );

  if( swap ) {
    for( i = 0; i < txf->num_glyphs; i++ ) {
      SWAPS( &txf->tgi[ i ].c, tmp );
      SWAPS( &txf->tgi[ i ].x, tmp );
      SWAPS( &txf->tgi[ i ].y, tmp );
    }
  }

  txf->tgvi = (arTexFont::TexGlyphVertexInfo*) malloc( txf->num_glyphs * sizeof( arTexFont::TexGlyphVertexInfo ) );
  if( txf->tgvi == NULL ) {
    FAILURE_RETURN;
  }

  w = txf->tex_width;
  h = txf->tex_height;
  xstep = 0.5f / w;
  ystep = 0.5f / h;

  for( i = 0; i < txf->num_glyphs; i++ ) {
    arTexFont::TexGlyphInfo* tgi;

    tgi = &txf->tgi[ i ];
    txf->tgvi[ i ].t0[ 0 ] = tgi->x / w + xstep;
    txf->tgvi[ i ].t0[ 1 ] = tgi->y / h + ystep;
    txf->tgvi[ i ].v0[ 0 ] = tgi->xoffset;
    txf->tgvi[ i ].v0[ 1 ] = tgi->yoffset;
    txf->tgvi[ i ].t1[ 0 ] = ( tgi->x + tgi->width ) / w + xstep;
    txf->tgvi[ i ].t1[ 1 ] = tgi->y / h + ystep;
    txf->tgvi[ i ].v1[ 0 ] = tgi->xoffset + tgi->width;
    txf->tgvi[ i ].v1[ 1 ] = tgi->yoffset;
    txf->tgvi[ i ].t2[ 0 ] = ( tgi->x + tgi->width ) / w + xstep;
    txf->tgvi[ i ].t2[ 1 ] = ( tgi->y + tgi->height ) / h + ystep;
    txf->tgvi[ i ].v2[ 0 ] = tgi->xoffset + tgi->width;
    txf->tgvi[ i ].v2[ 1 ] = tgi->yoffset + tgi->height;
    txf->tgvi[ i ].t3[ 0 ] = tgi->x / w + xstep;
    txf->tgvi[ i ].t3[ 1 ] = ( tgi->y + tgi->height ) / h + ystep;
    txf->tgvi[ i ].v3[ 0 ] = tgi->xoffset;
    txf->tgvi[ i ].v3[ 1 ] = tgi->yoffset + tgi->height;
    txf->tgvi[ i ].advance = tgi->advance;
  }

  min_glyph = txf->tgi[ 0 ].c;
  max_glyph = txf->tgi[ 0 ].c;
  for( i = 1; i < txf->num_glyphs; i++ ) {
    if( txf->tgi[ i ].c < min_glyph ) {
      min_glyph = txf->tgi[ i ].c;
    }

    if( txf->tgi[ i ].c > max_glyph ) {
      max_glyph = txf->tgi[ i ].c;
    }
  }

  txf->min_glyph = min_glyph;
  txf->range = max_glyph - min_glyph + 1;

  txf->lut = (arTexFont::TexGlyphVertexInfo **) calloc( txf->range, sizeof( arTexFont::TexGlyphVertexInfo* ) );
  if( txf->lut == NULL ) {
    FAILURE_RETURN;
  }

  for( i = 0; i < txf->num_glyphs; i++ ) {
    txf->lut[ txf->tgi[ i ].c - txf->min_glyph ] = &txf->tgvi[ i ];
  }

  switch( format ) {
    case TXF_FORMAT_BYTE:
      if( useLuminanceAlpha ) {
        unsigned char* orig = (unsigned char*) malloc( txf->tex_width * txf->tex_height );
        if( orig == NULL ) {
          FAILURE_RETURN;
        }

        got = fread( orig, 1, txf->tex_width * txf->tex_height, file );
        // orig leaks if this expect fails
        EXPECT( txf->tex_width * txf->tex_height );

        txf->teximage = (unsigned char*) malloc( 2 * txf->tex_width * txf->tex_height );
        if( txf->teximage == NULL ) {
          free( orig );
          FAILURE_RETURN;
        }

        for( i = 0; i < txf->tex_width * txf->tex_height; i++ ) {
          txf->teximage[ i * 2 ] = orig[ i ];
          txf->teximage[ i * 2 + 1 ] = orig[ i ];
        }

        free( orig );
      }
      else {
        txf->teximage = (unsigned char*) malloc( txf->tex_width * txf->tex_height );
        if( txf->teximage == NULL ) {
          FAILURE_RETURN;
        }

        got = fread( txf->teximage, 1, txf->tex_width * txf->tex_height, file );
        EXPECT( txf->tex_width * txf->tex_height );
      }
    break;

    case TXF_FORMAT_BITMAP:
      width = txf->tex_width;
      height = txf->tex_height;
      stride = ( width + 7 ) >> 3;

      texbitmap = (unsigned char*) malloc( stride * height );
      if( texbitmap == NULL ) {
        FAILURE_RETURN;
      }

      got = fread( texbitmap, 1, stride * height, file );
      // texbitmap leaks if this expect fails
      EXPECT( stride * height );

      if( useLuminanceAlpha ) {
        txf->teximage = (unsigned char*) calloc( width * height * 2, 1 );
        if( txf->teximage == NULL ) {
          FAILURE_RETURN;
        }

        for( i = 0; i < height; i++ ) {
          for( j = 0; j < width; j++ ) {
            if( texbitmap[ i * stride + ( j >> 3 ) ] & ( 1 << ( j & 7 ) ) ) {
              txf->teximage[ ( i * width + j ) * 2 ] = 255;
              txf->teximage[ ( i * width + j ) * 2 + 1 ] = 255;
            }
          }
        }
      }
      else {
        txf->teximage = (unsigned char*) calloc( width * height, 1 );
        if( txf->teximage == NULL ) {
          FAILURE_RETURN;
        }

        for( i = 0; i < height; i++ ) {
          for( j = 0; j < width; j++ ) {
            if( texbitmap[ i * stride + ( j >> 3 ) ] & ( 1 << ( j & 7 ) ) ) {
              txf->teximage[ i * width + j ] = 255;
            }
          }
        }
      }

      free( texbitmap );
    break;
  }

  fclose( file );

  establishTexture( txf, 0, GL_TRUE );

  _fonts[ font ] = txf;

  setCurrentFont( font );

  return 0;
}

int arTexFont::setCurrentFont( const std::string& font )
{
  if( _fonts.find( font ) == _fonts.end() ) {
    return -1;
  }

  _currentFont = _fonts[ font ];

  return 0;
}

GLuint arTexFont::establishTexture( arTexFont::TexFont* txf, GLuint texobj, bool setupMipmaps )
{
  if( txf->texobj == 0 ) {
    if( texobj == 0 ) {
      glGenTextures( 1, &txf->texobj );
    }
    else {
      txf->texobj = texobj;
    }
  }

  glBindTexture( GL_TEXTURE_2D, txf->texobj );

#ifndef AR_USE_WIN_32
  /* XXX Indigo2 IMPACT in IRIX 5.3 and 6.2 does not support the GL_INTENSITY
     internal texture format. Sigh. Win32 non-GLX users should disable this
     code. */
  if( useLuminanceAlpha == 0 ) {
    char *vendor, *renderer, *version;

    renderer = (char*) glGetString( GL_RENDERER );
    vendor = (char*) glGetString( GL_VENDOR );

    if( !strcmp( vendor, "SGI" ) && !strncmp( renderer, "IMPACT", 6 ) ) {
      version = (char*) glGetString( GL_VERSION );

      if( !strcmp( version, "1.0 Irix 6.2" ) || !strcmp( version, "1.0 Irix 5.3" ) ) {
        unsigned char* latex;
        int width = txf->tex_width;
        int height = txf->tex_height;

        useLuminanceAlpha = 1;
        latex = (unsigned char*) calloc( width * height * 2, 1 );  // unprotected alloc

        for( int i = 0; i < height * width; i++ ) {
          latex[ i * 2 ] = txf->teximage[ i ];
          latex[ i * 2 + 1 ] = txf->teximage[ i ];
        }

        free( txf->teximage );
        txf->teximage = latex;
      }
    }
  }
#endif

  if( useLuminanceAlpha ) {
    if( setupMipmaps ) {
      gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE_ALPHA,
                         txf->tex_width, txf->tex_height,
                         GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, txf->teximage );
    }
    else {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                    txf->tex_width, txf->tex_height, 0,
                    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, txf->teximage );
    }
  }
  else {
    if( setupMipmaps ) {
      gluBuild2DMipmaps( GL_TEXTURE_2D, GL_INTENSITY4,
                         txf->tex_width, txf->tex_height,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE, txf->teximage);
    }
    else {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_INTENSITY4,
                    txf->tex_width, txf->tex_height, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, txf->teximage);
    }
  }

  return txf->texobj;
}

int arTexFont::unloadFont( const std::string& font )
{
  if( _fonts.find( font ) == _fonts.end() ) {
    return -1;
  }

  return unloadFont( _fonts[ font ] );
}

int arTexFont::unloadFont( arTexFont::TexFont* txf )
{
  if( !txf ) {
    return -1;
  }

  if( txf == _currentFont ) {
    _currentFont = NULL;
  }

  if( txf->teximage ) {
    free( txf->teximage );
  }

  if( txf->tgi ) {
    free( txf->tgi );
  }

  if( txf->tgvi ) {
    free( txf->tgvi );
  }

  if( txf->lut ) {
    free( txf->lut );
  }

  if( txf->texobj ) {
    glDeleteTextures( 1, &txf->texobj );
  }

  free( txf );

  return 0;
}

int arTexFont::getStringMetrics( const std::string& text,
                                  int& width, int& max_ascent, int& max_descent)
{
  if( !_currentFont ) {
    return -1;
  }

  int w = 0;

  for( unsigned int i = 0; i < text.length(); i++)  {
    w += getTCVI( _currentFont, text[ i ] )->advance;
  }

  width = w;
  max_ascent = _currentFont->max_ascent;
  max_descent = _currentFont->max_descent;

  return 0;
}

void arTexFont::setupOpenGL( bool setMatrices2D, GLbitfield attrib, GLfloat filter,
                             GLint texFunc, GLenum alphaFunc,
                             GLenum srcFunc, GLenum destFunc,
                             bool alphaTest, bool blending,
                             bool lighting, bool depthTest ) {
  if( !_currentFont ) {
    return;
  }

  glPushAttrib( attrib );

  glEnable( GL_TEXTURE_2D );

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texFunc );

  glBindTexture( GL_TEXTURE_2D, _currentFont->texobj );

  if( alphaTest ) {
    glEnable( GL_ALPHA_TEST );
  }
  else {
    glDisable( GL_ALPHA_TEST );
  }

  if( blending ) {
    glEnable( GL_BLEND );
    glAlphaFunc( alphaFunc, 0.0625f );
    glBlendFunc( srcFunc, destFunc );
  }
  else {
    glDisable( GL_BLEND );
  }

  if( lighting ) {
    glEnable( GL_LIGHTING );
  }
  else {
    glDisable( GL_LIGHTING );
  }

  if( depthTest ) {
    glEnable( GL_DEPTH_TEST );
  }
  else {
    glDisable( GL_DEPTH_TEST );
  }

  if( setMatrices2D ) {
    int view[ 4 ];
    glGetIntegerv( GL_VIEWPORT, view );

    glMatrixMode( GL_PROJECTION );
  	glPushMatrix();
  	glLoadIdentity();
  	glOrtho( 0, view[ 2 ], 0, view[ 3 ], -1, 1 );

  	glMatrixMode( GL_MODELVIEW );
  	glPushMatrix();
  	glLoadIdentity();
  }
}

void arTexFont::tearDownOpenGL( bool popMatrices ) {
  if( popMatrices ) {
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
  }

  glPopAttrib();
}

float arTexFont::renderGlyph( int c, bool advance )
{
  arTexFont::TexGlyphVertexInfo* tgvi = getTCVI( _currentFont, c );

  glBegin( GL_QUADS );
    glTexCoord2fv( tgvi->t0 );
    glVertex2sv(   tgvi->v0 );
    glTexCoord2fv( tgvi->t1 );
    glVertex2sv(   tgvi->v1 );
    glTexCoord2fv( tgvi->t2 );
    glVertex2sv(   tgvi->v2 );
    glTexCoord2fv( tgvi->t3 );
    glVertex2sv(   tgvi->v3 );
  glEnd();

  if( advance ) {
    glTranslatef( tgvi->advance, 0.0f, 0.0f );
  }

  return tgvi->advance;
}

int arTexFont::renderString( const std::string& text )
{
  if( !_currentFont ) {
    return -1;
  }

  setupOpenGL( false );

  for ( unsigned int i = 0; i < text.length(); i++ ) {
    renderGlyph( text[ i ] );
  }

  tearDownOpenGL( false );

  return 0;
}

int arTexFont::renderString2D( const std::string& text,
                                float posX, float posY, float scaleX, float scaleY,
                                bool scalePerGlyphX, bool scalePerGlyphY )
{
  if( !_currentFont ) {
    return -1;
  }

  CLAMP( posX, 0.0f, 1.0f );
  CLAMP( posY, 0.0f, 1.0f );

  CLAMP( scaleX, 0.0f, 1.0f );
  CLAMP( scaleY, 0.0f, 1.0f );

  int view[ 4 ];
  glGetIntegerv( GL_VIEWPORT, view );

  setupOpenGL();

	glTranslatef( posX * float( view[ 2 ] ), posY * float( view[ 3 ] ), 0.0f );

	int width = 0, ascent = 0, descent = 0, fontHeight = 0;

	if( !scalePerGlyphX || !scalePerGlyphY ) {
  	getStringMetrics( text, width, ascent, descent );

  	fontHeight = ascent + descent;

  	float newScaleX = scaleX * float( view[ 2 ] ) / float( width );
  	float newScaleY = scaleY * float( view[ 3 ] ) / float( fontHeight );

  	glScalef( scalePerGlyphX ? 1.0f : newScaleX, scalePerGlyphY ? 1.0f : newScaleY, 1.0f );
	}

	float lineWidth = 0.0f;

  for( unsigned int i = 0; i < text.length(); i++ ) {
    float glyphWidth = 0.0f, glyphHeight = 0.0f;
    float perScaleX = 1.0f, perScaleY = 1.0f;

    if( scalePerGlyphX || scalePerGlyphY ) {
      arTexFont::TexGlyphVertexInfo* tgvi = getTCVI( _currentFont, int( text[ i ] ) );

      glyphWidth = float( tgvi->v1[ 0 ] - tgvi->v0[ 0 ] );
      glyphHeight = float( tgvi->v3[ 1 ] - tgvi->v0[ 1 ] );

      perScaleX = scaleX * float( view[ 2 ] ) / tgvi->advance;
      perScaleY = scaleY * float( view[ 3 ] ) / glyphHeight;
    }

    if( ( text[ i ] == '\012' ) || ( text[ i ] == '\015' ) ) {
      glTranslatef( -lineWidth, scalePerGlyphY ? -scaleY * float( view[ 3 ] ) : -fontHeight, 0.0f );
      lineWidth = 0.0f;

      if( text[ i ] == '\015' && ( ( i + 1 ) < text.length() ) && ( text[ i + 1 ] == '\012' ) ) {
        i++;
      }

      continue;
    }

    if( scalePerGlyphX || scalePerGlyphY ) {
      glMatrixMode( GL_MODELVIEW );
      glPushMatrix();

      /*
      std::cout << "v0: (" << tgvi->v0[ 0 ] << ", " << tgvi->v0[ 1 ] << ")" << std::endl;
      std::cout << "v1: (" << tgvi->v1[ 0 ] << ", " << tgvi->v1[ 1 ] << ")" << std::endl;
      std::cout << "v2: (" << tgvi->v2[ 0 ] << ", " << tgvi->v2[ 1 ] << ")" << std::endl;
      std::cout << "v3: (" << tgvi->v3[ 0 ] << ", " << tgvi->v3[ 1 ] << ")" << std::endl;
      std::cout << "advance: " << tgvi->advance << std::endl;

      std::cout << "glyphWidth: " << glyphWidth << " glyphHeight: " << glyphHeight << std::endl;

      std::cout << "scaleX: " << perScaleX << " scaleY: " << perScaleY << std::endl;
      */

      glScalef( scalePerGlyphX ? perScaleX : 1.0f, scalePerGlyphY ? perScaleY : 1.0f, 1.0f );
    }

    float advance = renderGlyph( text[ i ] );
    lineWidth += scalePerGlyphX ? scaleX * float( view[ 2 ] ) : advance;

    if( scalePerGlyphX || scalePerGlyphY ) {
      glPopMatrix();
      glTranslatef( scalePerGlyphX ? scaleX * float( view[ 2 ] ) : advance, 0.0f, 0.0f );
    }
  }

  tearDownOpenGL();

  return 0;
}

int arTexFont::renderFile2D( const std::string& filename,
                              float posX, float posY, float scaleX, float scaleY,
                              bool scalePerGlyphX, bool scalePerGlyphY )
{
  std::ifstream file( filename.c_str(), std::ios::in );

  if( !file.is_open() ) {
    return -1;
  }

  // get length of file:
  file.seekg( 0, ios::end );
  int length = file.tellg();
  file.seekg( 0, ios::beg );

  char* buf = new char[ length ];

  // read data as a block:
  file.read( buf, length );

  file.close();

  std::string text( buf );

  delete buf;

  return renderString2D( text, posX, posY, scaleX, scaleY, scalePerGlyphX, scalePerGlyphY );
}

int arTexFont::renderStringCurses( const std::string& text, int row, int col,
                                   bool vertical )
{
  // clamp row and col
  CLAMP( row, 0, _rows - 1 );
  CLAMP( col, 0, _cols - 1 );

  int view[ 4 ];
  glGetIntegerv( GL_VIEWPORT, view );

  setupOpenGL();

  float cellWidth = float( view[ 2 ] ) / float( _cols );
  float cellHeight = float( view[ 3 ] ) / float( _rows );

  for( unsigned int i = 0; i < text.length(); i++ ) {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();

    glTranslatef( float( col ) * cellWidth, float( ( _rows - 1 ) - row ) * cellHeight, 0.0f );

    arTexFont::TexGlyphVertexInfo* tgvi = getTCVI( _currentFont, int( text[ i ] ) );

    float glyphWidth = float( tgvi->v1[ 0 ] - tgvi->v0[ 0 ] );
    float glyphHeight = float( tgvi->v3[ 1 ] - tgvi->v0[ 1 ] );

    float scaleX = cellWidth / tgvi->advance;
    float scaleY = cellHeight / glyphHeight;

    glScalef( scaleX, scaleY, 1.0f );

    renderGlyph( text[ i ], false );

    glPopMatrix();

    if( vertical ) {
      if( ++row == _rows ) {
        row = 0;
        col++;
      }
    }
    else {
      if( ++col == _cols ) {
        col = 0;
        row++;
      }
    }
  }

  tearDownOpenGL();

  return 0;
}

/*
enum {
  MONO, TOP_BOTTOM, LEFT_RIGHT, FOUR
};

void txfRenderFancyString( const TexFont * txf, char *string, int len)
{
  TexGlyphVertexInfo *tgvi;
  GLubyte c[4][3];
  int mode = MONO;
  int i;

  for (i = 0; i < len; i++) {
    if (string[i] == 27) {
      switch (string[i + 1]) {
      case 'M':
        mode = MONO;
        glColor3ubv((GLubyte *) & string[i + 2]);
        i += 4;
        break;
      case 'T':
        mode = TOP_BOTTOM;
        memcpy(c, &string[i + 2], 6);
        i += 7;
        break;
      case 'L':
        mode = LEFT_RIGHT;
        memcpy(c, &string[i + 2], 6);
        i += 7;
        break;
      case 'F':
        mode = FOUR;
        memcpy(c, &string[i + 2], 12);
        i += 13;
        break;
      }
    } else {
      switch (mode) {
      case MONO:
        txfRenderGlyph(txf, string[i]);
        break;
      case TOP_BOTTOM:
        tgvi = getTCVI(txf, string[i]);
        glBegin(GL_QUADS);
        glColor3ubv(c[0]);
        glTexCoord2fv(tgvi->t0);
        glVertex2sv(tgvi->v0);
        glTexCoord2fv(tgvi->t1);
        glVertex2sv(tgvi->v1);
        glColor3ubv(c[1]);
        glTexCoord2fv(tgvi->t2);
        glVertex2sv(tgvi->v2);
        glTexCoord2fv(tgvi->t3);
        glVertex2sv(tgvi->v3);
        glEnd();
        glTranslatef(tgvi->advance, 0.0, 0.0);
        break;
      case LEFT_RIGHT:
        tgvi = getTCVI(txf, string[i]);
        glBegin(GL_QUADS);
        glColor3ubv(c[0]);
        glTexCoord2fv(tgvi->t0);
        glVertex2sv(tgvi->v0);
        glColor3ubv(c[1]);
        glTexCoord2fv(tgvi->t1);
        glVertex2sv(tgvi->v1);
        glColor3ubv(c[1]);
        glTexCoord2fv(tgvi->t2);
        glVertex2sv(tgvi->v2);
        glColor3ubv(c[0]);
        glTexCoord2fv(tgvi->t3);
        glVertex2sv(tgvi->v3);
        glEnd();
        glTranslatef(tgvi->advance, 0.0, 0.0);
        break;
      case FOUR:
        tgvi = getTCVI(txf, string[i]);
        glBegin(GL_QUADS);
        glColor3ubv(c[0]);
        glTexCoord2fv(tgvi->t0);
        glVertex2sv(tgvi->v0);
        glColor3ubv(c[1]);
        glTexCoord2fv(tgvi->t1);
        glVertex2sv(tgvi->v1);
        glColor3ubv(c[2]);
        glTexCoord2fv(tgvi->t2);
        glVertex2sv(tgvi->v2);
        glColor3ubv(c[3]);
        glTexCoord2fv(tgvi->t3);
        glVertex2sv(tgvi->v3);
        glEnd();
        glTranslatef(tgvi->advance, 0.0, 0.0);
        break;
      }
    }
  }
}
*/
