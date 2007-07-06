//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** argv){
  arPhleetConfig config;
  if (argc != 3){
    cerr << "usage: ddelinterface name address\n";
    return 1;
  }

  if (!config.read()) {
    // Maybe the first time this program was run.
    cout << "ddelinterface writing new config file.\n";
  }
  return config.deleteInterface(argv[1], argv[2]) &&
         config.write() ? 0 : 1;
}
