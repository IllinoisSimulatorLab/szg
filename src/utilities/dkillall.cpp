//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc > 2) {
    ar_log_error() << "usage: dkillall [-a | virtual_computer]\n";
    (void)szgClient.sendInitResponse(false);
    return 1;
  }

  if (argc == 2 && !strcmp(argv[1], "-a")) {
    // Kill all non-szgd apps in process table.
    const string r = szgClient.getProcessList();
    list<int> ids;

    // Strip out szgds (and us ourselves?).  Rough copypaste of dps.cpp

    for (unsigned i=0; i < r.length(); ++i) {
      // Parse one process-line.
      int iBuf = 0;
      char szProcess[256];
      while (r[i] != ':' && i < r.length())
	szProcess[iBuf++] = r[i++];
      szProcess[iBuf] = '\0';

      // Skip "hostname/szgd/number" lines.
      char* pch = strstr(szProcess, "/szgd/");
      if (pch && strspn(pch+6,"0123456789") == strlen(pch+6))
	continue;
      // Skip dkillall, i.e. this program itself.
      pch = strstr(szProcess, "/dkillall/");
      if (pch && strspn(pch+10,"0123456789") == strlen(pch+10))
	continue;

      // Extract process id.
      pch = strrchr(szProcess, '/')+1;
      ids.push_back(atoi(pch));
    }

    szgClient.killIDs(&ids);
    return 0;
  }

  arAppLauncher launcher("dkillall");
  if (argc == 2) {
    launcher.setSZGClient(&szgClient);
    if (!launcher.setVircomp(argv[1])) {
      return 1;
    }
  }
  return launcher.killAll() ? 0 : 1;
}
