#ifndef AREXPERIMENTTRIALPHASE_H
#define AREXPERIMENTTRIALPHASE_H

#include "arSZGAppFramework.h"
#include "arExperimentDataField.h"
#include <string>
#include <map>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class arExperiment;

class SZG_CALL arExperimentTrialPhase {
  public:
    arExperimentTrialPhase( const std::string& name="" ) : _name(name) {}
    virtual ~arExperimentTrialPhase() {}
    virtual bool init( arSZGAppFramework*, arExperiment* ) { return true; }
    virtual bool update( arSZGAppFramework*, arExperiment* ) { return true; }
    virtual bool update( arSZGAppFramework*, arExperiment*, arIOFilter*, arInputEvent& ) { return true; }
    std::string getName() const { return _name; }
  protected:
    std::string _name;
    arTimer _timer;
};

typedef std::map< std::string, arExperimentTrialPhase*, less<std::string> > arExperimentTrialPhaseMap_t;


#endif        //  #ifndefAREXPERIMENTTRIALPHASE_H

