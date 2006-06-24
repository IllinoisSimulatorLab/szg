//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfigParser.h"

#include <iostream>

int main(int argc, char** argv){
  arPhleetConfigParser parser;
  if (argc != 3){
    cout << "usage: dports first size\n";
    return 1;
  }
  if (!parser.parseConfigFile()) {
    // THIS IS NOT AN ERROR
    cout << "dports error: writing new config file.\n";
  }
  parser.setPortBlock(atoi(argv[1]), atoi(argv[2]));
  return parser.writeConfigFile() ? 0 : 1;
}
