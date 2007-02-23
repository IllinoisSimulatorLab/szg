//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILE_SOURCE_H
#define AR_FILE_SOURCE_H

#include "arInputSource.h"
#include "arInputLanguage.h"
#include "arStructuredDataParser.h"
#include "arFileTextStream.h"
#include "arDriversCalling.h"

class SZG_CALL arFileSource:public arInputSource{
  friend void ar_fileSourceEventTask(void*);
  void _eventThread();
 public:
  arFileSource();
  ~arFileSource();

  bool init(arSZGClient&);
  bool start();
 protected:
  arFileTextStream _dataStream;
  string _dataFileName;
  string _dataFilePath;
  arInputLanguage _lang;
  arStructuredDataParser* _parser;
};

#endif
