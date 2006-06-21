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
  if (argc != 3 && argc != 4){
    cout << "usage: daddinterface name address [netmask]\n";
    return 1;
  }
  if (!parser.parseConfigFile()) {
    // Maybe this is the first time the program has been run.
    cout << "daddinterface remark: writing new config file.\n";
  }

  const string netmask((argc == 4) ? argv[3] : "255.255.255.0");
  parser.addInterface(argv[1], argv[2], netmask);
  return parser.writeConfigFile() ? 0 : 1;
}
