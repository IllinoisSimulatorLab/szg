//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPForth.h"
#include "arPForthDatabaseVocabulary.h"
#include "arSZGClient.h"

using std::string;

int main( int argc, char** argv ) {
  arSZGClient client;
  client.init(argc, argv);
  if (!client) {
    cerr << "pfconsole error: SZGCLient failed to connect.\n";
    return 1;
  }
  arPForth pforth;
  if (!pforth) {
    cerr << "failed to initialize PForth.\n";
    return 0;
  }
  if (!arPForthSpace::ar_PForthAddDatabaseVocabulary( &pforth )) {
    cerr << "failed to add database vocabulary.\n";
    return 0;
  }
  arPForthSpace::ar_PForthSetSZGClient( &client );
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
