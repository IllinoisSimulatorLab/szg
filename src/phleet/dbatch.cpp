//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSZGClient.h"
#include <stdio.h>

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "dbatch error: failed to initialize SZGClient.\n";
    return 1;
  }
  
  if (argc<2){
    cerr << "usage: " << argv[0] << " batch_file\n";
    return 1;
  }

  szgClient.parseParameterFile(argv[1]);
  return 0;
}
