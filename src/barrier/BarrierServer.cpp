//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDataUtilities.h"
#include "arBarrierServer.h"
#include "arSZGClient.h"

using namespace std;

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arBarrierServer barrierServer;
  if (!barrierServer.init("SZG_BARRIER", "default", szgClient) || !barrierServer.start())
    return 1;

  arThread dummy(ar_messageTask, &szgClient);
  while (szgClient.running()) {
    barrierServer.activatePassiveSockets(NULL);
    ar_usleep(100000);
  } 
  szgClient.messageTaskStop();
  return 0;
}
