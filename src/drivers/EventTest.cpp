//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arEventUtilities.h"
#include "arStructuredData.h"

void PrintState( arInputState& s ) {
  unsigned int i;
  for (i=0; i<s.getNumberButtons(); i++)
    cerr << i << " " << s.getButton( i ) << endl;
  cerr << "-----\n";
  for (i=0; i<s.getNumberAxes(); i++)
    cerr << i << " " << s.getAxis( i ) << endl;
  cerr << "-----\n";
  for (i=0; i<s.getNumberMatrices(); i++)
    cerr << i << endl << s.getMatrix( i ) << endl;
  cerr << "-----\n";
}

int main( int /*argc*/, char** /*argv*/ ) {
  arInputEventQueue q;
  arInputState s;

  arTemplateDictionary dict;
  arDataTemplate t1("Simulated_Input_Event");

  t1.add("signature",AR_INT);
  t1.add("indices",AR_INT);
  t1.add("types",AR_INT);
  t1.add("buttons",AR_INT);
  t1.add("axes",AR_FLOAT);
  t1.add("matrices",AR_FLOAT);
  dict.add(&t1);

  arStructuredData d(&t1);

  cerr << "Setting signature to (1,2,3).\n";
  q.setSignature(1,2,3);
  cerr << "Appending events.\n";
  cerr << " button 0 = 1.\n";
  q.appendEvent( arButtonEvent( 0, 1 ) );
  cerr << " axis 1 = 1.\n";
  q.appendEvent( arAxisEvent( 1, (float)1 ) );
  cerr << " axis 0 = 1.\n";
  q.appendEvent( arAxisEvent( 0, (float)1 ) );
  cerr << " matrix 3 = ar_translationMatrix( 1, 2, 3 ).\n";
  cerr << " Note that this requires an increase in the matrix signature.\n";
  q.appendEvent( arMatrixEvent( 3, ar_translationMatrix(1,2,3) ) );
  
  cerr << "Saving queue to arStructuredData.\n";
  if (!ar_saveEventQueueToStructuredData( &q, &d )) {
    cerr << "failed to save event queue.\n";
    return 1;
  }
  cerr << "Exported arStructuredData:\n";
  d.print(); cout.flush();
  
  cerr << "Clearing & reloading queue from arStructuredData.\n";
  q.clear();
  if (!ar_setEventQueueFromStructuredData( &q, &d )) {
    cerr << "failed to set event queue.\n";
    return 1;
  }
  cerr << "Adding 2nd button event 0 = 1.\n";
  q.appendEvent( arButtonEvent( 0, 1 ) );
  cerr << "Creating 2nd queue:\n";
  arInputEventQueue q2;
  cerr << "Adding matrix event 1 = ar_translationMatrix(3,3,3).\n";
  q2.appendEvent( arMatrixEvent( 1, ar_translationMatrix(3,3,3) ) );
  cerr << "Appending second queue to first.\n";
  q.appendQueue( q2 );
  cerr << "Attempting to reduce signature to ( 1, 1, 1 ) (should fail).\n";
  q.setSignature(1,1,1);
  cerr << "Re-saving queue to arStructuredData.\n";
  if (!ar_saveEventQueueToStructuredData( &q, &d )) {
    cerr << "failed to save event queue.\n";
    return 1;
  } 
  cerr << "Exported arStructuredData:\n";
  d.print(); cout.flush();
  
  cerr << "Start messing with arInputState and device maps.\n";
  cerr << "Adding input device, signature (0,0,1).\n";
  s.addInputDevice( 0, 0, 1 );
  cerr << "Adding input device, signature (1,2,3).\n";
  s.addInputDevice( 1, 2, 3 );
  cerr << "Initializing arInputState from arStructuredData.\n";
  if (!ar_setInputStateFromStructuredData( &s, &d )) {
    cerr << "failed to set input state.\n";
    return 1;
  }
  cerr << "Adding matrix event 0 = ar_rotationMatrix('y',0.5).\n";
  s.setMatrix( 0, ar_rotationMatrix('y',0.5) );
  cerr << "Saving arInputState to arStructuredData.\n";
  if (!ar_saveInputStateToStructuredData( &s, &d )) {
    cerr << "failed to save input state.\n";
    return 1;
  }  
  cerr << "Exported arStructuredData:\n";
  d.print(); cout.flush();
  
  PrintState( s );
  cerr << "Now change first device's signature to (3,3,2).\n";
  cerr << "   (this should cause an upward shift in each event category\n"
       << "   for events with indices greater than (2,2,1).\n";
  s.remapInputDevice(0,3,3,2);
  PrintState( s );
  
  cerr << "Now set device 0 signature to (2,2,1) (should shift down).\n";  
  s.remapInputDevice(0,2,2,1);
  PrintState( s );
  cerr << "Set device 1 signature to (3,1,3).\n";
  s.remapInputDevice(1,3,1,3);
  PrintState( s );

  cerr << "Now we will arbitarily throw away matrices by decreasing the signature.\n";
  s.setSignature(2,2,2);
  PrintState( s );
  
  cerr << "Device event offsets:\n";
  unsigned int o0, o1;
  if (s.getButtonOffset(0,o0)&&s.getButtonOffset(1,o1))
    cerr << "Buttons: " << o0 << " " << o1 << endl;
  if (s.getAxisOffset(0,o0)&&s.getAxisOffset(1,o1))
    cerr << "Axes: " << o0 << " " << o1 << endl;
  if (s.getMatrixOffset(0,o0)&&s.getMatrixOffset(1,o1))
    cerr << "Matrices: " << o0 << " " << o1 << endl;
  return 0;
}
  
