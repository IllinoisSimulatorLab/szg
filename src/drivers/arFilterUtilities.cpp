//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFilterUtilities.h"

int ar_getNumberEvents(arStructuredData* d){
  return d->getDataDimension("types");
}

int ar_getNumberEventsType(int eventType, arStructuredData* d) {
  int count = 0;
  for (int i=0; i<ar_getNumberEvents(d); i++)
    if (ar_getEventType(i,d) == eventType)
      count++;
  return count;
}

int ar_getEventID(int eventType, int eventNumber, arStructuredData* d) {
  if (eventNumber < 0)
    return -1;
  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<ar_getNumberEvents(d); i++)
    if (eventTypePtr[i] == eventType) {
      if (count == eventNumber)
        return i;
      ++count;
    }
  return -1;
}

// note that event IDs start with 0...

int ar_getEventType(int eventID, arStructuredData* d){
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return AR_EVENT_GARBAGE;

  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  if (!eventTypePtr){
    return AR_EVENT_GARBAGE;
  }
  return eventTypePtr[eventID];
}

int ar_getEventIndex(int eventID, arStructuredData* d){
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return 0;

  return ((int*) d->getDataPtr("indices",AR_INT))[eventID];
}

int ar_getEventButtonValue(int eventID, arStructuredData* d){
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return 0;

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_BUTTON){
      ++count;
    }
  }
  if (count==0){
    return 0;
  }
  return ((int*) d->getDataPtr("buttons",AR_INT))[count-1];
}

float ar_getEventAxisValue(int eventID, arStructuredData* d){
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return 0.;

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_AXIS){
      ++count;
    }
  }
  if (count==0){
    return 0.;
  }
  return ((float*) d->getDataPtr("axes",AR_FLOAT))[count-1];
}

arMatrix4 ar_getEventMatrixValue(int eventID, arStructuredData* d){
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return ar_identityMatrix();

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_MATRIX)
      ++count;
  }
  if (count==0)
    return ar_identityMatrix();
  return arMatrix4(((float*) d->getDataPtr("matrices",AR_FLOAT)) + (count-1)*16);
}


int ar_getEventButtonNumberValue(int buttonNumber, arStructuredData* d) {
  if (buttonNumber < 0)
    return 0;

  int count = 0;
  int* typePtr = (int*) d->getDataPtr("types",AR_INT);
  int* indexPtr = (int*) d->getDataPtr("indices",AR_INT);
  for (int i=0; i<ar_getNumberEvents( d ); i++) {
    if (typePtr[i] == AR_EVENT_BUTTON) {
      count++;
      if (indexPtr[i] == buttonNumber)
        return ((int*) d->getDataPtr("buttons",AR_INT))[count-1];
    }
  }
  return 0;
}

float ar_getEventAxisNumberValue(int axisNumber, arStructuredData* d) {
  if (axisNumber < 0)
    return 0;

  int count = 0;
  int* typePtr = (int*) d->getDataPtr("types",AR_INT);
  int* indexPtr = (int*) d->getDataPtr("indices",AR_INT);
  for (int i=0; i<ar_getNumberEvents( d ); i++) {
    if (typePtr[i] == AR_EVENT_AXIS) {
      count++;
      if (indexPtr[i] == axisNumber)
        return ((float*) d->getDataPtr("axes",AR_FLOAT))[count-1];
    }
  }
  return 0;
}

arMatrix4 ar_getEventMatrixNumberValue(int matrixNumber, arStructuredData* d) {
  if (matrixNumber < 0)
    return ar_identityMatrix();

  int count = 0;
  int* typePtr = (int*) d->getDataPtr("types",AR_INT);
  int* indexPtr = (int*) d->getDataPtr("indices",AR_INT);
  for (int i=0; i<=ar_getNumberEvents( d ); i++) {
    if (typePtr[i] == AR_EVENT_MATRIX) {
      count++;
      if (indexPtr[i] == matrixNumber)
        return arMatrix4(((float*) d->getDataPtr("matrices",AR_FLOAT)) + (count-1)*16);
    }
  }
  return ar_identityMatrix();
}

bool ar_replaceButtonEvent(int eventID, int value, arStructuredData* d){
  if (ar_getEventType(eventID,d) != AR_EVENT_BUTTON)
    return false;

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_BUTTON)
      ++count;
  }
  ((int*) d->getDataPtr("buttons",AR_INT))[count-1] = value;
  return true;
}

