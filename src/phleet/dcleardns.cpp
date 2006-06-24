//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc<2){
    cerr << "usage: " << argv[0] << " hostname\n";
    return 1;
  }

  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  return szgClient.clearPseudoDNS(string(argv[1])) ? 0 : 1;
}
