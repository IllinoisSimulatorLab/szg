//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arQueuedData.h"

arQueuedData::arQueuedData() {
  // It used to be that we relied on an arDataTemplate from some language
  // or another to construct the internal arStructruedData buffers.
  // As an intermediate step, we construct the arStructuredData, etc.
  // ourselves. Eventually, just use a buffer...
  _bufferTemplate = new arDataTemplate("buffer");
  _bufferTemplate->setID(0);
  BUFFER = _bufferTemplate->add("buffer", AR_CHAR);
  _buffer1 = new arStructuredData(_bufferTemplate);
  _buffer2 = new arStructuredData(_bufferTemplate);

  _minSendSize = 200;  // this seems to be pretty reasonable...
                       // makes even the Win32 TCP stack send
                       // the data without delay
  _maxBufferSize = 1000000;
                       // _maxBufferSize is automatically grown.

  _frontBuffer = _buffer1;
  _backBuffer = _buffer2;

  // the first 8 bytes will hold buffer size info plus the number of
  // records. yes, this is redundant over what's in the arStructuredData
  // reord now, but that should be phased-out.
  _bufferLocation = 8;
  _frontBufferSize = 0;
  _numberBufferRecords = 0;

  _buffer1->setStorageDimension(BUFFER, _maxBufferSize);
  _buffer2->setStorageDimension(BUFFER, _maxBufferSize);
  _buffer1->setDataDimension(BUFFER, _minSendSize);
  _buffer2->setDataDimension(BUFFER, _minSendSize);
}

arQueuedData::~arQueuedData() {
  delete _buffer1;
  delete _buffer2;
}

void arQueuedData::setMinimumSendSize(int theSize) {
  _minSendSize = theSize;
}

ARchar* arQueuedData::getFrontBufferRaw() {
  return (ARchar*)_frontBuffer->getDataPtr(BUFFER, AR_CHAR);
}

ARint arQueuedData::getFrontBufferSize() {
  return _frontBufferSize;
}

int arQueuedData::getBackBufferSize() {
  return _bufferLocation;
}

void arQueuedData::swapBuffers() {
  // prepare back buffer to be sent across the network
  int dataAmount = _bufferLocation;
  if (dataAmount < _minSendSize)
    dataAmount = _minSendSize;

  _backBuffer->setDataDimension(BUFFER, dataAmount);

  arStructuredData* temp = _frontBuffer;
  _frontBuffer = _backBuffer;
  _backBuffer = temp;

  // This is for the raw buffer manipulation.
  _frontBufferSize = _bufferLocation;
  ARchar* theBuffer = getFrontBufferRaw();
  ar_packData(theBuffer, &_frontBufferSize, AR_INT, 1);
  ar_packData(theBuffer+AR_INT_SIZE, &_numberBufferRecords, AR_INT, 1);

  // Start the buffer 2 ints later.
  _bufferLocation = 2*AR_INT_SIZE;
  _numberBufferRecords = 0;
}

void arQueuedData::forceQueueData(arStructuredData* theData) {
  // Grow only _backBuffer.  Another thread's reading _frontBuffer.
  const int recordSize = theData->size();
  const int actualSize = _bufferLocation + recordSize;
  int currentStorageDimension = _backBuffer->getStorageDimension(BUFFER);
  if (actualSize > currentStorageDimension) {
    currentStorageDimension *= 2;
    if (currentStorageDimension < actualSize)
      currentStorageDimension = actualSize;
    // Bug? why does setStorageDimension fail here?
    _backBuffer->setDataDimension(BUFFER, currentStorageDimension);
  }

  ARchar* bufferPtr = (ARchar*) _backBuffer->getDataPtr(BUFFER, AR_CHAR);
  theData->pack(bufferPtr + _bufferLocation);
  _bufferLocation += recordSize;
  _numberBufferRecords++;
}
