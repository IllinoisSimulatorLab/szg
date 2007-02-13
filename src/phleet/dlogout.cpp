//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** argv){
  if (argc != 1){
    cerr << "usage: " << argv[0] << "\n";
    return 1;
  }
  
  // Reset the login file's contents.
  arPhleetConfig config;
  config.setUserName("NULL");
  config.setServerName("NULL");
  config.setServerIP("NULL");
  config.setServerPort(0);
  return config.writeLogin() ? 0 : 1;
}
