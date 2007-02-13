//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDataUtilities.h"
#include "arBarrierClient.h"
#include "arSZGClient.h"
#include "arPhleetConfig.h"

#include <stdlib.h>

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arBarrierClient barrierClient;
  barrierClient.setServiceName("SZG_BARRIER");
  arPhleetConfig config;
  if (!config.read())
    return 1;

  barrierClient.setNetworks(config.getNetworks());
  if (!barrierClient.init(szgClient) || !barrierClient.start())
    return 1;

  arThread dummy(ar_messageTask, &szgClient);
  int count = 0;
  ar_timeval timePrev;
  while (true) {
    while (!barrierClient.checkConnection()){
      ar_usleep(100000);
    }

    // Connected to the barrier server.
    if (!barrierClient.checkActivation()){
      barrierClient.requestActivation();
      count = 0;
      timePrev = ar_time();
    }
    if (!barrierClient.sync())
      cerr << "BarrierClient warning: sync failed.\n";
    const int countMax = 1000;
    if (++count == countMax){
      const ar_timeval timeNow = ar_time();
      count = 0;
      cout << "Barrier sync averages " 
           << ar_difftime(timeNow, timePrev) / float(countMax)
	   << "usec.\n";
      timePrev = ar_time();
    }
  }

  return 0;
}
