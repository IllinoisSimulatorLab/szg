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
    cout << "usage: ddelinterface [-t] name address\n";
    return 1;
  }
  if (!parser.parseConfigFile()) {
    // THIS IS NOT AN ERROR!
    cout << "ddelinterface remark: writing new config file.\n";
  }
  return parser.deleteInterface(argv[1], argv[2]) &&
         parser.writeConfigFile() ? 0 : 1;
}
