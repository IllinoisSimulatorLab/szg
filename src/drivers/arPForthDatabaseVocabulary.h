//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARPFORTHDATABASEVOCABULARY_H
#define ARPFORTHDATABASEVOCABULARY_H

#include "arPForth.h"

class arSZGClient;

namespace arPForthSpace {

SZG_CALL bool ar_PForthAddDatabaseVocabulary( arPForth* pf );
SZG_CALL void ar_PForthSetSZGClient( arSZGClient* client );
SZG_CALL arSZGClient* ar_PForthGetSZGClient();

} // namespace arPForthSpace

#endif        //  #ifndefARPFORTHDATABASEVOCABULARY_H

