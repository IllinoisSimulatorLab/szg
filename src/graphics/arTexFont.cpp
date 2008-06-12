/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#include "arPrecompiled.h"
#include "arTexFont.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>

// Inefficient, but it won't be parsing long strings.
list<string> ar_parseLineBreaks(const string& text) {
  list<string> result;
  string currentLine("");
  char lastChar = '\0';
  int stringHead = 0;
  for (unsigned i=0; i<text.length(); i++) {
    if (text[i] == '\n') {
      if (lastChar != '\r') {
        result.push_back(text.substr(stringHead, i-stringHead));
      }
      stringHead = i+1;
    }
    else if (text[i] == '\r') {
      if (lastChar != '\n') {
        result.push_back(text.substr(stringHead, i-stringHead));
      }
      stringHead = i+1;
    }
    else if (i == text.length()-1) {
      result.push_back(text.substr(stringHead, i+1-stringHead));
    }
    lastChar = text[i];
  }
  return result;
}

arTexFont::arTexFont() {
}

arTexFont::~arTexFont() {
}

float arTexFont::characterWidth() {
  return _charWidth;
}

// The line can have extra spacing, as given by the format.
float arTexFont::lineHeight(arTextBox& format) {
  return _charHeight*format.lineSpacing;
}

float arTexFont::characterHeight() {
  return _charHeight;
}

bool arTexFont::load( const string& fontFilePath, int transparentColor ) {
  const bool ok =_fontTexture.readImage( fontFilePath, transparentColor );
  if (ok) {
    _fontTexture.setTextureFunc(GL_MODULATE);
    _fontTexture.mipmap(true);
  }
  return ok;
}

void arTexFont::setFontTexture( const arTexture& newFont ) {
  _fontTexture = newFont;
}

void arTexFont::lineFeed(int& currentColumn, int& currentRow, arTextBox& format) {
  glTranslatef( -currentColumn*characterWidth(), -lineHeight(format), 0);
  currentColumn = 0;
  currentRow++;
}

void arTexFont::advanceCursor(int& currentColumn, int& currentRow, arTextBox& format) {
  glTranslatef( characterWidth(), 0, 0 );
  currentColumn++;
  if (currentColumn >= format.columns) {
    lineFeed( currentColumn, currentRow, format );
  }
}

void arTexFont::renderGlyph(int c, int& currentColumn, int& currentRow, arTextBox& format) {
  // Handle whitespace. Wrap lines. ar_parseLineBreaks() already eliminated cr/lf's.
  if (c == ' ') {
    advanceCursor(currentColumn, currentRow, format);
  }
  else if (c == '\t') {
    for (int i=0; i<format.tabWidth; i++)
      advanceCursor(currentColumn, currentRow, format);
  } else {
    // Our font is encoded in a single texture. It is divided into a 16x16 grid, with the 0th character in the upper
    // left corner, the 1st character one to the right, the 16th character 1 below the 0th, etc.
    float texX = (c%16)/16.0;
    float texY = (15.0 - c/16)/16.0;
    float delta = 1/16.0;
    // The character is centered in its box, with half a width on either side.
    float charOffsetX = (1.0-_charWidth)/2.0; // Depends on the font's texture.
    // We also want to pop in ever so slightly (in y), to deal with rounding error and characters like _!
    float charOffsetY = 0.015; // Depends on the font's texture.
    float charHeight = 1.0 - 2*charOffsetY;
    glBegin( GL_QUADS );
    glTexCoord2f( texX+charOffsetX*delta, texY+charOffsetY*delta );
    glVertex3f(   0, charOffsetY, 0 );
    glTexCoord2f( texX+(charOffsetX+_charWidth)*delta, texY+charOffsetY*delta );
    glVertex3f(   _charWidth, 0, 0 );
    glTexCoord2f( texX+(charOffsetX+_charWidth)*delta, texY+(charOffsetY+charHeight)*delta );
    glVertex3f(   _charWidth, charOffsetY+charHeight, 0 );
    glTexCoord2f( texX+charOffsetX*delta , texY+(charOffsetY+charHeight)*delta);
    glVertex3f(   0, charOffsetY+charHeight, 0 );
    glEnd();
    advanceCursor(currentColumn, currentRow, format);
  }
}

float arTexFont::getTextWidth( const string& text, arTextBox& format) {
  float width = 0;
  float height = 0;
  getTextMetrics(text, format, width, height);
  return width;
}