bool ar_replaceAxisEvent(int eventID, float value, arStructuredData* d){
  if (ar_getEventType(eventID,d) != AR_EVENT_AXIS)
    return false;

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_AXIS)
      ++count;
  }
  ((float*) d->getDataPtr("axes",AR_FLOAT))[count-1] = value;
  return true;
}

bool ar_replaceMatrixEvent(int eventID, const arMatrix4& value, arStructuredData* d){
  if (ar_getEventType(eventID,d) != AR_EVENT_MATRIX)
    return false;

  int count = 0;
  int* eventTypePtr = (int*) d->getDataPtr("types",AR_INT);
  for (int i=0; i<=eventID; i++){
    if (eventTypePtr[i] == AR_EVENT_MATRIX)
      ++count;
  }
  memcpy(((float*) d->getDataPtr("matrices",AR_FLOAT)) + (count-1)*16,
         value.v, 16*sizeof(float));
  return true;
}

bool ar_insertButtonEvent( int buttonNumber, const int value, arStructuredData* d ) {
  int eventID = ar_getEventID( AR_EVENT_BUTTON, buttonNumber, d );
  if (eventID != -1)
      return ar_replaceButtonEvent( eventID, value, d );
  else {
    int numEvents = ar_getNumberEvents( d );
    int numButtons = ar_getNumberEventsType( AR_EVENT_BUTTON, d );
    if ((!d->setDataDimension("indices",numEvents+1)) ||
        (!d->setDataDimension("types",numEvents+1)) ||
        (!d->setDataDimension("buttons",numButtons+1)))
      return false;
    ((int*) d->getDataPtr("indices",AR_INT))[numEvents] = buttonNumber;
    ((int*) d->getDataPtr("types",AR_INT))[numEvents] = AR_EVENT_BUTTON;
    ((int*) d->getDataPtr("buttons",AR_INT))[numButtons] = value;
    ((int*) d->getDataPtr("signature",AR_INT))[0]++;
    return true;
  }
}

bool ar_insertAxisEvent( int axisNumber, const float value, arStructuredData* d ) {
  int eventID = ar_getEventID( AR_EVENT_AXIS, axisNumber, d );
  if (eventID != -1)
      return ar_replaceAxisEvent( eventID, value, d );
  else {
    int numEvents = ar_getNumberEvents( d );
    int numAxes = ar_getNumberEventsType( AR_EVENT_AXIS, d );
    if ((!d->setDataDimension("indices",numEvents+1)) ||
        (!d->setDataDimension("types",numEvents+1)) ||
        (!d->setDataDimension("axes",numAxes+1)))
      return false;
    ((int*) d->getDataPtr("indices",AR_INT))[numEvents] = axisNumber;
    ((int*) d->getDataPtr("types",AR_INT))[numEvents] = AR_EVENT_AXIS;
    ((float*) d->getDataPtr("axes",AR_FLOAT))[numAxes] = value;
    ((int*) d->getDataPtr("signature",AR_INT))[1]++;
    return true;
  }
}

bool ar_insertMatrixEvent( int matrixNumber, const arMatrix4& value, arStructuredData* d ) {
  int eventID = ar_getEventID( AR_EVENT_MATRIX, matrixNumber, d );
  if (eventID != -1)
      return ar_replaceMatrixEvent( eventID, value, d );
  else {
    int numEvents = ar_getNumberEvents( d );
    int numMatrices = ar_getNumberEventsType( AR_EVENT_MATRIX, d );
    if ((!d->setDataDimension("indices",numEvents+1)) ||
        (!d->setDataDimension("types",numEvents+1)) ||
        (!d->setDataDimension("matrices",(numMatrices+1)*16)))
      return false;
    ((int*) d->getDataPtr("indices",AR_INT))[numEvents] = matrixNumber;
    ((int*) d->getDataPtr("types",AR_INT))[numEvents] = AR_EVENT_MATRIX;
    memcpy( ((float*)d->getDataPtr("matrices",AR_FLOAT))+16*numMatrices,
            value.v, 16*sizeof(float) );
    ((int*) d->getDataPtr("signature",AR_INT))[2]++;
    return true;
  }
}

bool ar_setEventIndex(int eventID,int newIndex,arStructuredData* d) {
  if (eventID < 0 || eventID >= ar_getNumberEvents(d))
    return false;

  ((int*) d->getDataPtr("indices",AR_INT))[eventID] = newIndex;
  return true;
}
