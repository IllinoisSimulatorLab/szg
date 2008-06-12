//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** argv) {
  arPhleetConfig config;
  if (argc != 3) {
    cout << "usage: dports first size\n";
    return 1;
  }
  if (!config.read()) {
    // THIS IS NOT AN ERROR
    cout << "dports error: writing new config file.\n";
  }
  config.setPortBlock(atoi(argv[1]), atoi(argv[2]));
  return config.write() ? 0 : 1;
}
