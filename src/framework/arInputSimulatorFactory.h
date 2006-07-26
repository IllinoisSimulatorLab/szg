//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_INPUTSIM_FACTORY_H
#define AR_SZG_INPUTSIM_FACTORY_H

#include "arInputSimulator.h"
#include "arSZGClient.h"
#include "arFrameworkCalling.h"

class SZG_CALL arInputSimulatorFactory {
  public:
    arInputSimulator* createSimulator( arSZGClient& szgClient );
};

#endif        //  #ifndefAR_SZG_INPUTSIM_FACTORY_H

