//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    return 1;
  }
  client.printPendingServiceRequests();
}
