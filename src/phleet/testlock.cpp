//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc != 2){
    cout << "usage: testlock <lock name>\n";
    return 1;
  }
  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    return 1;
  }
  int ownerID = -1;
  for (;;){
    if (!client.getLock(argv[1], ownerID)){
      cout << argv[0] << " warning: unable to get lock.\n";
      cout << "  Will try again when the current holder releases.\n";
      int match = client.requestLockReleaseNotification(argv[1]);
      list<int> tags;
      tags.push_back(match);
      if (client.getLockReleaseNotification(tags) < 0){
        cout << argv[0] << " error: mismatched lock release notification.\n";
        return 1;
      }
      if (!client.getLock(argv[1], ownerID)){
        cout << argv[0] << " error: failed twice to get lock.\n";
        return 1;
      }
    }
    cout << argv[0] << " remark: holding lock for 10 seconds.\n";
    int i;
    for (i=0; i<10; i++){
      cout << i << "\n";
      ar_usleep(1000000);
    }
    client.releaseLock(argv[1]);
    cout << argv[0] << "remark: will try to obtain lock again in 10 seconds.\n";
    for (i=0; i<10; i++){
      cout << i << "\n";
      ar_usleep(1000000);
    }
  }
  return 0;
}
