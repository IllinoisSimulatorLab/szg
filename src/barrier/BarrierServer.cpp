//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include <iostream>
#include "arBarrierServer.h"
#include "arSZGClient.h"
using namespace std;

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

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
