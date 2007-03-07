//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arPForth.h"
#include "arPForthDatabaseVocabulary.h"
#include "arSZGClient.h"

int main( int argc, char** argv ) {
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arPForth pforth;
  if (!pforth) {
    ar_log_error() << "pfconsole failed to initialize PForth.\n";
    return 1;
  }

  if (!ar_PForthAddDatabaseVocabulary( &pforth )) {
    ar_log_error() << "pfconsole failed to add database vocabulary.\n";
    return 1;
  }

  ar_PForthSetSZGClient( &szgClient );
  string program;
  for (;;) {
    cout << "OK ";
    getline( cin, program );
    if (program == "quit")
      break;
    if (pforth.compileProgram( program ))
      pforth.runProgram();
  }
  return 0;
}
