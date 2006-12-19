//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arInputSimulatorFactory.h"

arInputSimulator* arInputSimulatorFactory::createSimulator( arSZGClient& szgClient ) {
  string simType = szgClient.getAttribute( "SZG_INPUTSIM", "simtype" );
  ar_log_debug() << "arInputSimulatorFactory: SZG_INPUTSIM/simtype = "
                  << simType << ", but this is currently ignored.\n";
  // As new simulator types become available, test for them here.
  // (perhaps load from dlls?)
  // return NULL by default, telling caller to use its default simulator.
  // Any relevant error messages will be printed in this method.
  return NULL;
}


