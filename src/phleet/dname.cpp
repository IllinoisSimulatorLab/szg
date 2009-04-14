//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfig.h"

int printusage() {
  cout << "dname ERROR: usage: dname <szgname>' (set name)\n"
       << "dname ERROR: usage: dname -hostname' (set name automatically)\n"
       << "dname ERROR: usage: dname (print current name)\n";
  return 1;
}

int main(int argc, char** argv) {
  if ((argc != 1)&&(argc != 2)) {
    return printusage();
  }

  arPhleetConfig config;
  char* host = argv[1];
  char buf[200];
  bool ok = false;

  if (argc == 1) {
    if (!config.read()) {
      cout << "dname ERROR: computer's Syzygy name has not been set.\n";
      return printusage();
    }
    ok = true;

  } else {

    if (!strcmp( host, "-hostname" )) {
#ifdef AR_USE_WIN_32
      if (!ar_winSockInit())
        return 1;
#endif
      gethostname(buf, sizeof(buf)-1);
      // Truncate domain name after the period.
      char* pch = strchr(buf, '.');
      if (pch)
        *pch = '\0';
      cout << "dname REMARK: gethostname() returned '" << buf << "'.\n";
      host = buf;
    }

    bool fNewFile = config.read();
    config.setComputerName(host);
    ok = config.write();
    if (ok && fNewFile)
      cout << "dname REMARK: wrote new config file.\n";
  }

  if (ok)
    cout << "dname CRITICAL: computer's Syzygy name is '" << config.getComputerName() << "'.\n";
  return ok ? 0 : 1;
}
