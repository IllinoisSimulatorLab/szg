//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

// Print login info, szgserver name/IP/port, user name.
int main(int argc, char** argv) {
  if (argc != 1) {
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }

  arPhleetConfig config;
  if (config.readLogin())
    return config.printLogin() ? 0 : 1;

  string configLocation, loginLocation;
  config.determineFileLocations( configLocation, loginLocation );
  bool exists, isFile;
  cerr << argv[0] << ": ";
  if (!ar_fileExists( configLocation, exists, isFile )) {
    cerr << configLocation << " existence indeterminate.\n";
  }
  else if (!exists) {
    cerr << "first run dname and daddinterface to create " << configLocation << ".\n";
  }
  else if (!isFile) {
    cerr << configLocation << " is not a file. First run dname and daddinterface to recreate it.\n";
  }
  else if (!ar_fileExists( loginLocation, exists, isFile )) {
    cerr << loginLocation << " existence indeterminate.\n";
  }
  else if (!exists) {
    cerr << "no " << loginLocation << ". First dlogin.\n";
  }
  else if (!isFile) {
    cerr << loginLocation << " is not a file. First dlogin.\n";
  }
  else {
    cerr << loginLocation << " contains no valid user information (file corrupt?). First dlogin.\n";
  }
  return 1;
}
