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
  
  if (argc < 2){
    cerr << "usage: dbatch filename.txt\n";
    return 1;
  }

  szgClient.parseParameterFile(argv[1]);
  return 0;
}
