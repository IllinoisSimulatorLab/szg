//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARINTERACTIONUTILITIES_H
#define ARINTERACTIONUTILITIES_H

#include "arEffector.h"
#include "arInteractable.h"
#include <list>

SZG_CALL bool ar_pollingInteraction( arEffector& effector,
                                     std::list<arInteractable*>& objects );
SZG_CALL bool ar_pollingInteraction( arEffector& effector,
                                     arInteractable* object );

#endif        //  #ifndefARINTERACTIONUTILITIES_H

