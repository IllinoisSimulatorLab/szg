//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTERACTION_UTILITIES_H
#define AR_INTERACTION_UTILITIES_H

#include "arEffector.h"
#include "arInteractable.h"
#include "arInteractionCalling.h"

#include <list>

SZG_CALL bool ar_pollingInteraction( arEffector&, list<arInteractable*>& );
SZG_CALL bool ar_pollingInteraction( arEffector&, arInteractable* );

#endif
