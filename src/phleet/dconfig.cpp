//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** /*argv*/) {
  if (argc != 1) {
    cout << "szg:ERROR:usage: dconfig\n";
    return 1;
  }
  arPhleetConfig config;
  if (!config.read()) {
    cout << "szg:ERROR:failed to parse Syzygy configuration file.\n"
         << "szg:ERROR:  see szg/doc/DistributedOS.html for instructions.\n";
    return 1;
  }
  return config.print() ? 0 : 1;
}
