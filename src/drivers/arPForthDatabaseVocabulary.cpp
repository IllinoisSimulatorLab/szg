//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPForth.h"
#include "arDataUtilities.h"
#include "arSZGClient.h"

namespace arPForthSpace {

// variables

arSZGClient* __PForthSZGClient = 0;

// functions

void ar_PForthSetSZGClient( arSZGClient* client ) {
  __PForthSZGClient = client;
}
arSZGClient* ar_PForthGetSZGClient() {
  return __PForthSZGClient;
}

// Run-time behaviors

class GetDatabaseParameter : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetDatabaseParameter::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  long paramValAddress    = (long)pf->stackPop();
  long paramNameAddress = (long)pf->stackPop();
  long groupNameAddress = (long)pf->stackPop();
  std::string groupName = pf->getString( groupNameAddress );
  std::string paramName = pf->getString( paramNameAddress );
  std::string paramVal = arPForthSpace::__PForthSZGClient->getAttribute( groupName, paramName );
  cerr << groupName << ", " << paramName << ", " << paramVal << endl;
  pf->putString( paramValAddress, paramVal );
  return true;
}

class GetFloatDatabaseParameters : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetFloatDatabaseParameters::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  long paramValAddress    = (long)pf->stackPop();
  unsigned long numParams = (unsigned long)pf->stackPop();
  long paramNameAddress = (long)pf->stackPop();
  long groupNameAddress = (long)pf->stackPop();
  float *ptr = new float[numParams];
  if (!ptr)
    throw arPForthException("Failed to allocate float pointer.");
  std::string groupName = pf->getString( groupNameAddress );
  std::string paramName = pf->getString( paramNameAddress );
  bool stat = arPForthSpace::__PForthSZGClient->getAttributeFloats( groupName, paramName, ptr, (int)numParams );
  if (!stat)
    throw arPForthException("getAttributeFloats() failed.");
  pf->putDataArray( paramValAddress, ptr, numParams );
  delete[] ptr;
  return true;
}


// installer function

bool ar_PForthAddDatabaseVocabulary( arPForth* pf ) {
  if (!pf->addSimpleActionWord( "getStringParameter", new GetDatabaseParameter() ))
    return false;
  if (!pf->addSimpleActionWord( "getFloatParameters", new GetFloatDatabaseParameters() ))
    return false;
  return true;
}

};

