//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#include "arMath.h"
#include "arDataUtilities.h"
#include "arRS232Port.h"

using namespace std;

int main(int, char **) {
  arRS232Port port;
  if (!port.ar_open( 1, 9600, 8, 1, "none" )) {
    cout << "failed to open port\n";
    return -1;
  }
  port.setReadTimeout( 10 );
  port.ar_write( "_G" );
  char inbuf[4096];
  int numRead = port.ar_read( inbuf, 4000 );
  if (numRead == 0) {
    cerr << "No bytes read from Faro Arm\n";
    return -1;
  }
  inbuf[numRead] = '\0';
  string inString( inbuf );
  cerr << "String read: " << inString << endl;
  string::size_type n1 = inString.find("\n",0);
  string::size_type n2 = inString.find("\n",n1+2);
  if ((n1==string::npos)||(n2==string::npos)) {
    cerr << "Didn't find expected endl's\n";
    return -1;
  }
  inString = inString.substr( n1+2, n2-(n1+2) );
  if (inString.length()==0) {
    cerr << "Bad string returned: " << inString << endl;
    return -1;
  }
  cerr << "The string is: " << inString << endl;
  
  istringstream inStream( inString );
  double nums[14];
  int i;
  for (i=0; i<14; i++) {
    inStream >> nums[i];
    cerr << nums[i] << endl;
  }
  
  const double probeLength = 40.;
  arVector3 probeDims( nums[3], nums[4], probeLength );
  arVector3 customProbeDims( nums[9], nums[10], nums[11] );
  if (++customProbeDims > 1.0e-6) {
    cerr << "Probe #3 does not have dimension 0\n";
    cerr << "Resetting Faro Arm and exitting, please reboot arm and try again.\n";
    port.ar_write( "_M" );
    return 0;
  }
  const int probeNum = int(nums[12]);
  if (probeNum==3)
    cerr << "already using probe 3\n";
  else {
    cerr << "setting probe number to 3\n";
    port.ar_write( "_H3" );
    ar_usleep( 10000 );
  }
  while (true) {
    port.ar_write( "_K" );
    numRead = port.ar_read( inbuf, 90 );
    string dataString( inbuf );
    istringstream dataStream( dataString );
    for (i=0; i<9; i++)
      dataStream >> nums[i];
    const arVector3 position( nums[1], nums[2], nums[3] );
    const arVector3 angles(   nums[4], nums[5], nums[6] );
    const int switches = static_cast<int>( nums[0] );
    const int button1 = 0x1 & switches;
    if (button1 != 0)
      break;
    const int button2 = (0x2 & switches) >> 1;
    if (button2) {
      cerr << "Position:" << position;
      cerr << "; Angles:" << angles;
      cerr << "; Buttons:" << button1 << "," << button2 << endl;
    }
  }
  
  return 0;
}
