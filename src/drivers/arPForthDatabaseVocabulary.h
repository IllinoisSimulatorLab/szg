//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PFORTH_DATABASE_VOCABULARY_H
#define AR_PFORTH_DATABASE_VOCABULARY_H

#include "arPForth.h"
#include "arSZGClient.h"
#include "arDriversCalling.h"

SZG_CALL bool ar_PForthAddDatabaseVocabulary( arPForth* pf );
SZG_CALL void ar_PForthSetSZGClient( arSZGClient* client );
SZG_CALL arSZGClient* ar_PForthGetSZGClient();

#endif
