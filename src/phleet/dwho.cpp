//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
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
