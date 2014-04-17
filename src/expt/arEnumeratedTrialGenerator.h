#ifndef ARENUMERATEDTRIALGENERATOR_H
#define ARENUMERATEDTRIALGENERATOR_H

#include "arTrialGenerator.h"
#include "arDataType.h"
#include "arExperimentDataRecord.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

typedef std::vector< arExperimentDataRecord > arTrialTable_t;

class SZG_CALL arEnumeratedTrialGenerator: public arTrialGenerator {
  public:
    virtual bool init( const std::string experimentName,
                       std::string configPath,
                       const arHumanSubject& subject,
                       arExperimentDataRecord& factors,
                       const arStringSetMap_t& legalStringValues,
                       arSZGClient& SZGClient );
    virtual bool newTrial( arExperimentDataRecord& factors );
    virtual long numberTrials() const;
  private:
    bool validateRecord( arExperimentDataRecord& trialData,
                        arExperimentDataRecord& factors,
                        const arStringSetMap_t& legalStringValues );
    arTrialTable_t _trialTable;
};

#endif        //  #ifndefARENUMERATEDTRIALGENERATOR_H

