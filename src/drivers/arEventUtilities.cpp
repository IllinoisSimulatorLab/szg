//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arEventUtilities.h"

static inline int maxint(const int a, const int b) { return a>b ? a : b; }
static inline int maxint(const unsigned a, const int b) { return int(a)>b ? int(a) : b; }
static inline int maxint(const int a, const unsigned b) { return a>int(b) ? a : int(b); }
static inline int maxint(const unsigned a, const unsigned b) { return int(a>b ? a : b); }

// todo: ar_toStructuredData like ar_fromStructuredData, to decopypaste.

bool ar_fromStructuredData(int& sigLen, int& sigField, int* sigBuf,
         int*& typeBuf, int*& indexBuf, int*& buttonBuf, float*& axisBuf, float*& matrixBuf,
	 int& numItems, int& numButtons, int& numAxes, int& numMatrices,
         const arStructuredData* data) {

               sigField = data->getDataFieldIndex("signature");
  const int   typeField = data->getDataFieldIndex("types");
  const int  indexField = data->getDataFieldIndex("indices");
  const int buttonField = data->getDataFieldIndex("buttons");
  const int   axisField = data->getDataFieldIndex("axes");
  const int matrixField = data->getDataFieldIndex("matrices");

            sigLen      = data->getDataDimension(sigField);
            numItems    = data->getDataDimension(typeField);
  const int numIndices  = data->getDataDimension(indexField);
            numButtons  = data->getDataDimension(buttonField);
            numAxes     = data->getDataDimension(axisField);
            numMatrices = data->getDataDimension(matrixField);

  if (sigLen != 3)
    ar_log_warning() << "ar_fromStructuredData: invalid signature.\n";

  if (numMatrices % 16 != 0){
    ar_log_warning() << "ar_fromStructuredData: fractional number of matrices (" <<
      numMatrices << "/16).\n";
    return false;
  }

  numMatrices /= 16;
  if (numItems != numIndices){
    ar_log_warning() << "ar_fromStructuredData mismatch: " <<
      numItems << " types but " << numIndices << " indices.\n";
    return false;
  }

  if (numButtons + numAxes + numMatrices != numItems){
    ar_log_warning() << "ar_fromStructuredData dictionary mismatch: " <<
      numButtons <<"+"<< numAxes <<"+"<< numMatrices << " != " << numItems << ".\n";
    return false;
  }

  // Caller's responsible for delete[]'ing these.
  typeBuf = new int[numItems];
  indexBuf = new int[numItems];
  buttonBuf = new int[numButtons];
  axisBuf = new float[numAxes];
  matrixBuf = new float[16*numMatrices];

  if (!typeBuf || !indexBuf || !buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_warning() << "ar_fromStructuredData out of memory.\n";
    return false;
  }

  data->dataOut( typeField, typeBuf, AR_INT, numItems );
  data->dataOut( indexField, indexBuf, AR_INT, numItems );
  data->dataOut( buttonField, buttonBuf, AR_INT, numButtons );
  data->dataOut( axisField, axisBuf, AR_FLOAT, numAxes );
  data->dataOut( matrixField, matrixBuf, AR_FLOAT, numMatrices*16 );

  unsigned i;
  if (sigLen == 3) {
    data->dataOut( sigField, sigBuf, AR_INT, sigLen );
    for (i=0; i<(unsigned)sigLen; i++) {
      if (sigBuf[i] < 0) {
        ar_log_warning() << "ar_fromStructuredData zeroing received negative signature.\n";
        sigBuf[i] = 0;
      }
    }
  }

  return true;
}

void ar_fromStructuredDataEnd(int*& typeBuf, int*& indexBuf, int*& buttonBuf, float*& axisBuf, float*& matrixBuf) {
  delete [] typeBuf;
  delete [] indexBuf;
  delete [] buttonBuf;
  delete [] axisBuf;
  delete [] matrixBuf;
}

bool ar_setEventQueueFromStructuredData( arInputEventQueue* q,
                                         const arStructuredData* data ) {
  int sigLen, sigField, sigBuf[3];
  int* typeBuf;
  int* indexBuf;
  int* buttonBuf;
  float* axisBuf;
  float* matrixBuf;
  int numItems, numButtons, numAxes, numMatrices;
  if (!ar_fromStructuredData(sigLen, sigField, sigBuf,
      typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
      numItems, numButtons, numAxes, numMatrices,
      data))
    return false;

  if (sigLen == 3) {
    sigBuf[0] = maxint( sigBuf[0], numButtons );
    sigBuf[1] = maxint( sigBuf[1], numAxes );
    sigBuf[2] = maxint( sigBuf[2], numMatrices );
    q->setSignature( (unsigned)sigBuf[0], (unsigned)sigBuf[1], (unsigned)sigBuf[2] );
  }

  const bool ok = q->setFromBuffers(
    typeBuf, indexBuf, buttonBuf, numButtons, axisBuf, numAxes, matrixBuf, numMatrices );
  ar_fromStructuredDataEnd(typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf);
  return ok;
}

