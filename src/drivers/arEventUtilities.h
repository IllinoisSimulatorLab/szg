#ifndef AREVENTUTILITIES_H
#define AREVENTUTILITIES_H

#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arStructuredData.h"

SZG_CALL bool ar_setEventQueueFromStructuredData( arInputEventQueue* q, 
                                                  arStructuredData* data );
SZG_CALL bool ar_saveEventQueueToStructuredData( arInputEventQueue* q, 
                                                 arStructuredData* data );

SZG_CALL bool ar_setInputStateFromStructuredData( arInputState* state, 
                                                  arStructuredData* data );
SZG_CALL bool ar_saveInputStateToStructuredData( arInputState* state, 
                                                 arStructuredData* data );

#endif        //  #ifndefAREVENTUTILITIES_H

