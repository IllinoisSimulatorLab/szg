//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int main(int argc, char** argv){
  arPhleetConfig config;

  char* host = argv[1];
  char buf[200];
  if (argc == 1){
#ifdef AR_USE_WIN_32
    if (!ar_winSockInit())
      return 1;
#endif
    gethostname(buf, sizeof(buf)-1);
    // Truncate domain name after the period.
    char* pch = strchr(buf, '.');
    if (pch)
      *pch = '\0';
    cout << "dname remark:  defaulting to " << buf << endl;
    host = buf;
  }
  else if (argc != 2){
    cout << "usage: dname [name]\n";
    return 1;
  }

  if (!config.read()) {
    // THIS IS NOT AN ERROR!
    // This can occur the first time one of these program is run.
    cout << "dname remark: writing new config file.\n";
  }
  config.setComputerName(host);
  return config.write() ? 0 : 1;
}
