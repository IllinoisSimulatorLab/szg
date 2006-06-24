//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPhleetConfigParser.h"

#include <iostream>

int main(int argc, char** argv){
  arPhleetConfigParser parser;

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

  if (!parser.parseConfigFile()) {
    // THIS IS NOT AN ERROR!
    // This can occur the first time one of these program is run.
    cout << "dname remark: writing new config file.\n";
  }
  parser.setComputerName(host);
  return parser.writeConfigFile() ? 0 : 1;
}
