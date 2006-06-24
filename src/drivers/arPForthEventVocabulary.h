//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PFORTH_EVENT_VOCABULARY_H
#define AR_PFORTH_EVENT_VOCABULARY_H

#include "arPForth.h"
#include "arDriversCalling.h"

class arPForthFilter;

SZG_CALL bool ar_PForthAddEventVocabulary( arPForth* pf );
SZG_CALL void ar_PForthSetInputEvent( arInputEvent* inputEvent );
SZG_CALL void ar_PForthSetFilter( arPForthFilter* filter );
SZG_CALL arInputEvent* ar_PForthGetCurrentEvent();
SZG_CALL arPForthFilter* ar_PForthGetFilter();

#endif
