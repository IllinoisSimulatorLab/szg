//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arPhleetConfigParser.h"
#include <iostream>
using namespace std;

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
