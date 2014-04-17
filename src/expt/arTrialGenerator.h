#ifndef ARTRIALGENERATOR_H
#define ARTRIALGENERATOR_H

#include <string>
#include <vector>
#include "arSZGClient.h"
#include "arExperimentDataField.h"
#include "arExperimentDataRecord.h"
#include "arHumanSubject.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arTrialGenerator {
  public:
    arTrialGenerator( const std::string& comment="" ) : _trialNumber(0), _comment(comment) {}
    virtual ~arTrialGenerator() {}
    
    std::string comment() { return _comment; }
    
    virtual bool init( const std::string /*experiment*/,
                       std::string /*configPath*/,
                       const arHumanSubject& /*subject*/,
                       arExperimentDataRecord& /*factors*/,
                       const arStringSetMap_t& /*legalStringValues*/,
                       arSZGClient& /*SZGClient*/ ) { return true; }
                    
    virtual bool newTrial( arExperimentDataRecord& factors ) = 0;
    unsigned long trialNumber() const { return _trialNumber; }
    virtual long numberTrials() const { return -1; }
  protected:
    unsigned long _trialNumber;
    std::string _comment;
    bool convertFactor( const std::string& theName, const arDataType theType, const void* theAddress );
};

#endif        //  #ifndefARTRIALGENERATOR_H

