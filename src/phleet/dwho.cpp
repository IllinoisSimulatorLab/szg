//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

// prints the log-in information, szgserver name, szgserver IP, szgserver
// port, and user name
int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }

  arPhleetConfig config;
  config.parseLogin();
  config.printLogin();
  return 0;
}
