#include "arPrecompiled.h"
#include <string>
#include "arTrialGenBuilder.h"
#include "arEnumeratedTrialGenerator.h"

arTrialGenerator* arTrialGenBuilder::build( arSZGClient& SZGClient ) {
  // Select trial generator type and run it.
  std::string trialGenType = SZGClient.getAttribute("SZG_EXPT", "method");
  arTrialGenerator* gen = this->build( trialGenType );
  if (gen == 0) {
    cerr << "    Check the value of SZG_EXPT/method.\n";
  }
  return gen;
}

arTrialGenerator* arTrialGenBuilder::build( const std::string& trialGenMethod ) {
  if (trialGenMethod == "enumerated") {
    cerr << "arExperiment remark: SZG_EXPT/method == " << trialGenMethod << endl;
    arTrialGenerator* trialGen = new arEnumeratedTrialGenerator;
    if (trialGen==0)
      cerr << "arExperiment error: couln't create arEnumeratedTrialGenerator.\n";
    return trialGen;
  } else {
    cerr << "arExperiment error: " << trialGenMethod << " is not a valid method.\n"
         << "    Valid values are: enumerated\n"
         << "    That is all.\n";
    return (arTrialGenerator*)0;
  }
}

