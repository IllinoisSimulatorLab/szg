//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arXMLUtilities.h"
#include "arBuffer.h"
#include <iostream>
using namespace std;

/// Take an arTextStream and skip through any whitespace at the beginning.
/// If we hit EOF before the whitespace ends, return false. Otherwise,
/// use ar_ungetc so that the next character in the stream will be the
/// first non-whotespace character and return true.
bool ar_ignoreWhitespace(arTextStream* textStream){
  // IGNORE WHITESPACE! OK... so we've got:
  // ' ' = space
  // '\n' = linefeed
  // '\r' = 13 = carriage return (this doesn't really exist in Unix
  //   text files, but definitely exists in Windows text files)
  // 9 = horizontal tab 
  int ch = ' ';
  while (ch == ' ' || ch == '\n' || ch == 13 || ch == 9){
    ch = textStream->ar_getc();
    if (ch == EOF){
      // DO NOT PRINT OUT ANYTHING HERE!!
      return false;
    }
  }
  textStream->ar_ungetc(ch);
  return true;
}

/// Take an arTextStream and record any characters between the current
/// position and the next occuring '<'. These characters are stuffed
/// into the resizable buffer passed-in by the caller. The characters get
/// NULL-terminated before returning. Note that the next character returned
/// from the text stream will be '<' because of the ar_ungetc(...) called.
/// If EOF occurs before we've made it to the next '<', go ahead and
/// return false. Otherwise, return true.
/// The parsers for various field types all call this first... and then
/// try to parse the string into their types (for instance, into a sequence
/// of floats).
bool ar_getTextBeforeTag(arTextStream* textStream, arBuffer<char>* textBuffer){
  int ch = ' ';
  int location = 0;
  while (ch != '<'){
    ch = textStream->ar_getc();
    if (ch == EOF){
      cout << "Syzygy XML error: unexpected end of file while reading "
	   << "field text.\n";
      return false;
    }
    if (ch != '<'){
      // Better over-grow a little bit... otherwise this code is slow as
      // molasses
      if (location >= textBuffer->size()){
        textBuffer->grow(2*textBuffer->size() + 1);
      }
      textBuffer->data[location] = char(ch);
      location++;
    }
  }
  textStream->ar_ungetc(ch);
  textBuffer->grow(location+1);
  // VERY IMPORTANT THAT THE CHARACTER BUFFER BE NULL-TERMINATED.
  // THIS MAKES FOR EASY CONVERSION TO A C++ STRING!
  textBuffer->data[location] = '\0';
  return true;
} 

/// Gets the text of the next tag, leaving the arTextStream at the character
/// after the closing '>'. For instance "<foo>" returns "foo" and 
/// "</foo>" returns "/foo". The caller passes in a resizable work buffer
/// to use in parsing the variable length string.
/// NOTE: returns "NULL" on error. And ignores initial whitespace.
/// NOTE: THIS IS NOT QUITE CORRECT!!! SPECIFICALLY, WE SHOULD IGNORE
///   INITIAL AND TRAILING WHITESPACE AND ONLY SUPPORT TAG NAMES WITH
///   NO WHITESPACE!
string ar_getTagText(arTextStream* textStream, arBuffer<char>* buffer){
  if (!ar_ignoreWhitespace(textStream)){
    return string("NULL");
  }
  int ch = textStream->ar_getc();
  if (ch == EOF){
    cout << "Syzygy XML error: unexpected end of file while searching for"
	 << "tag begin.\n";
    return string("NULL");
  }
  if (ch != '<'){
    cout << "Syzygy XML error: did not find expected tag start.\n";
    return string("NULL");
  }
  // Packing at the beginning of our resizable buffer....
  int location = 0;
  while (ch != '>'){
    ch = textStream->ar_getc();
    if (ch == EOF){
      cout << "Syzygy XML error: unexpected end of file inside tag.\n";
      return string("NULL");
    }
    if (ch != '>'){
      buffer->grow(location+1);
      buffer->data[location] = char(ch);
      location++;
    }
  }
  buffer->grow(location+1);
  buffer->data[location] = '\0';
  string result(buffer->data);
  return result;
} 

// A less efficient version designed for those who do not want the buffer
// management features.
string ar_getTagText(arTextStream* textStream){
  arBuffer<char> buffer(256);
  return ar_getTagText(textStream, &buffer);
}

/// Should ususally operate on the return value from ar_getTagText(...).
/// Returns true if the first character is '/'.
bool ar_isEndTag(const string& tagText){
  if (tagText.length() >= 1 && tagText[0] == '/'){
    return true;
  }
  return false;
}

/// Returns the tag type, irrespective of whether it is a beginning or
/// an ending tag. Specifically, "foo" and "/foo" both return "foo".
string ar_getTagType(const string& tagText){
  string result;
  if (tagText.length() >= 1 && tagText[0] == '/'){
    result = tagText.substr(1,tagText.length()-1);
  }
  else{
    result = tagText;
  }
  return result;
}

