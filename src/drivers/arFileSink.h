//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILE_SINK_H
#define AR_FILE_SINK_H

#include "arInputSink.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

// Record I/O device data to a file, for later playback.  Has timing issues.

class SZG_CALL arFileSink : public arInputSink{
 public:
  arFileSink();
  ~arFileSink() {}
 
  bool init(arSZGClient&);
  bool start();
  bool stop();

  void receiveData(int,arStructuredData*);
 private:
  string _dataFilePath;
  string _dataFileName;
  FILE* _dataFile;
  bool _logging;
  arMutex _logLock;
};

#endif 