float arTexFont::getTextHeight( const string& text, arTextBox& format) {
  float width = 0;
  float height = 0;
  getTextMetrics(text, format, width, height);
  return height;
}

void arTexFont::getTextMetrics(const string& text, arTextBox& format, float& width, float& height) {
  list<string> parse = ar_parseLineBreaks(text);
  getTextMetrics(parse, format, width, height);
}

void arTexFont::getTextMetrics(list<string>& parse, arTextBox& format, float& width, float& height) {
  int maxColumns = -1;
  int rows = 0;
  for (list<string>::iterator i = parse.begin(); i != parse.end(); i++) {
    if (i->length() >= unsigned(format.columns)) {
      // There is overflow on the line.
      rows += i->length() / format.columns;
      // Handle the case where the line isn't a multiple of the column width.
      if (i->length()%format.columns != 0) {
	++rows;
      }
    }
    else{
      // No overflow for this line.
      ++rows;
    }
    // Our text box is taking up all the format columns if any row does.
    if (i->length() >= unsigned(format.columns)) {
      maxColumns = format.columns;
    }
    // If the row does not take up the whole width, only increase maxColumns if the line is bigger.
    else if (maxColumns < 0 || i->length() > unsigned(maxColumns)) {
      maxColumns = i->length();
    }
  }
  // We know the number of rows and columns. compute the floating point size.
  width = (maxColumns <= 0) ? 0 :
    format.width * float(maxColumns) / float(format.columns);
  // How tall is a character in screen units?
  const float actualCharHeight = format.width*characterHeight()/(format.columns*characterWidth());
  // How tall is a line (counting the extra spacing) in screen units?
  const float actualLineHeight = format.width*lineHeight(format)/(format.columns*characterWidth());
  height = (rows == 0) ? 0 :
    // All rows except the last one have extra spacing, given by format.
    actualCharHeight + (rows-1)*actualLineHeight;
}

void arTexFont::renderString(const string& text, arTextBox& format ) {
  // Breaks our text up into a collection of lines.
  list<string> parse = ar_parseLineBreaks(text);
  renderText(parse, format);
}

void arTexFont::renderText(list<string>& parse, arTextBox& format ) {
  glPushAttrib(GL_LIGHTING | GL_BLEND);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // These parameters seem to give the best text minimization.
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  _fontTexture.activate();
  // The character width and height are in the character coordinate system.
  _charHeight = 1;  // This is fixed by convention.
  _charWidth = 0.6; // This actually depends on the font's texture.
  float scl = format.width/(format.columns*_charWidth);
  glPushMatrix();
  // Put the cursor in the initial spot for character rendering. The upper left hand corner
  // of the text box.
  glTranslatef(format.upperLeft[0], format.upperLeft[1], format.upperLeft[2]);
  // Transform into the character coordinate system.
  glScalef(scl, scl, scl);
  int column = 0;
  int row = 0;
  // Move down a little bit so the top of our character hits the top of the text box (drawing of the
  // character is from the "bottom").
  glTranslatef(0, -characterHeight(), 0);
  // Render each row. The renderGlyph function handles advancing the cursor after displaying the
  // character. It also handles advancing the cursor for spaces, tabs, and line overflows (when there
  // are more characters on a line than the text box has columns.
  for (list<string>::iterator i = parse.begin();
       i != parse.end(); i++) {
    for ( unsigned int j = 0; j < (*i).length(); j++ ) {
      glColor3f(format.color[0], format.color[1], format.color[2]);
      renderGlyph( (*i)[j], column, row, format );
    }
    // Do not line feed if cursor advance already did so. True if length is an integer mutliple
    // of columns.
    if ((*i).length()%format.columns != 0) {
      // Increments the row, returns the column to 0, and makes the appropriate glTranslatef.
      lineFeed(column, row, format);
    }
  }
  glPopMatrix();
  _fontTexture.deactivate();
  glDisable(GL_BLEND);
  glPopAttrib();
}

bool arTexFont::renderFile(const string& filename, arTextBox& format) {
  std::ifstream file(filename.c_str(), std::ios::in);
  if (!file.is_open()) {
    return false;
  }
  // Get length of file:
  file.seekg(0, ios::end);
  int length = file.tellg();
  file.seekg(0, ios::beg);
  char* buf = new char[ length ];
  // Read data as a block:
  file.read(buf, length);
  file.close();
  string text(buf);
  delete buf;
  renderString(text, format);
  return true;
}
