//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfigParser.h"

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
