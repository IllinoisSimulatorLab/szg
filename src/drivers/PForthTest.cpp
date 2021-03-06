//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

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

void PrintState( arInputState& s ) {
  unsigned int i;
  for (i=0; i<s.getNumberButtons(); i++)
    cout << i << " " << s.getButton( i ) << "\n";
  cout << "-----\n";
  for (i=0; i<s.getNumberAxes(); i++)
    cout << i << " " << s.getAxis( i ) << "\n";
  cout << "-----\n";
  for (i=0; i<s.getNumberMatrices(); i++)
    cout << i << "\n" << s.getMatrix( i ) << "\n";
  cout << "-----\n";
}

int main(int argc, char** argv) {
  arSZGClient client;
  client.init(argc, argv);
  if (!client) {
    cerr << "PForthTest error: SZGCLient failed to connect.\n";
    return 1;
  }
  if (argc < 2) {
    cerr << "usage: PForthTest program_name\n";
    return 1;
  }
  arPForthFilter filter;
  // We will load the PForth filter program from a "global" attribute.
  ar_PForthSetSZGClient(&client);
  string programText = client.getGlobalAttribute(argv[1]);
  if (programText == "NULL") {
    cerr << "PForthTest error: no global attribute '" << argv[1] << "'.\n";
    return 1;
  }
  if (!filter.loadProgram(programText)) {
    cerr << "PForthTest error: failed to load filter program.\n";
    return 1;
  }

  arTemplateDictionary dict;
  arDataTemplate t1("Simulated_Input_Event");

  t1.add("signature", AR_INT);
  t1.add("indices", AR_INT);
  t1.add("types", AR_INT);
  t1.add("buttons", AR_INT);
  t1.add("axes", AR_FLOAT);
  t1.add("matrices", AR_FLOAT);
  dict.add(&t1);

  arStructuredData data1(&t1);

  arInputEventQueue q;
  cout << "Setting signature to (1,2,4).\n";
  q.setSignature(1, 2, 3);
  cout << "Appending events.\n";
  cout << " button 0 = 1.\n";
  q.appendEvent( arButtonEvent( 0, 1 ) );
  cout << " axis 1 = 1.\n";
  q.appendEvent( arAxisEvent( 1, (float)1 ) );
  cout << " axis 0 = 1.\n";
  q.appendEvent( arAxisEvent( 0, (float)1 ) );
  cout << " matrix 0 = ar_translationMatrix( 1, 2, 3 ).\n";
  q.appendEvent( arMatrixEvent( 0, ar_translationMatrix(1, 2, 3) ) );
  cout << " matrix 1 = ar_rotationMatrix( 'y', 0.5 ).\n";
  q.appendEvent( arMatrixEvent( 1, ar_rotationMatrix('y', 0.5) ) );
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
