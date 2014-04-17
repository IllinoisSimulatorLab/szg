#ifndef ARTRIALGENBUILDER_H
#define ARTRIALGENBUILDER_H

#include "arSZGClient.h"
#include "arTrialGenerator.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arTrialGenBuilder {
  public:
    arTrialGenerator* build( arSZGClient& SZGClient );
    arTrialGenerator* build( const std::string& trialGenMethod );
};
  
#endif        //  #ifndefARTRIALGENBUILDER_H

