#ifndef AREXPERIMENTUTILITIES_H
#define AREXPERIMENTUTILITIES_H

#include "arStructuredData.h"
#include <string>
#include <vector>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

bool SZG_CALL ar_getStringField( arStructuredData* dataPtr,
                         const std::string name,
                         std::string& value );
bool SZG_CALL ar_extractTokenList( arStructuredData* dataPtr,
                           const std::string name,
                           std::vector<std::string>& outList,
                           const char delim );

#endif        //  #ifndefAREXPERIMENTUTILITIES_H

