//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_EVENT_UTILITIES_H
#define AR_EVENT_UTILITIES_H

#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arStructuredData.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

SZG_CALL bool ar_setEventQueueFromStructuredData( arInputEventQueue* q, 
                                                  arStructuredData* data );
SZG_CALL bool ar_saveEventQueueToStructuredData( arInputEventQueue* q, 
                                                 arStructuredData* data );

SZG_CALL bool ar_setInputStateFromStructuredData( arInputState* state, 
                                                  arStructuredData* data );
SZG_CALL bool ar_saveInputStateToStructuredData( arInputState* state, 
                                                 arStructuredData* data );

#endif        //  #ifndefAREVENTUTILITIES_H

