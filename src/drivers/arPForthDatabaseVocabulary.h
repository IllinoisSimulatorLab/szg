#ifndef ARPFORTHDATABASEVOCABULARY_H
#define ARPFORTHDATABASEVOCABULARY_H

#include "arPForth.h"

class arSZGClient;

namespace arPForthSpace {

bool ar_PForthAddDatabaseVocabulary( arPForth* pf );
void ar_PForthSetSZGClient( arSZGClient* client );
arSZGClient* ar_PForthGetSZGClient();

} // namespace arPForthSpace

#endif        //  #ifndefARPFORTHDATABASEVOCABULARY_H

