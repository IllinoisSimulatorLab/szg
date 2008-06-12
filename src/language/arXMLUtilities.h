//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_XML_UTILITIES_H
#define AR_XML_UTILITIES_H

#include "arTextStream.h"
#include "arBuffer.h"
#include "arDataType.h"
#include "arLanguageCalling.h"
#include <string>
#include <sstream>

using namespace std;

// XML parsing
SZG_CALL bool ar_ignoreWhitespace(arTextStream* textStream,
                                  arBuffer<char>* optionalBuffer = NULL);
SZG_CALL bool ar_getTextBeforeTag(arTextStream* textStream,
                                  arBuffer<char>* textBuffer,
                                  bool concatenate = false);
SZG_CALL bool ar_getTextUntilEndTag(arTextStream* textStream,
                                    const string& endTag,
                                    arBuffer<char>* textBuffer);
SZG_CALL string ar_getTagText(arTextStream* textStream,
                              arBuffer<char>* buffer,
                              bool concatenate = false);
SZG_CALL string ar_getTagText(arTextStream*);
SZG_CALL bool   ar_isEndTag(const string& tagText);
SZG_CALL string ar_getTagType(const string& tagText);

template <class T>
void ar_growXMLBuffer(T*& data, int location, int& size) {
  if (location < size)
    return;
  T* newData = new T[size *= 2];
  for (int i=0; i<location; i++) {
    newData[i] = data[i];
  }
  T* tmp = data;
  data = newData;
  delete [] tmp;
}

template <class T>
int ar_parseXMLData(arBuffer<char>* buffer, T*& data,
                    bool skipWhitespace = true) {
  int bufferSize = 128;
  data = new T[bufferSize];
  int location = 0;
  stringstream s(buffer->data);
  if (!skipWhitespace) {
    noskipws(s);
  }
  while (true) {
    s >> data[location];
    // The failure conditions on streams are a little obnoxious.
    // If the above operation failed, then fail() should return true.
    // PLEASE NOTE: !good() is incorrect. This can occur when the
    // piece of data occupies space up to the last character (i.e. we've
    // got EOF. So, a seperate check for that is needed.
    if (!s.fail()) {
      ar_growXMLBuffer(data, ++location, bufferSize);
    }
    if (s.eof()) {
      break;
    }
  }
  return location;
}

#endif
