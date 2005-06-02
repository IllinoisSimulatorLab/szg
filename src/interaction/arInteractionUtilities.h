//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTERACTION_UTILITIES_H
#define AR_INTERACTION_UTILITIES_H

#include "arEffector.h"
#include "arInteractable.h"
#include <list>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arInteractionCalling.h"

SZG_CALL bool ar_pollingInteraction( arEffector& effector,
                                     std::list<arInteractable*>& objects );
SZG_CALL bool ar_pollingInteraction( arEffector& effector,
                                     arInteractable* object );

#endif        //  #ifndefARINTERACTIONUTILITIES_H

