//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILTER_UTILITIES_H
#define AR_FILTER_UTILITIES_H

#include "arStructuredData.h"
#include "arMath.h"
#include "arInputEvent.h"

//enum {AR_EVENT_GARBAGE=-1, AR_EVENT_BUTTON=0, 
//      AR_EVENT_AXIS=1, AR_EVENT_MATRIX=2};

int ar_getNumberEvents(arStructuredData*);
int ar_getNumberEventsType(int eventType, arStructuredData*);
int ar_getEventID(int eventType, int eventNumber, arStructuredData*);
int ar_getEventType(int eventID,arStructuredData*);
int ar_getEventIndex(int eventID,arStructuredData*);
int ar_getEventButtonValue(int eventID,arStructuredData*);
float ar_getEventAxisValue(int eventID,arStructuredData*);
arMatrix4 ar_getEventMatrixValue(int eventID,arStructuredData*);
int ar_getEventButtonNumberValue(int numButton,arStructuredData*);
float ar_getEventAxisNumberValue(int numAxis,arStructuredData*);
arMatrix4 ar_getEventMatrixNumberValue(int numMatrix,arStructuredData*);
bool ar_replaceButtonEvent(int,int,arStructuredData*);
bool ar_replaceAxisEvent(int,float,arStructuredData*);
bool ar_replaceMatrixEvent(int, const arMatrix4&, arStructuredData*);
bool ar_insertButtonEvent(int buttonNumber, const int value, arStructuredData* d);
bool ar_insertAxisEvent(int axisNumber, const float value, arStructuredData* d);
bool ar_insertMatrixEvent(int matrixNumber, const arMatrix4& value, arStructuredData* d);
bool ar_setEventIndex(int eventID,int newIndex,arStructuredData*);

#endif
