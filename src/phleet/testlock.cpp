//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc != 2){
    cout << "usage: testlock lock_name\n";
    return 1;
  }

  int ownerID = -1;
  for (;;){
    if (!szgClient.getLock(argv[1], ownerID)){
      cout << argv[0] << " warning: unable to get lock.\n";
      cout << "  Will try again when the current holder releases.\n";
      int match = szgClient.requestLockReleaseNotification(argv[1]);
      list<int> tags;
      tags.push_back(match);
      if (szgClient.getLockReleaseNotification(tags) < 0){
        cout << argv[0] << " error: mismatched lock release notification.\n";
        return 1;
      }
      if (!szgClient.getLock(argv[1], ownerID)){
        cout << argv[0] << " error: failed twice to get lock.\n";
        return 1;
      }
    }

    cout << argv[0] << " remark: holding lock for 10 seconds.\n";
    ar_usleep(10000000);
    szgClient.releaseLock(argv[1]);
    cout << argv[0] << "remark: will try to get lock again in 10 seconds.\n";
    ar_usleep(10000000);
  }
  return 0;
}
