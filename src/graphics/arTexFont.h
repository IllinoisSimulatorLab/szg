
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#ifndef _AR_TEX_FONT_H__
#define _AR_TEX_FONT_H__

#include "arMath.h"
#include "arGraphicsHeader.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <string>
#include <map>

#define TXF_FORMAT_BYTE		0
#define TXF_FORMAT_BITMAP	1

#define GLATTRIBS GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | \
                  GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT

/// Description of a 3D rectangle upon which text can get printed.
/// Passed in to the arTexFont renderString command. This also
/// includes some formating info, namely a tab's width.
class SZG_CALL arTextBox{
 public:
  // NOTE: The height of the text box is, essentially, calculated
  // from the physical width and the proportions of the font.
  // NOTE: The normal is assumed to point AWAY from the rect's text side.
  arTextBox():
    tabWidth(2),
    lineSpacing(1.2),
    columns(80),
    rows(20),
    width(2.0),
    upperLeft(0,0,0){}
  ~arTextBox(){}

  int tabWidth;
  float lineSpacing;
  int columns;
  int rows;
  float width;
  arVector3 upperLeft;
};

class SZG_CALL arTexFont
{
  public:

    typedef struct {
      unsigned short c;       // Potentially support 16-bit glyphs.
      unsigned char width;
      unsigned char height;
      signed char xoffset;
      signed char yoffset;
      signed char advance;
      char dummy;             // Space holder for alignment reasons.
      short x;
      short y;
    } TexGlyphInfo;

    typedef struct {
      GLfloat t0[ 2 ];
      GLshort v0[ 2 ];
      GLfloat t1[ 2 ];
      GLshort v1[ 2 ];
      GLfloat t2[ 2 ];
      GLshort v2[ 2 ];
      GLfloat t3[ 2 ];
      GLshort v3[ 2 ];
      GLfloat advance;
    } TexGlyphVertexInfo;

    typedef struct {
      GLuint texobj;
      int tex_width;
      int tex_height;
      int max_ascent;
      int max_descent;
      int num_glyphs;
      int min_glyph;
      int range;
      unsigned char *teximage;
      TexGlyphInfo *tgi;
      TexGlyphVertexInfo *tgvi;
      TexGlyphVertexInfo **lut;
    } TexFont;

    arTexFont( const std::string& font = "", int rows = 80, int cols = 80 );
    ~arTexFont( void );

    int loadFont( const std::string& font );

    int unloadFont( const std::string& font );

    int setCurrentFont( const std::string& font );

    int getStringMetrics( const std::string& text,
                          int& width, int& max_ascent, int& max_descent );
    
    float characterWidth();
    
    float characterHeight(arTextBox& format);
    
    void moveCursor(int column, int row, arTextBox& format);
    
    void lineFeed(int& currentColumn, int& currentRow, arTextBox& format);
    
    void advanceCursor( int& currentColumn, int& currentRow, arTextBox& format );
    
    void renderGlyph( int c, int& currentColumn, int& currentRow, arTextBox& format );
    
    int renderString( const std::string& text, arTextBox& format );

    int renderString2D( const std::string& text,
                        float posX, float posY, float scaleX = 1.0f, float scaleY = 1.0f,
                        bool scalePerGlyphX = false, bool scalePerGlyphY = false );

    int renderFile2D( const std::string& filename,
                      float posX, float posY, float scaleX = 1.0f, float scaleY = 1.0f,
                      bool scalePerGlyphX = false, bool scalePerGlyphY = false );

    int renderStringCurses( const std::string& text, int row = 0, int col = 0,
                            bool vertical = false );

    // void txfRenderFancyString( const TexFont * txf, char *string, int len);

    void setRows( int rows )  { _rows = rows; }
    void setCols( int cols )  { _cols = cols; }

    int getRows( void )  { return _rows; }
    int getCols( void )  { return _cols; }

  private:

    void setupOpenGL( bool setMatrices2D = true, GLbitfield attrib = GLATTRIBS,
                      GLfloat filter = GL_LINEAR_MIPMAP_LINEAR,
                      GLint texFunc = GL_MODULATE,
                      GLenum alphaFunc = GL_GEQUAL,
                      GLenum srcFunc = GL_SRC_ALPHA,
                      GLenum destFunc = GL_ONE_MINUS_SRC_ALPHA,
                      bool alphaTest = true, bool blending = true,
                      bool lighting = false, bool depthTest = false );

    void tearDownOpenGL( bool popMatrices = true );

    int unloadFont( arTexFont::TexFont* txf );

    GLuint establishTexture( arTexFont::TexFont* txf, GLuint texobj = 0, bool setupMipmaps = true );

    float renderGlyph( int c, bool advance = true );

    std::map<std::string, TexFont* > _fonts;

    arTexFont::TexFont* _currentFont;

    int _rows, _cols;
    
    int _charWidth;
    int _charHeight;

};

#endif
