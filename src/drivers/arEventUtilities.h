//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_EVENT_UTILITIES_H
#define AR_EVENT_UTILITIES_H

#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arStructuredData.h"
#include "arDriversCalling.h"

SZG_CALL bool ar_setEventQueueFromStructuredData(
  arInputEventQueue*, arStructuredData*);
SZG_CALL bool ar_saveEventQueueToStructuredData(
  arInputEventQueue*, arStructuredData*);

SZG_CALL bool ar_setInputStateFromStructuredData(
  arInputState*, arStructuredData*);
SZG_CALL bool ar_saveInputStateToStructuredData(
  arInputState*, arStructuredData*);

#endif
