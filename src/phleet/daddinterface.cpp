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
    cout << "usage: daddinterface [-t] name address\n";
    return 1;
  }
  if (!parser.parseConfigFile()) {
    // THIS IS NOT AN ERROR!
    // This can occur the first time one of these program is run!
    cout << "daddinterface remark: writing new config file.\n";
  }
  parser.addInterface(argv[1], argv[2]);
  return parser.writeConfigFile() ? 0 : 1;
}
