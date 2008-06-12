//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_QUEUED_DATA_H
#define AR_QUEUED_DATA_H

#include "arDataTemplate.h"
#include "arStructuredData.h"
#include "arDataUtilities.h"
#include "arLanguageCalling.h"

// Group several arStructuredData objects into one.

class SZG_CALL arQueuedData{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arQueuedData();
  ~arQueuedData();

  void setMinimumSendSize(int theSize);
  ARchar* getFrontBufferRaw();
  ARint getFrontBufferSize();
  int getBackBufferSize();
  void swapBuffers();
  void forceQueueData(arStructuredData*);

 private:
  arDataTemplate* _bufferTemplate;
  int BUFFER;

  int _maxBufferSize;
  int _minSendSize;

  arStructuredData* _frontBuffer;
  arStructuredData* _backBuffer;
  arStructuredData* _buffer1;
  arStructuredData* _buffer2;

  int _bufferLocation;
  ARint _frontBufferSize;
  int _numberBufferRecords;
};

#endif
