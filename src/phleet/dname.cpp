//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

void printusage() {
    cout << "szg:ERROR:usage: 'dname [name]' (to set manually) or 'dname -hostname' (to set automatically).\n"
         << "szg:ERROR:       'dname' prints current name.\n";
}

int main(int argc, char** argv) {
  arPhleetConfig config;

  char* host = argv[1];
  char buf[200];
  int stat;

  if ((argc != 1)&&(argc != 2)) {
    printusage();
    return 1;
  }

  if (argc == 1) {
    if (!config.read()) {
      cout << "szg:ERROR:Computer's Syzygy name has not been set.\n";
      printusage();
      return 1;
    }
    stat = 0;
  } else {
    if (strcmp( host, "-hostname" )==0) {
#ifdef AR_USE_WIN_32
      if (!ar_winSockInit())
        return 1;
#endif
      gethostname(buf, sizeof(buf)-1);
      // Truncate domain name after the period.
      char* pch = strchr(buf, '.');
      if (pch)
        *pch = '\0';
      cout << "szg:REMARK: gethostname() returned the name " << buf << endl;
      host = buf;
    }

    if (!config.read()) {
      // THIS IS NOT AN ERROR!
      // This can occur the first time one of these program is run.
      cout << "szg:REMARK: writing new config file.\n";
    }
    config.setComputerName(host);
    stat = config.write() ? 0 : 1;
  }
  cout << "szg:CRITICAL:Computer name = " << config.getComputerName() << endl;
  return stat;
}
