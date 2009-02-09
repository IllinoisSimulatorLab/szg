//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arSharedLib.h"
#include "arInputSimulatorFactory.h"

arInputSimulator* arInputSimulatorFactory::createSimulator( arSZGClient& szgClient ) {
#if defined( AR_LINKING_DYNAMIC ) || defined( AR_USE_MINGW )
  const string simType(szgClient.getAttribute( "SZG_INPUTSIM", "sim_type" ));
  if (simType == "NULL") {
    ar_log_debug() << "arInputSimulatorFactory ignoring NULL SZG_INPUTSIM/sim_type.\n";
    return NULL;
  }

  // A dynamically loaded library.
  arSharedLib* inputSimSharedLib = new arSharedLib();
  if (!inputSimSharedLib) {
    ar_log_error() << "arInputSimulatorFactory::getInputSimulator out of memory.\n";
    ar_log_error() << "    Using default arInputSimulator.\n";
    return NULL;
  }
  string error;
  if (!inputSimSharedLib->createFactory( simType, _execPath, "arInputSimulator", error )) {
    ar_log_error() << error;
    ar_log_error() << "    Using default arInputSimulator.\n";
    return NULL;
  }
  arInputSimulator* theSim = (arInputSimulator*) inputSimSharedLib->createObject();
  delete inputSimSharedLib;
  ar_log_critical() << "Loaded input simulator '" << simType << "'.\n";
  return theSim;
#else
  ar_log_warning() << "Loading input simulator plugins requires dynamic linking or MinGW compilation on Win32.\n";
  return NULL;
#endif
}

