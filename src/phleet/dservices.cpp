//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSZGClient.h"

int main(int argc, char** argv){
  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    return 1;
  }
  client.printServices();
}
