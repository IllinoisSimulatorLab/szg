//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPhleetConfigParser.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv){
  arPhleetConfigParser parser;
  parser.useAlternativeConfigFile(arTFlag(argc, argv));
  if (argc != 3){
    cout << "usage: dports [-t] first size\n";
    return 1;
  }
  if (!parser.parseConfigFile()) {
    // THIS IS NOT AN ERROR
    cout << "dports error: writing new config file.\n";
  }
  parser.setPortBlock(atoi(argv[1]), atoi(argv[2]));
  return parser.writeConfigFile() ? 0 : 1;
}
