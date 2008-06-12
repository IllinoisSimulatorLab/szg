/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#ifndef _AR_TEX_FONT_H__
#define _AR_TEX_FONT_H__

#include "arMath.h"
#include "arTexture.h"
#include "arGraphicsHeader.h"
#include "arGraphicsCalling.h"

#include <string>
#include <map>
#include <list>

SZG_CALL list<string> ar_parseLineBreaks(const string& text);

// Description of a 3D rectangle upon which text can get printed.
// Passed in to the arTexFont renderString command.
// Also includes tab-width info.

class SZG_CALL arTextBox {
 public:
  // NOTE: The height of the text box is, essentially, calculated
  // from the physical width and the proportions of the font.
  arTextBox( float w=2.0, int cols=80, float spacing=1.2,
             arVector3 upLeft=arVector3(0, 0, 0), arVector3 col=arVector3(1, 1, 1),
             int tabW=2 ) :
    width( w ),
    columns( cols ),
    lineSpacing( spacing ),
    upperLeft( upLeft ),
    color( col ),
    tabWidth( tabW ) {
    }
  ~arTextBox() {}

  float width;
  int columns;
  float lineSpacing;
  arVector3 upperLeft;
  arVector3 color;
  int tabWidth;
};

class SZG_CALL arTexFont
{
  public:
    arTexFont();
    ~arTexFont();

    bool load( const string& fontFilePath,
               int transparentColor=0 );
    void setFontTexture( const arTexture& newFont );
    float characterWidth();
    float lineHeight(arTextBox& format);
    float characterHeight();
    void lineFeed(int& currentColumn, int& currentRow, arTextBox& format);
    void advanceCursor(int& currentColumn, int& currentRow, arTextBox& format);
    void renderGlyph(int c, int& currentColumn, int& currentRow, arTextBox& format);
    float getTextWidth(const string& text, arTextBox& format);
    float getTextHeight(const string& text, arTextBox& format);
    void getTextMetrics(const string& text, arTextBox& format, float& width, float& height);
    void getTextMetrics(list<string>& parse, arTextBox& format, float& width, float& height);
    void renderString(const string& text, arTextBox& format);
    void renderText(list<string>& parse, arTextBox& format );
    bool renderFile(const string& filename, arTextBox& format);

  private:
    arTexture _fontTexture;
    float _charWidth;
    float _charHeight;
};

#endif
