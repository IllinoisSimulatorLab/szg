//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FILTER_UTILITIES_H
#define AR_FILTER_UTILITIES_H

#include "arStructuredData.h"
#include "arMath.h"
#include "arInputEvent.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

SZG_CALL int ar_getNumberEvents(arStructuredData*);
SZG_CALL int ar_getNumberEventsType(int eventType, arStructuredData*);
SZG_CALL int ar_getEventID(int eventType, int eventNumber, arStructuredData*);
SZG_CALL int ar_getEventType(int eventID,arStructuredData*);
SZG_CALL int ar_getEventIndex(int eventID,arStructuredData*);
SZG_CALL int ar_getEventButtonValue(int eventID,arStructuredData*);
SZG_CALL float ar_getEventAxisValue(int eventID,arStructuredData*);
SZG_CALL arMatrix4 ar_getEventMatrixValue(int eventID,arStructuredData*);
SZG_CALL int ar_getEventButtonNumberValue(int numButton,arStructuredData*);
SZG_CALL float ar_getEventAxisNumberValue(int numAxis,arStructuredData*);
SZG_CALL arMatrix4 ar_getEventMatrixNumberValue(int numMatrix,
                                                arStructuredData*);
SZG_CALL bool ar_replaceButtonEvent(int,int,arStructuredData*);
SZG_CALL bool ar_replaceAxisEvent(int,float,arStructuredData*);
SZG_CALL bool ar_replaceMatrixEvent(int, const arMatrix4&, arStructuredData*);
SZG_CALL bool ar_insertButtonEvent(int buttonNumber, const int value, 
                                   arStructuredData* d);
SZG_CALL bool ar_insertAxisEvent(int axisNumber, const float value, 
                                 arStructuredData* d);
SZG_CALL bool ar_insertMatrixEvent(int matrixNumber, const arMatrix4& value, 
                                   arStructuredData* d);
SZG_CALL bool ar_setEventIndex(int eventID,int newIndex,arStructuredData*);

#endif
