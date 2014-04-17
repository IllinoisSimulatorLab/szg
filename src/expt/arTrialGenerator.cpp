#include "arPrecompiled.h"
#include "arTrialGenerator.h"

bool arTrialGenerator::convertFactor( const string& theString, const arDataType theType,
                                      const void* theAddress ) {
  switch (theType) {
    case AR_DOUBLE:
      if (!ar_stringToDoubleValid( theString, *(double*)theAddress )) {
        cerr << "arTrialGenerator error: failed string->double conversion of " << theString << ")\n.";
        return false;
      }
      break;
    case AR_LONG:
       if (!ar_stringToLongValid( theString, *(long*)theAddress )) {
        cerr << "arTrialGenerator error: failed string->long conversion of " << theString << ")\n.";
        return false;
      }
      break;
    case AR_CHAR:
      memcpy( const_cast<void*>(theAddress), theString.c_str(), theString.size() );
      // put trailing '\0'...
      *(((char*)theAddress)+theString.size()) = '\0';
      break;
    default:
      cerr << "arTrialGenerator error: invalid factor type.\n";
      return false;
  }
  return true;
}

