//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARPFORTHEVENTVOCABULARY_H
#define ARPFORTHEVENTVOCABULARY_H

#include "arPForth.h"

class arPForthFilter;

namespace arPForthSpace {

SZG_CALL bool ar_PForthAddEventVocabulary( arPForth* pf );
SZG_CALL void ar_PForthSetInputEvent( arInputEvent* inputEvent );
SZG_CALL void ar_PForthSetFilter( arPForthFilter* filter );
SZG_CALL arInputEvent* ar_PForthGetCurrentEvent();
SZG_CALL arPForthFilter* ar_PForthGetFilter();

} // namespace arPForthSpace

#endif        //  #ifndefARPFORTHEVENTVOCABULARY_H

