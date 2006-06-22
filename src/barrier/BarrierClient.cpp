//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arDataUtilities.h"
#include <stdlib.h>
#include <iostream>
#include "arBarrierClient.h"
#include "arSZGClient.h"
#include "arPhleetConfigParser.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arBarrierClient barrierClient;
  barrierClient.setServiceName("SZG_BARRIER");
  arPhleetConfigParser parser;
  if (!parser.parseConfigFile())
    return 1;

  barrierClient.setNetworks(parser.getNetworks());
  if (!barrierClient.init(szgClient) || !barrierClient.start())
    return 1;

  arThread dummy(ar_messageTask, &szgClient);
  int count = 0;
  ar_timeval time1, time2;
  while (true) {
    while (!barrierClient.checkConnection()){
      ar_usleep(100000);
    }
    // Connected to the server.
    if (!barrierClient.checkActivation()){
      barrierClient.requestActivation();
      count = 0;
      time1 = ar_time();
    }
    if (!barrierClient.sync())
      cerr << "BarrierClient warning: sync failed.\n";
    ++count;
    if (count%1000 == 0){
      time2 = ar_time();
      cout << "Average time for one barrier sync = " 
           << ar_difftime(time2,time1)/1000.0 << "usec\n";
      time1 = ar_time();
    }
  }
  return 0;
}
