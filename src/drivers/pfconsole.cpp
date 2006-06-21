//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arPForth.h"
#include "arPForthDatabaseVocabulary.h"
#include "arSZGClient.h"

int main( int argc, char** argv ) {
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "pfconsole error: SZGClient failed to connect.\n";
    return 1;
  }

  arPForth pforth;
  if (!pforth) {
    cerr << "failed to initialize PForth.\n";
    return 1;
  }

  if (!ar_PForthAddDatabaseVocabulary( &pforth )) {
    cerr << "failed to add database vocabulary.\n";
    return 1;
  }

  ar_PForthSetSZGClient( &szgClient );
  string program;
  cout << "OK ";
  getline( cin, program );
  while (program != "quit") {
    if (pforth.compileProgram( program ))
      pforth.runProgram();
    cout << "OK ";
    getline( cin, program );
  }
  return 0;
}
