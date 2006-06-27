//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfigParser.h"

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
