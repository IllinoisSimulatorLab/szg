//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  if (argc<2){
    cerr << "usage: " << argv[0] << " hostname\n";
    return 1;
  }

  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  const string hostname(argv[1]);
  return szgClient.clearPseudoDNS(hostname) ? 0 : 1;
}
