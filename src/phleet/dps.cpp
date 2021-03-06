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

  const string result = szgClient.getProcessList();
  for (unsigned place=0; place < result.length(); ++place) {
    // Build a line.
    int bufferLoc = 0;
    char buffer[256];
    while (result[place] != ':' && place < result.length())
      buffer[bufferLoc++] = result[place++];
    buffer[bufferLoc] = '\0';

    // Skip "hostname/dps/number" lines (i.e., this program itself).
    const char* pch = strstr(buffer, "/dps/");
    if (pch && strspn(pch+5, "0123456789") == strlen(pch+5))
      continue;

    // Print a line.
    // If no arg was given, print all lines.
    // Otherwise only print lines containing argv[1].
    if (argc <= 1 || strstr(buffer, argv[1]))
      cout << buffer << endl;
  }
  return 0;
}
