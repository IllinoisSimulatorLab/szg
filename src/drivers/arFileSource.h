//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILE_SOURCE_H
#define AR_FILE_SOURCE_H

#include "arInputSource.h"
#include "arInputLanguage.h"
#include "arStructuredDataParser.h"
#include "arFileTextStream.h"

class arFileSource:public arInputSource{
  friend void ar_fileSourceEventTask(void*);
 public:
  arFileSource();
  ~arFileSource();

  bool init(arSZGClient&);
  bool start();
 protected:
  arThread _eventThread;
  arFileTextStream _dataStream;
  string _dataFileName;
  string _dataFilePath;
  arInputLanguage _lang;
  arStructuredDataParser* _parser;
};

#endif
