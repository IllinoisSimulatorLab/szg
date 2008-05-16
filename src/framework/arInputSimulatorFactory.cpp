//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arInputSimulatorFactory.h"

arInputSimulator* arInputSimulatorFactory::createSimulator( arSZGClient& szgClient ) {
  const string simType(szgClient.getAttribute( "SZG_INPUTSIM", "simtype" ));
  if (simType == "NULL") {
    ar_log_debug() << "arInputSimulatorFactory ignoring NULL SZG_INPUTSIM/simtype.\n";
    return NULL;
  }

  ar_log_debug() << "arInputSimulatorFactory: unimplemented SZG_INPUTSIM/simtype '" <<
    simType << "'.\n";
  // As new simulator types become available, test for them here.
  // (perhaps load from dlls?)
  // return NULL by default, telling caller to use its default simulator.
  // Any relevant error messages will be printed in this method.
  return NULL;
}


