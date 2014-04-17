#include "arPrecompiled.h"
#include "arExperimentEventFilter.h"

arExperimentEventFilter::arExperimentEventFilter( arSZGAppFramework* fw, arExperiment* expt ) :
  arFrameworkEventFilter(fw),
  _expt(expt) {
}
bool arExperimentEventFilter::_processEvent( arInputEvent& event ) {
  if (!_expt) {
    cerr << "arExperimentEventFilter error: NULL experiment pointer.\n";
    return false;
  }
  _expt->updateTrialPhase( getFramework(), this, event );
  return true;
}


