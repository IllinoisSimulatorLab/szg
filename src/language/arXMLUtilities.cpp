//********************************************************
// Syzygy is licensed under the BSD license v2
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
  int ch = 0;
  while (true){
    ch = textStream->ar_getc();
    if (ch == EOF){
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

/// Record characters from an arTextStream between the current
/// position and the next '<'. Stuff these characters
/// into the resizable buffer passed-in by the caller, null-terminated.
/// The next character returned
/// from the text stream will be '<' because of an ar_ungetc().
/// Return false iff EOF occurs before the next '<'.
/// The parsers for various field types call this first, and then
/// parse the string into their types, e.g. a sequence of floats).
///
/// By default, start at the beginning of the buffer.
/// But sometimes it's better to concatenate: if so, set "concatenate" to true.
bool ar_getTextBeforeTag(arTextStream* textStream, arBuffer<char>* textBuffer,
                         bool concatenate){
  int ch = ' ';
  if (!concatenate){
    textBuffer->pushPosition = 0;
  }
  while (ch != '<'){
    ch = textStream->ar_getc();
    if (ch == EOF){
      // DO NOT PRINT ANYTHING HERE! LET THE CALLER PRINT AN ERROR 
      // MESSAGE!
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
/// character after </end_tag>. The textBuffer contains a NULL-terminated
/// string with the characters up to the beginning of the final tag (but
/// not including it).
bool ar_getTextUntilEndTag(arTextStream* textStream,
                           const string& endTag,
			   arBuffer<char>* textBuffer){
  // We always start at the beginning of the buffer in this case.
  textBuffer->pushPosition = 0;
  string tagText;
  int finalPosition = 0;
  while (true){
    // Get text until the next tag.
    if (!ar_getTextBeforeTag(textStream, textBuffer, true)){
      return false;
    }
    // Get the next tag. Note how we save the position.
    finalPosition = textBuffer->pushPosition;
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
  // NULL-terminate the array; We use the position before
  // the final tag. Since the final tag has at least 4 characters, we
  // do not need to grow the array.
  textBuffer->data[finalPosition] = '\0';
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
    // DO NOT PRINT ANYTHING HERE! LET THE CALLER DO IT!
    return string("NULL");
  }
  if (ch != '<'){
    // DO NOT PRINT ANYTHING HERE! LET THE CALLER DO IT!
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
      // DO NOT PRINT ANYTHING HERE! LET THE CALLER DO IT!
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

