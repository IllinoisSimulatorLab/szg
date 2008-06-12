//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

int main(int argc, char** argv) {
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  const bool fAll = argc >= 2 && !strcmp(argv[1], "-a");
  if ((fAll && argc > 3) || (!fAll && (argc==1 || argc==3 || argc>4))) {
    ar_log_critical() << "usage:\n  " <<
      "dget computer parameter_group parameter_name\n  " <<
      "dget global_parameter_name\n  " <<
      "dget -a             (Produce a dbatch-file.)\n  " <<
      "dget -a substring\n  ";
    return 1;
  }

  if (fAll) {
    cout << szgClient.getAllAttributes(argc==3 ? argv[2] : "ALL");
  }
  else{
    if (argc==2) {
      // NOTE: with the global attributes we might very well be trying to
      // reach down into the guts of an XML representation (which is
      // assumed if the first arg is a forward slash-delimited path.
      arSlashString pathList(argv[1]);
      if (pathList.size() > 1) {
        cout << szgClient.getSetGlobalXML(pathList) << endl;
      }
      else{
        cout << szgClient.getGlobalAttribute(argv[1]) << endl;
      }
    }
    else{
      // argc = 4
      cout << szgClient.getAttribute(argv[1], argv[2], argv[3], "") << endl;
    }
  }
  return 0;
}
