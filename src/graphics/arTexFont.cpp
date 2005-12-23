
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
#include <list>

#include "arMath.h"
#include "arTexFont.h"

/// Not very efficient, but we're essentially assuming that it won't be parsing
/// *very* long amounts of text.
list<string> ar_parseLineBreaks(const string& text){
  list<string> result;
  string currentLine("");
  char lastChar('\0');
  int stringHead = 0;
  for (int i=0; i<text.length(); i++){
    if (text[i] == '\n'){
      if (lastChar != '\r'){
        result.push_back(text.substr(stringHead, i-stringHead));
      }
      stringHead = i+1;
    }
    else if (text[i] == '\r'){
      if (lastChar != '\n'){
        result.push_back(text.substr(stringHead, i-stringHead));
      }
      stringHead = i+1;
    }
    else if (i == text.length()-1){
      result.push_back(text.substr(stringHead, i+1-stringHead));
    }
    lastChar = text[i];
  }
  return result;
}

arTexFont::arTexFont() 
{
}

arTexFont::~arTexFont()
{
}

float arTexFont::characterWidth(){
  return _charWidth;
}

float arTexFont::characterHeight(arTextBox& format){
  return _charHeight*format.lineSpacing;
}

bool arTexFont::load( const std::string& font ){
  // Black pixels should be transparent.
  bool state =_fontTexture.readPPM( font, 0);
  // ppm images are flipped as read in by the current buggy software.
  if (state){
    _fontTexture.setTextureFunc(GL_MODULATE);
    _fontTexture.flipHorizontal();
    _fontTexture.mipmap(true);
  }
  return state;
  
}

void arTexFont::moveCursor(int column, int row, arTextBox& format){
  // The bottom-left corner of the character is its origin for rendering.
  // BUG BUG BUG BUG BUG BUG BUG BUG???? Is this precisely right for a line spacing that isn't equal to 1?
  glTranslatef(column*characterWidth(), 
	       -(row+1)*characterHeight(format), 
	       0);
}

void arTexFont::lineFeed(int& currentColumn, int& currentRow, arTextBox& format){
  // Line feed.
  glTranslatef( -currentColumn*characterWidth(), -characterHeight(format), 0);
  currentColumn = 0;
  currentRow++;
}

void arTexFont::advanceCursor( int& currentColumn, int& currentRow, arTextBox& format ){
  glTranslatef( characterWidth(), 0, 0 );
  currentColumn++;
  if (currentColumn >= format.columns){
    lineFeed( currentColumn, currentRow, format );
  }
}

void arTexFont::renderGlyph( int c, int& currentColumn, int& currentRow, arTextBox& format ){
  // Deal with whitespace. Go ahead and wrap lines.
  if (c == ' ' || c == '\t'){
    if (c == ' '){
      advanceCursor(currentColumn, currentRow, format);
    }
    else{
      for (int i=0; i<format.tabWidth; i++){
	advanceCursor(currentColumn, currentRow, format);
      }
    }
  }
  else{
    float texX = (c%16)/16.0;
    float texY = (15.0 - c/16)/16.0;
    float delta = 1/16.0;
    // The character is centered in its box, with half a width on either side.
    float charOffsetX = (1.0-_charWidth)/2.0;
    // We also want to pop in ever so slightly (in y), to deal with rounding error and characters like _!
    float charOffsetY = 0.015;
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


int arTexFont::renderString( const std::string& text, arTextBox& format ){
  // Breaks our text up into a collection of lines.
  list<string> parse = ar_parseLineBreaks(text);
  glPushAttrib(GL_LIGHTING | GL_BLEND);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  _fontTexture.activate();
  _charHeight = 1;
  _charWidth = 0.6;
  float scl = format.width/(format.columns*_charWidth);
  glPushMatrix();
  glTranslatef(format.upperLeft[0], format.upperLeft[1], format.upperLeft[2]);
  glScalef(scl, scl, scl);
  int column = 0;
  int row = 0;
  moveCursor(column,row,format);
  for (list<string>::iterator i = parse.begin();
       i != parse.end(); i++){
    for ( unsigned int j = 0; j < (*i).length(); j++ ) {
      glColor3f(0,1,0);
      renderGlyph( (*i)[j], column, row, format );
    }
    // Do not line feed if cursor advance already did so.
    if ((*i).length() != format.columns){
      lineFeed(column, row, format);
    }
  }
  glPopMatrix();
  _fontTexture.deactivate();
  glDisable(GL_BLEND);
  glPopAttrib();
  return 0;
}

/*int arTexFont::renderFile2D( const std::string& filename,
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
}*/
