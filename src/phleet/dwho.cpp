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

/// prints the log-in information, szgserver name, szgserver IP, szgserver
/// port, and user name
int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }

  arPhleetConfigParser parser;
  parser.parseLoginFile();
  parser.printLogin();
  return 0;
}
