//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfigParser.h"

int main(int argc, char** /*argv*/){
  if (argc != 1){
    cout << "usage: dconfig\n";
    return 1;
  }
  arPhleetConfigParser parser;
  if (!parser.parseConfigFile()){
    cerr << "dconfig error: failed to parse phleet configuration file.\n";
    return 1;
  }
  parser.printConfig();
  return 0;
}
