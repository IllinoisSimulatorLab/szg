//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include <stdlib.h>
#include <iostream>
#include "arBarrierClient.h"
#include "arSZGClient.h"
#include "arPhleetConfigParser.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

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
    // Wait until we are connected to the server.
    while (!barrierClient.checkConnection()){
      ar_usleep(100000);
    }
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
}
