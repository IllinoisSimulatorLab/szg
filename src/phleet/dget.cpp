//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"

int main(int argc, char** argv){
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient; 
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "dget error: failed to initialize SZGClient.\n";
    return 1;
  }

  const bool fAll = (argc==2 || argc==3) && 
                    !strcmp(argv[1], "-a");
  if (argc<4 && !fAll){
    cerr << "usage:\n  "
         << argv[0] << " computer parameter_group parameter_name\n"
	 << argv[0] << " -a             (Produce a dbatch-file.)\n"
	 << argv[0] << " -a substring\n";
    return 1;
  }

  if (fAll){
    cout << szgClient.getAllAttributes(argc==3 ? argv[2] : "ALL");
  }
  else{
    cout << szgClient.getAttribute(argv[1], argv[2], argv[3], "") << endl;
  }
  return 0;
}
