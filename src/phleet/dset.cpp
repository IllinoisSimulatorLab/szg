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

  if (argc != 5 && argc != 3) {
    cerr << "usage: " << argv[0] << " hostname parameter_group parameter_name value\n"
         << "usage: " << argv[0] << " global_parameter value\n";
    return 1;
  }

  if (argc == 5) {
    szgClient.setAttribute(argv[1], argv[2], argv[3], argv[4]);
    return 0;
  }

  if (arSlashString(argv[1]).size() > 1) {
    // Arg 1 is slash-delimited.  Interpret it as a path to
    // an XML attribute to be modified in an XML doc stored in a global parameter.
    szgClient.getSetGlobalXML(argv[1], argv[2]);
  }
  else{
    szgClient.setGlobalAttribute(argv[1], argv[2]);
  }

  return 0;
}
