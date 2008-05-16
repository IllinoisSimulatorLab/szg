//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

// prints the log-in information, szgserver name, szgserver IP, szgserver
// port, and user name
int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }

  arPhleetConfig config;
  if (!config.readLogin()) {
    string configLocation, loginLocation;
    config.determineFileLocations( configLocation, loginLocation );
    bool exists, isFile;
    if (!ar_fileExists( configLocation, exists, isFile )) {
      cerr << "Could not determine whether or not " << configLocation << " exists?!?\n";
      return 1;
    }
    if (!exists) {
      cerr << configLocation << " does not exist. You must use dname and daddinterface to create it.\n";
      return 1;
    }
    if (!isFile) {
      cerr << configLocation << " exists, but is not a file. You must delete or move it, then use dname and daddinterface to create the file.\n";
      return 1;
    }
    if (!ar_fileExists( loginLocation, exists, isFile )) {
      cerr << "Could not determine whether or not " << loginLocation << " exists?!?\n";
      return 1;
    }
    if (!exists) {
      cerr << loginLocation << " does not exist. You must dlogin.\n";
      return 1;
    }
    if (!isFile) {
      cerr << loginLocation << " exists, but is not a file. You must delete or move it, then dlogin\n";
      return 1;
    }
    cerr << "Login file exists, but does not contain valid user information (possible file corruption?).\n"
         << "Suggest you dlogin to re-create it.\n";
  }
  return config.printLogin() ? 0 : 1;
}
