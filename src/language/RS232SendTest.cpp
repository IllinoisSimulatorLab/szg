//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdlib.h>
#include <iostream>
#include <string>
using namespace std;
#include "arRS232Port.h"

arRS232Port myPort;
const unsigned int portNum = 1;
const unsigned long baud = 9600;
const unsigned int dataBits = 8;
const float stopBits = 1;
const string parity = "none";
char bigThing[100];

int main( int /*argc*/, char** /*argv*/ ) {
  if (!myPort.ar_open( portNum, baud, dataBits, stopBits, parity )) {
    cout << "failed to open port.\n";
    return -1;
  }
  int k = 0;
  for (int i=0; i<100; i++) {
    bigThing[i] = 'A' + static_cast<char>(k);
    k = ++k % 26;
  }
  while (1) {
    const int numWrit = myPort.ar_write( bigThing, 100 );
    if (numWrit != 100)
      cout << "error: ";
    cout << "Wrote " << numWrit << " bytes." << endl;
  } 
  cout << myPort.ar_close() ? "Port closed.\n" : "failed to close port.\n";
  return 0;
}
