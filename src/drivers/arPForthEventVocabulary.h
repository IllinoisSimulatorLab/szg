#ifndef ARPFORTHEVENTVOCABULARY_H
#define ARPFORTHEVENTVOCABULARY_H

#include "arPForth.h"

class arPForthFilter;

namespace arPForthSpace {

bool ar_PForthAddEventVocabulary( arPForth* pf );
void ar_PForthSetInputEvent( arInputEvent* inputEvent );
void ar_PForthSetFilter( arPForthFilter* filter );
arInputEvent* ar_PForthGetCurrentEvent();
arPForthFilter* ar_PForthGetFilter();

} // namespace arPForthSpace

#endif        //  #ifndefARPFORTHEVENTVOCABULARY_H

