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

int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }
  
  // Reset the login file's contents.
  arPhleetConfigParser parser;
  parser.setUserName("NULL");
  parser.setServerName("NULL");
  parser.setServerIP("NULL");
  parser.setServerPort(0);
  return parser.writeLoginFile() ? 0 : 1;
}
