//********************************************************
// Syzygy is licensed under the BSD license v2
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
    cout << "usage: ddelinterface name address\n";
    return 1;
  }

  if (!parser.parseConfigFile()) {
    // Maybe the first time this program was run.
    cout << "ddelinterface remark: writing new config file.\n";
  }
  return parser.deleteInterface(argv[1], argv[2]) &&
         parser.writeConfigFile() ? 0 : 1;
}