bool ar_saveEventQueueToStructuredData( const arInputEventQueue* q,
                                        arStructuredData* data ) {
  const int typeField = data->getDataFieldIndex("types");
  const int indexField = data->getDataFieldIndex("indices");
  const int buttonField = data->getDataFieldIndex("buttons");
  const int axisField = data->getDataFieldIndex("axes");
  const int matrixField = data->getDataFieldIndex("matrices");

  const int numItems    = data->getDataDimension(typeField);
  const int numIndices  = data->getDataDimension(indexField);
  const int numButtons  = data->getDataDimension(buttonField);
  const int numAxes     = data->getDataDimension(axisField);
  const int numMatrices = data->getDataDimension(matrixField);

  const int _numButtons = q->getNumberButtons();
  const int _numAxes = q->getNumberAxes();
  const int _numMatrices = q->getNumberMatrices();
  const int _numItems = _numButtons + _numAxes + _numMatrices;
  if (_numItems != int(q->size()))
    ar_log_warning() << "ar_saveEventQueueToStructuredData: queue miscount, " <<
      _numButtons << "+" << _numAxes << "+" << _numMatrices << " != " << q->size() << ".\n";

  if (numButtons != _numButtons)
    if (!data->setDataDimension( buttonField, _numButtons )) {
      ar_log_warning() << "ar_saveEventQueueToStructuredData "
	   << "failed to set button data dimension.\n";
      return false;
    }
  if (numAxes != _numAxes)
    if (!data->setDataDimension( axisField, _numAxes )) {
      ar_log_warning() << "ar_saveEventQueueToStructuredData "
	   << "failed to set axis data dimension.\n";
      return false;
    }
  if (numMatrices != _numMatrices)
    if (!data->setDataDimension( matrixField, _numMatrices )) {
      ar_log_warning() << "ar_saveEventQueueToStructuredData "
	   << "failed to set matrix data dimension.\n";
      return false;
    }
  if (numItems != _numItems)
    if (!data->setDataDimension( typeField, _numItems )) {
      ar_log_warning() << "ar_saveEventQueueToStructuredData "
	   << "failed to set button data dimension.\n";
      return false;
    }
  if (numIndices != _numItems)
    if (!data->setDataDimension( indexField, _numItems )) {
      ar_log_warning() << "ar_saveEventQueueToStructuredData "
	   << "failed to set button data dimension.\n";
      return false;
    }

  int* typeBuf = new int[_numItems];
  int* indexBuf = new int[_numItems];
  int* buttonBuf = new int[_numButtons];
  float* axisBuf = new float[_numAxes];
  float* matrixBuf = new float[16*_numMatrices];

  if (!typeBuf || !indexBuf || !buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_warning() << "ar_setEventQueueToStructuredData out of memory.\n";
    return false;
  }

  const int sigBuf[] = {
    q->getButtonSignature(),
    q->getAxisSignature(), 
    q->getMatrixSignature() };
  // ar_log_critical() << "packing sig " << sigBuf[0] << sigBuf[1] << sigBuf[2] << ".\n";
  data->dataIn( "signature", sigBuf, AR_INT, 3 );

  const bool ok = q->saveToBuffers( typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf );
  if (ok) {
    data->dataIn( typeField, typeBuf, AR_INT, _numItems );
    data->dataIn( indexField, indexBuf, AR_INT, _numItems );
    data->dataIn( buttonField, buttonBuf, AR_INT, _numButtons );
    data->dataIn( axisField, axisBuf, AR_FLOAT, _numAxes );
    data->dataIn( matrixField, matrixBuf, AR_FLOAT, _numMatrices*16 );
  }
  delete [] typeBuf;
  delete [] indexBuf;
  delete [] buttonBuf;
  delete [] axisBuf;
  delete [] matrixBuf;
  return ok;
}

bool ar_setInputStateFromStructuredData( arInputState* state,
                                         const arStructuredData* data ) {
  int sigLen, sigField, sigBuf[3];
  int* typeBuf;
  int* indexBuf;
  int* buttonBuf;
  float* axisBuf;
  float* matrixBuf;
  int numItems, numButtons, numAxes, numMatrices;
  if (!ar_fromStructuredData(sigLen, sigField, sigBuf,
      typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
      numItems, numButtons, numAxes, numMatrices,
      data))
    return false;

  // Compute signature from max of eventIndex.
  unsigned i;
  unsigned stateSig[3] = {0};
  for (i=0; i<(unsigned)numItems; i++) {
    const unsigned eventIndex = unsigned(indexBuf[i]);
    switch (typeBuf[i]) {
      case AR_EVENT_GARBAGE:
        break;
      case AR_EVENT_BUTTON:
      case AR_EVENT_AXIS:
      case AR_EVENT_MATRIX:
        unsigned& sig = stateSig[typeBuf[i]];
	if (eventIndex >= sig)
          sig = eventIndex + 1;
        break;
    }
  }

  // Do not max stateSig with the
  // number of buttons, axes, and matrices of the **input state**.
  // This lets a disconnected input device *decrease* its signature.

  for (i=0; i<3; ++i)
    stateSig[i] = (unsigned)maxint( stateSig[i], sigBuf[i]);
  state->setSignature( stateSig[0], stateSig[1], stateSig[2] );

  bool ok = true;
  unsigned iButton = 0;
  unsigned iAxis = 0;
  unsigned iMatrix = 0;
  for (i=0; i<(unsigned)numItems; i++) {
    if (indexBuf[i] < 0) {
      ar_log_warning() << "ar_setInputStateFromStructuredData ignoring negative event index.\n";
      ok = false;
      continue;
    }
    unsigned eventIndex = unsigned(indexBuf[i]);
    const int eventType = typeBuf[i];
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= (unsigned)numButtons) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra buttons in index field.\n";
          ok = false;
        } else
          state->setButton( eventIndex, buttonBuf[iButton++] );
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= (unsigned)numAxes) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra axes in index field.\n";
          ok = false;
        } else
          state->setAxis( eventIndex, axisBuf[iAxis++] );
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= (unsigned)numMatrices) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra matrices in index field.\n";
          ok = false;
        } else
          state->setMatrix( eventIndex, matrixBuf + 16*iMatrix++ );
        break;
      default:
        ar_log_warning() << "ar_setInputStateFromStructuredData ignoring event type "
             << eventType << ".\n";
        ok = false;
    }
  }

  ar_fromStructuredDataEnd(typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf);
  return ok;
}

