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
///
/// We can, optionally, pass a second parameter to this function so that
/// the ignored characters will be recorded at the end of the given
/// text buffer.
bool ar_ignoreWhitespace(arTextStream* textStream, 
                         arBuffer<char>* optionalBuffer){
  // IGNORE WHITESPACE! OK... so we've got:
  // ' ' = space
  // '\n' = linefeed
  // '\r' = 13 = carriage return (this doesn't really exist in Unix
  //   text files, but definitely exists in Windows text files)
  // 9 = horizontal tab 
  int ch;
  while (true){
    ch = textStream->ar_getc();
    if (ch == EOF){
      // DO NOT PRINT OUT ANYTHING HERE!!
      return false;
    }
    if (ch == ' ' || ch == '\n' || ch == 13 || ch == 9){
      // Whitespace (must record if the optional buffer has been set)
      if (optionalBuffer){
        optionalBuffer->push(char(ch));
      }
    }
    else{
      // Not whitespace.
      break;
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
///
/// NOTE: By default, we begin recording at the beginning of the buffer.
/// However, sometimes it is more desirable to concatenate. In this case,
/// the concatenate parameter must be set to true.
bool ar_getTextBeforeTag(arTextStream* textStream, arBuffer<char>* textBuffer,
                         bool concatenate){
  int ch = ' ';
  if (!concatenate){
    textBuffer->pushPosition = 0;
  }
  while (ch != '<'){
    ch = textStream->ar_getc();
    if (ch == EOF){
      cout << "Syzygy XML error: unexpected end of file while reading "
	   << "field text.\n";
      return false;
    }
    if (ch != '<'){
      textBuffer->push(char(ch));
    }
  }
  textStream->ar_ungetc(ch);
  textBuffer->grow(textBuffer->pushPosition+1);
  // VERY IMPORTANT THAT THE CHARACTER BUFFER BE NULL-TERMINATED.
  // THIS MAKES FOR EASY CONVERSION TO A C++ STRING!
  // We do not change the "pushPosition" here (i.e. the terminator will
  // be overwritten on the next "push").
  textBuffer->data[textBuffer->pushPosition] = '\0';
  return true;
} 

/// Take an arTextStream and record any characters between the current position
/// and the next occuring </end_tag>. The arTextStream is left at the first
/// character after </end_tag>. 
bool ar_getTextUntilEndTag(arTextStream* textStream,
                           const string& endTag,
			   arBuffer<char>* textBuffer){
  // We always start at the beginning of the buffer in this case.
  textBuffer->pushPosition = 0;
  string tagText;
  while (true){
    // Get text until the next tag.
    if (!ar_getTextBeforeTag(textStream, textBuffer, true)){
      return false;
    }
    // Get the next tag.
    tagText = ar_getTagText(textStream, textBuffer, true);
    if (tagText == "NULL"){
      // There has been an error!
      return false;
    }
    // Is it our end tag? If so, break.
    if (tagText == "/"+endTag){
      break;
    }
  }
  // Go ahead and NULL-terminate the array;
  textBuffer->grow(textBuffer->pushPosition+1);
  textBuffer->data[textBuffer->pushPosition] = '\0';
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
/// 
/// Sometimes we want to be able to record the entire text stream in
/// our buffer. The default is NOT to do so, because the concatenate 
/// parameter is false unless explicitly set.
string ar_getTagText(arTextStream* textStream, arBuffer<char>* buffer,
                     bool concatenate){
  if (concatenate){
    if (!ar_ignoreWhitespace(textStream, buffer)){
      return string("NULL");
    }
  }
  else{
    if (!ar_ignoreWhitespace(textStream)){
      return string("NULL");
    }
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
  // Only record the '<' character if we are concatenating!
  else if (concatenate){
    buffer->push(char(ch));
  }
  // Only begin packing at the beginning if we are NOT in concatenate mode.
  if (!concatenate){
    buffer->pushPosition = 0;
  }
  int tagStart = buffer->pushPosition;
  while (ch != '>'){
    ch = textStream->ar_getc();
    if (ch == EOF){
      cout << "Syzygy XML error: unexpected end of file inside tag.\n";
      return string("NULL");
    }
    if (ch != '>'){
      buffer->push(char(ch));
    }
  }
  int tagLength = buffer->pushPosition - tagStart;
  // Record the final '>' if we are concatenating.
  if (concatenate){
    buffer->push(char(ch));
  }
  // NOTE: Code might try to treat the buffer array as a C-string.
  // Null-terminate it.
  buffer->grow(buffer->pushPosition+1);
  buffer->data[buffer->pushPosition] = '\0';
  string result(buffer->data + tagStart, tagLength);
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

