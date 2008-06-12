//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arXMLUtilities.h"
#include "arBuffer.h"

// Take an arTextStream and skip through any whitespace at the beginning.
// If we hit EOF before the whitespace ends, return false. Otherwise,
// use ar_ungetc so that the next character in the stream will be the
// first non-whotespace character and return true.
//
// We can, optionally, pass a second parameter to this function so that
// the ignored characters will be recorded at the end of the given
// text buffer.
bool ar_ignoreWhitespace(arTextStream* textStream,
                         arBuffer<char>* optionalBuffer) {
  // IGNORE WHITESPACE! OK... so we've got:
  // ' ' = space
  // '\n' = linefeed
  // '\r' = 13 = carriage return (this doesn't really exist in Unix
  //   text files, but definitely exists in Windows text files)
  // 9 = horizontal tab
  int ch = 0;
  while (true) {
    ch = textStream->ar_getc();
    if (ch == EOF) {
      return false;
    }
    if (ch == ' ' || ch == '\n' || ch == 13 || ch == 9) {
      // Whitespace (must record if the optional buffer has been set)
      if (optionalBuffer) {
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

// Record characters from an arTextStream up to next '<'.
// Stuff these characters into the resizable buffer passed-in by the caller,
// null-terminated.  The next character returned from the text stream will be
// '<' because of an ar_ungetc().
//
// Return false iff EOF occurs before the next '<'.
// The parsers for various field types call this first, and then
// parse the string into their types, e.g. a sequence of floats.
//
// Print no diagnostics - leave that up to the caller.
bool ar_getTextBeforeTag(arTextStream* textStream, arBuffer<char>* textBuffer,
                         bool concatenate) {
  int ch = ' ';
  if (!concatenate) {
    // Start at the beginning of the buffer.
    textBuffer->pushPosition = 0;
  }
  while (ch != '<') {
    ch = textStream->ar_getc();
    if (ch == EOF) {
      // Found no tag.
      return false;
    }
    if (ch != '<') {
      textBuffer->push(char(ch));
    }
  }
  textStream->ar_ungetc(ch);
  textBuffer->grow(textBuffer->pushPosition+1);
  // Null-terminate for easy conversion to std::string.
  // But don't change "pushPosition", so the null gets
  // overwritten on the next "push".
  textBuffer->data[textBuffer->pushPosition] = '\0';
  return true;
}

// Take an arTextStream and record any characters between the current position
// and the next occuring </end_tag>. The arTextStream is left at the first
// character after </end_tag>. The textBuffer contains a NULL-terminated
// string with the characters up to the beginning of the final tag (but
// not including it).
bool ar_getTextUntilEndTag(arTextStream* textStream,
                           const string& endTag,
			   arBuffer<char>* textBuffer) {
  // Start at the beginning of the buffer.
  textBuffer->pushPosition = 0;
  while (true) {
    // Get text until the next tag.
    if (!ar_getTextBeforeTag(textStream, textBuffer, true)) {
      return false;
    }
    // Get the next tag. Save the position.
    const int finalPosition = textBuffer->pushPosition;
    const string tagText(ar_getTagText(textStream, textBuffer, true));
    if (tagText == "NULL") {
      // There has been an error!
      return false;
    }
    if (tagText == "/"+endTag) {
      // End tag.
      // NULL-terminate the array; use the position before the final tag.
      // Since the final tag has at least 4 characters, the array need not grow.
      textBuffer->data[finalPosition] = '\0';
      return true;
    }
  }
}

// Get the text of the next tag, leaving the arTextStream just past the closing '>'.
// For instance "<foo>" returns "foo" and "</foo>" returns "/foo".
// The caller supplies a resizable buffer for parsing the variable-length string.
//
// Returns "NULL" on error.
// Ignores initial whitespace.
//
// Bug: should ignore trailing whitespace.
// Bug: should reject tag names which include whitespace.
//
// Sometimes we want to be able to record the entire text stream in
// our buffer. The default is NOT to do so, because the concatenate
// parameter defaults to false.
//
// Print no diagnostics - leave that up to the caller.
string ar_getTagText(arTextStream* textStream, arBuffer<char>* buffer,
                     bool concatenate) {
  if (concatenate) {
    if (!ar_ignoreWhitespace(textStream, buffer)) {
      return string("NULL");
    }
  }
  else{
    if (!ar_ignoreWhitespace(textStream)) {
      return string("NULL");
    }
  }
  int ch = textStream->ar_getc();
  if (ch == EOF) {
    // Reasonable end of file.
    return string("NULL");
  }
  if (ch != '<') {
    // Missing start of tag.
    return string("NULL");
  }

  if (concatenate) {
    // Record the start-of-tag '<'.
    buffer->push(char(ch));
  } else {
    // Begin packing at the beginning.
    buffer->pushPosition = 0;
  }
  const int tagStart = buffer->pushPosition;
  while (true) {
    ch = textStream->ar_getc();
    if (ch == EOF) {
      // EOF during tag
      return string("NULL");
    }
    if (ch == '>') {
      break;
    }
    buffer->push(char(ch));
  }
  // Reached end of tag.
  const int tagLength = buffer->pushPosition - tagStart;
  if (concatenate) {
    // Record the end-of-tag '>'.
    buffer->push(char(ch));
  }
  // Caller might treat result as a C string, so null-terminate it.
  buffer->grow(buffer->pushPosition+1);
  buffer->data[buffer->pushPosition] = '\0';
  return string(buffer->data + tagStart, tagLength);
}

// Inefficient version, without buffer management.
string ar_getTagText(arTextStream* textStream) {
  arBuffer<char> buffer(256);
  return ar_getTagText(textStream, &buffer);
}

// Reports if the first character is '/'.
// Ususally called on ar_getTagText()'s return value.
bool ar_isEndTag(const string& tagText) {
  return !tagText.empty() && tagText[0] == '/';
}

// Return the tag type, irrespective of whether it is a beginning or
// an ending tag. So "foo" and "/foo" both return "foo".
string ar_getTagType(const string& tagText) {
  return ar_isEndTag(tagText) ?
    tagText.substr(1, tagText.length()-1) : tagText;
}
