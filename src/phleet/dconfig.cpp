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
