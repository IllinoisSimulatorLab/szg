#ifndef AREVENTUTILITIES_H
#define AREVENTUTILITIES_H

#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arStructuredData.h"

bool ar_setEventQueueFromStructuredData( arInputEventQueue* q, arStructuredData* data );
bool ar_saveEventQueueToStructuredData( arInputEventQueue* q, arStructuredData* data );

bool ar_setInputStateFromStructuredData( arInputState* state, arStructuredData* data );
bool ar_saveInputStateToStructuredData( arInputState* state, arStructuredData* data );

#endif        //  #ifndefAREVENTUTILITIES_H

