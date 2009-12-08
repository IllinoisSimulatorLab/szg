//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_INPUTSIM_FACTORY_H
#define AR_SZG_INPUTSIM_FACTORY_H

#include "arSZGClient.h"
#include "arInputSimulator.h"
#include "arFrameworkCalling.h"

class SZG_CALL arInputSimulatorFactory {
  public:
    arInputSimulator* createSimulator( arSZGClient& szgClient );
  private:
#if defined( AR_USE_MINGW ) || defined( AR_LINKING_DYNAMIC) || !defined( AR_USE_WIN_32 )
    string _execPath;
#endif
};

#endif        //  #ifndefAR_SZG_INPUTSIM_FACTORY_H

