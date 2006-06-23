
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#ifndef _AR_TEX_FONT_H__
#define _AR_TEX_FONT_H__

#include "arMath.h"
#include "arTexture.h"
#include "arGraphicsHeader.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <string>
#include <map>

SZG_CALL list<string> ar_parseLineBreaks(const string& text);

// Description of a 3D rectangle upon which text can get printed.
// Passed in to the arTexFont renderString command.
// Also includes tab-width info.

class SZG_CALL arTextBox{
 public:
  // NOTE: The height of the text box is, essentially, calculated
  // from the physical width and the proportions of the font.
  arTextBox():
    color(1,1,1),
    tabWidth(2),
    lineSpacing(1.2),
    columns(80),
    width(2.0),
    upperLeft(0,0,0){}
  ~arTextBox(){}

  arVector3 color;
  int tabWidth;
  float lineSpacing;
  int columns;
  float width;
  arVector3 upperLeft;
};

class SZG_CALL arTexFont
{
  public:
    arTexFont();
    ~arTexFont();
    
    bool load(const string& font);
    float characterWidth();
    float lineHeight(arTextBox& format);
    float characterHeight();
    void moveCursor(int column, int row, arTextBox& format);
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
