//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <iostream>
#include <string>
using namespace std;
#include "arRS232Port.h"

int main(int, char**) {
  arRS232Port myPort;
  const unsigned int portNum = 1;  // Win32 COM1, Linux /dev/ttyS0
  const unsigned long baud = 9600;
  const unsigned int dataBits = 8;
  const float stopBits = 1;
  const string parity("none");

  if (!myPort.ar_open( portNum, baud, dataBits, stopBits, parity )) {
    cerr << "failed to open port.\n";
    return -1;
  }

  myPort.setReadTimeout( 100 ); // 10 seconds
  char buf[102];
  do {
    const int numRead = myPort.ar_read( buf, 100 );  // Read for 100 bytes or 10 seconds
    cout << "Read " << numRead << " bytes\n";
    cout.flush();  // Necessary under Windows!  Dunno why yet.
    buf[numRead] = '\0';
    if (numRead > 0) {
      cout << buf << endl;
      const int numWrit = myPort.ar_write( buf, numRead );
      if (numWrit != numRead)
        cout << "error: ";
      cout << "Returned " << numWrit << " bytes." << endl;
    }
  }
  while (*buf != 'q');

  // Not strictly necessary (destructor should handle it)
  if (!myPort.ar_close())
    cerr << "failed to close port.\n";
  return 0;
}
