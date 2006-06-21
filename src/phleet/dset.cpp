//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSZGClient.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "dset error: failed to initialize SZGClient.\n";
    return 1;
  }

  if (argc != 5 && argc != 3){
    cerr << "usage: " << argv[0]
         << " hostname parameter_group parameter_name value\n";
    cerr << "usage: " << argv[0]
	 << " global_parameter value\n";
    return 1;
  }

  if (argc == 5){
    szgClient.setAttribute(argv[1], argv[2], argv[3], argv[4]);
  }
  else{
    // NOTE: we might be trying to reach into the guts of an XML doc stored
    // in a global parameter. This is triggered if arg 1 is a
    // forward-slash-delimited string (in which case it will give the path
    // to the XML attribute to be modified).
    arSlashString pathList(argv[1]);
    if (pathList.size() > 1){
      szgClient.getSetGlobalXML(argv[1], argv[2]);
    }
    else{
      szgClient.setGlobalAttribute(argv[1], argv[2]);
    }
  }
  return 0;
}
