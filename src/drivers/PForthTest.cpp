//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arMath.h"
#include "arPForthFilter.h"
#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arEventUtilities.h"
#include <string>
#include <iostream>

void PrintState( arInputState& s ) {
  unsigned int i;
  for (i=0; i<s.getNumberButtons(); i++)
    cout << i << " " << s.getButton( i ) << endl;
  cout << "-----\n";
  for (i=0; i<s.getNumberAxes(); i++)
    cout << i << " " << s.getAxis( i ) << endl;
  cout << "-----\n";
  for (i=0; i<s.getNumberMatrices(); i++)
    cout << i << endl << s.getMatrix( i ) << endl;
  cout << "-----\n";
}


int main(int argc, char** argv) {
  arSZGClient client;
  client.init(argc, argv);
  if (!client) {
    cerr << "PForthTest error: SZGCLient failed to connect.\n";
    return 1;
  }
  arPForthFilter filter;
  if (!filter.configure( &client )) {
    cerr << "PForthTest error: failed to configure filter.\n";
    return 1;
  }

  arTemplateDictionary dict;
  arDataTemplate t1("Simulated_Input_Event");

  t1.add("signature",AR_INT);
  t1.add("indices",AR_INT);
  t1.add("types",AR_INT);
  t1.add("buttons",AR_INT);
  t1.add("axes",AR_FLOAT);
  t1.add("matrices",AR_FLOAT);
  dict.add(&t1);

  arStructuredData data1(&t1);
  
  arInputEventQueue q;
  cout << "Setting signature to (1,2,4).\n";
  q.setSignature(1,2,3);
  cout << "Appending events.\n";
  cout << " button 0 = 1.\n";
  q.appendEvent( arButtonEvent( 0, 1 ) );
  cout << " axis 1 = 1.\n";
  q.appendEvent( arAxisEvent( 1, (float)1 ) );
  cout << " axis 0 = 1.\n";
  q.appendEvent( arAxisEvent( 0, (float)1 ) );
  cout << " matrix 0 = ar_translationMatrix( 1, 2, 3 ).\n";
  q.appendEvent( arMatrixEvent( 0, ar_translationMatrix(1,2,3) ) );
  cout << " matrix 1 = ar_rotationMatrix( 'y', 0.5 ).\n";
  q.appendEvent( arMatrixEvent( 1, ar_rotationMatrix('y',0.5) ) );
  cout << "Saving queue to arStructuredData.\n";
  if (!ar_saveEventQueueToStructuredData( &q, &data1 )) {
    cerr << "PForthTest error: failed to save event queue.\n";
    return 1;
  }
  cout << "Exported arStructuredData:\n";
  data1.print();
  ar_usleep( 50000 );
  
  arInputState s;
  if (!filter.filter( &q, &s )) {
    cerr << "PForthTest error: filtering failed.\n";
    return 1;
  }
  
  cout << "PForthTest remark: saving queue to arStructuredData.\n";
  if (!ar_saveEventQueueToStructuredData( &q, &data1 )) {
    cout << "failed to save event queue.\n";
    return 1;
  }
  cout << "PForthTest remark: exported arStructuredData:\n";
  data1.print();
  ar_usleep( 50000 );
  
  cout << "PForthTest remark: saving state to arStructuredData.\n";
  if (!ar_saveInputStateToStructuredData( &s, &data1 )) {
    cout << "failed to save event queue.\n";
    return 1;
  }
  cout << "PForthTest remark: exported arStructuredData:\n";
  data1.print();
  ar_usleep( 50000 );
  
  return 0;  
}
