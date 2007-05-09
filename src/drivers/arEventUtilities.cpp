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

  // ar_fromStructuredDataEnd() deletes these.
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

bool ar_toStructuredData(
  int& _numButtons, int& _numAxes, int& _numMatrices, int& _numItems,
  int& typeField, int& indexField, int& buttonField, int& axisField, int& matrixField,
  int*& typeBuf, int*& indexBuf, int*& buttonBuf, float*& axisBuf, float*& matrixBuf,
  arStructuredData* data
)
{
  typeField   = data->getDataFieldIndex("types");
  indexField  = data->getDataFieldIndex("indices");
  buttonField = data->getDataFieldIndex("buttons");
  axisField   = data->getDataFieldIndex("axes");
  matrixField = data->getDataFieldIndex("matrices");

  const int numItems    = data->getDataDimension(typeField);
  const int numIndices  = data->getDataDimension(indexField);
  const int numButtons  = data->getDataDimension(buttonField);
  const int numAxes     = data->getDataDimension(axisField);
  const int numMatrices = data->getDataDimension(matrixField);

  _numItems = _numButtons + _numAxes + _numMatrices;

  if (numButtons != _numButtons &&
      !data->setDataDimension( buttonField, _numButtons )) {
    ar_log_warning() << "ar_toStructuredData failed to set button data dimension.\n";
    return false;
  }
  if (numAxes != _numAxes &&
      !data->setDataDimension( axisField, _numAxes )) {
    ar_log_warning() << "ar_toStructuredData failed to set axis data dimension.\n";
    return false;
  }
  if (numMatrices != _numMatrices &&
      !data->setDataDimension( matrixField, _numMatrices )) {
    ar_log_warning() << "ar_toStructuredData failed to set matrix data dimension.\n";
    return false;
  }
  if (numItems != _numItems &&
      !data->setDataDimension( typeField, _numItems )) {
    ar_log_warning() << "ar_toStructuredData failed to set button data dimension.\n";
    return false;
  }
  if (numIndices != _numItems &&
      !data->setDataDimension( indexField, _numItems )) {
    ar_log_warning() << "ar_toStructuredData failed to set button data dimension.\n";
    return false;
  }

  typeBuf = new int[_numItems];
  indexBuf = new int[_numItems];
  buttonBuf = new int[_numButtons];
  axisBuf = new float[_numAxes];
  matrixBuf = new float[16*_numMatrices];

  if (!typeBuf || !indexBuf || !buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_warning() << "ar_toStructuredData out of memory.\n";
    return false;
  }
  return true;
}

bool ar_toStructuredDataEnd(
  const bool ok,
  const int sigB, const int sigA, const int sigM,
  const int& _numButtons,
  const int& _numAxes,
  const int& _numMatrices,
  const int& _numItems,
  const int& typeField,
  const int& indexField,
  const int& buttonField,
  const int& axisField,
  const int& matrixField,
  int*& typeBuf, int*& indexBuf, int*& buttonBuf, float*& axisBuf, float*& matrixBuf,
  arStructuredData* data) {

  const int sigBuf[] = { sigB, sigA, sigM };
  if (ok) {
    data->dataIn( "signature", sigBuf, AR_INT, 3 );
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
  int* typeBuf;
  int* indexBuf;
  int* buttonBuf;
  float* axisBuf;
  float* matrixBuf;
  int _numButtons = q->getNumberButtons();
  int _numAxes = q->getNumberAxes();
  int _numMatrices = q->getNumberMatrices();
  int _numItems;
  int typeField, indexField, buttonField, axisField, matrixField;
  if (!ar_toStructuredData(
    _numButtons, _numAxes, _numMatrices, _numItems,
    typeField, indexField, buttonField, axisField, matrixField,
    typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
    data)) {
    return false;
  }

  return ar_toStructuredDataEnd(
    q->saveToBuffers( typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf ),
    q->getButtonSignature(), q->getAxisSignature(), q->getMatrixSignature(),
    _numButtons, _numAxes, _numMatrices, _numItems,
    typeField, indexField, buttonField, axisField, matrixField,
    typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
    data);
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
  unsigned iBAM[3] = {0}; // button, axis, matrix
  for (i=0; i<(unsigned)numItems; i++) {
    if (indexBuf[i] < 0) {
      ar_log_warning() << "ar_setInputStateFromStructuredData ignoring negative event index.\n";
      ok = false;
      continue;
    }
    unsigned eventIndex = unsigned(indexBuf[i]);
    const int eventType = typeBuf[i];
    unsigned& iGizmo = iBAM[eventType];
    // todo: eventType-indexed array instead of numButtons numAxes numMatrices.
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iGizmo >= (unsigned)numButtons) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra buttons in index field.\n";
          ok = false;
        } else
          state->setButton( eventIndex, buttonBuf[iGizmo++] );
        break;
      case AR_EVENT_AXIS:
        if (iGizmo >= (unsigned)numAxes) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra axes in index field.\n";
          ok = false;
        } else
          state->setAxis( eventIndex, axisBuf[iGizmo++] );
        break;
      case AR_EVENT_MATRIX:
        if (iGizmo >= (unsigned)numMatrices) {
          ar_log_warning() << "ar_setInputStateFromStructuredData ignoring extra matrices in index field.\n";
          ok = false;
        } else
          state->setMatrix( eventIndex, matrixBuf + 16*iGizmo++ );
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
  int* typeBuf;
  int* indexBuf;
  int* buttonBuf;
  float* axisBuf;
  float* matrixBuf;
  int _numButtons = state->getNumberButtons();
  int _numAxes = state->getNumberAxes();
  int _numMatrices = state->getNumberMatrices();
  int _numItems;
  int typeField, indexField, buttonField, axisField, matrixField;
  if (!ar_toStructuredData(
    _numButtons, _numAxes, _numMatrices, _numItems,
    typeField, indexField, buttonField, axisField, matrixField,
    typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
    data)) {
    return false;
  }

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

  return ar_toStructuredDataEnd(
    state->saveToBuffers( buttonBuf, axisBuf, matrixBuf ),
    _numButtons, _numAxes, _numMatrices,
    _numButtons, _numAxes, _numMatrices,
    _numItems,
    typeField, indexField, buttonField, axisField, matrixField,
    typeBuf, indexBuf, buttonBuf, axisBuf, matrixBuf,
    data);
}
