#ifndef AREXPERIMENTEVENTFILTER_H
#define AREXPERIMENTEVENTFILTER_H

#include "arSZGAppFramework.h"
#include "arFrameworkEventFilter.h"
#include "arExperiment.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arExperimentEventFilter : public arFrameworkEventFilter {
  public:
    arExperimentEventFilter( arSZGAppFramework* fw, arExperiment* expt=0 );
    virtual ~arExperimentEventFilter() {}
    void setExperiment( arExperiment* expt ) { _expt = expt; }
    arExperiment* getExperiment() { return _expt; }
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
    arExperiment* _expt;
};

#endif        //  #ifndefAREXPERIMENTEVENTFILTER_H