bool ar_saveInputStateToStructuredData( const arInputState* state,
                                        arStructuredData* data ) {
  const int typeField   = data->getDataFieldIndex("types");
  const int indexField  = data->getDataFieldIndex("indices");
  const int buttonField = data->getDataFieldIndex("buttons");
  const int axisField   = data->getDataFieldIndex("axes");
  const int matrixField = data->getDataFieldIndex("matrices");

  const int numItems    = data->getDataDimension(typeField);
  const int numIndices  = data->getDataDimension(indexField);
  const int numButtons  = data->getDataDimension(buttonField);
  const int numAxes     = data->getDataDimension(axisField);
  const int numMatrices = data->getDataDimension(matrixField);

  const int _numButtons = state->getNumberButtons();
  const int _numAxes = state->getNumberAxes();
  const int _numMatrices = state->getNumberMatrices();
  const int _numItems = _numButtons + _numAxes + _numMatrices;

  if (numButtons != _numButtons)
    if (!data->setDataDimension( buttonField, _numButtons )) {
      ar_log_warning() << "ar_saveInputStateToStructuredData "
	   << "failed to set button data dimension.\n";
      return false;
    }
  if (numAxes != _numAxes)
    if (!data->setDataDimension( axisField, _numAxes )) {
      ar_log_warning() << "ar_saveInputStateToStructuredData "
	   << "failed to set axis data dimension.\n";
      return false;
    }
  if (numMatrices != _numMatrices)
    if (!data->setDataDimension( matrixField, _numMatrices )) {
      ar_log_warning() << "ar_saveInputStateToStructuredData "
	   << "failed to set matrix data dimension.\n";
      return false;
    }
  if (numItems != _numItems)
    if (!data->setDataDimension( typeField, _numItems )) {
      ar_log_warning() << "ar_saveInputStateToStructuredData "
	   << "failed to set type data dimension.\n";
      return false;
    }
  if (numIndices != _numItems)
    if (!data->setDataDimension( indexField, _numItems )) {
      ar_log_warning() << "ar_saveInputStateToStructuredData "
	   << "failed to set index data dimension.\n";
      return false;
    }

  const int sigBuf[3] = { _numButtons, _numAxes, _numMatrices };
  int* typeBuf = new int[_numItems];
  int* indexBuf = new int[_numItems];
  int* buttonBuf = new int[_numButtons];
  float* axisBuf = new float[_numAxes];
  float* matrixBuf = new float[16*_numMatrices];

  unsigned i;
  unsigned iEvent = 0;
  for (i=0; i<(unsigned)_numButtons; i++, iEvent++) {
    typeBuf[iEvent] = AR_EVENT_BUTTON;
    indexBuf[iEvent] = (int)i;
  }
  for (i=0; i<(unsigned)_numAxes; i++, iEvent++) {
    typeBuf[iEvent] = AR_EVENT_AXIS;
    indexBuf[iEvent] = (int)i;
  }
  for (i=0; i<(unsigned)_numMatrices; i++, iEvent++) {
    typeBuf[iEvent] = AR_EVENT_MATRIX;
    indexBuf[iEvent] = (int)i;
  }
  const bool ok = state->saveToBuffers( buttonBuf, axisBuf, matrixBuf );

  data->dataIn( "signature", sigBuf, AR_INT, 3 );
  data->dataIn( typeField, typeBuf, AR_INT, _numItems );
  data->dataIn( indexField, indexBuf, AR_INT, _numItems );
  data->dataIn( buttonField, buttonBuf, AR_INT, _numButtons );
  data->dataIn( axisField, axisBuf, AR_FLOAT, _numAxes );
  data->dataIn( matrixField, matrixBuf, AR_FLOAT, _numMatrices*16 );

  delete [] typeBuf;
  delete [] indexBuf;
  delete [] buttonBuf;
  delete [] axisBuf;
  delete [] matrixBuf;
  return ok;
}
