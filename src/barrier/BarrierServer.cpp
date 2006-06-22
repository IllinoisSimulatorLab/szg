//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arDataUtilities.h"
#include <iostream>
#include "arBarrierServer.h"
#include "arSZGClient.h"
using namespace std;

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arBarrierServer barrierServer;
  barrierServer.setServiceName("SZG_BARRIER");
  barrierServer.setChannel("default");
  if (!barrierServer.init(szgClient) || !barrierServer.start())
    return 1;

  arThread dummy(ar_messageTask, &szgClient);
  while (true) {
    barrierServer.activatePassiveSockets(NULL);
    ar_usleep(100000);
  } 
  return 0;
}
