//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** /*argv*/){
  if (argc != 1){
    cout << "usage: dconfig\n";
    return 1;
  }
  arPhleetConfig config;
  if (!config.read()){
    cerr << "dconfig error: failed to parse phleet configuration file.\n";
    return 1;
  }
  return config.print() ? 0 : 1;
}
