//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Note regarding Faro probe dimensions:
// Locate the FaroArm segment most distal from the base with the two buttons
// on it.  Call this part A1.  It is rigidly connected to an orthogonal segment.
// Call this A2. A2 is connected to a collinear short segment of the next most proximal
// (i.e. towards the base) arm segment, call it B1.  A2 and B1 share an axis and
// rotate around it with respect to one another.  B1 is rigidly connected to an
// orthogonal segment; call this B2.  B2 in turn is connected to and collinear with
// the most distal long segment of the arm.
//
// The origin of the probe coordinate system is where the rotation axis of B2 meets
// the common axis of B1 and A2. The Z coordinate is along the primary axis of A1, starting
// from where the A1 axis meets the A2 rotation axis; this corresponds most directly to
// the length of the probe.  The Y coordinate is along the A2/B1 axis, with negative
// values in the direction of A2. The X coordinate is orthogonal to these two, haven't
// figured out the signs yet.
//
// Further Note: the foregoing is based on the stored dimensions for the two probes supplied by
// Faro with an application of common sense. To wit, the dimensions for both are on the order of
// (.25,-2,5.1).  The length of the probe + the probe holder (part A1) is about 5 inches.  The length
// of part A2 + part B1 is 2 inches.  Seems obvious, right?  However, experience teaches us that
// "Faro" and "common sense" never belong in the same sentence. We are using a 57-inch
// probe (that includes the length of the probe holder).  Based on the foregoing,
// the expected probe dimensions would be roughly (.25,-2,57).  The values that _work_
// (i.e. give us roughly constant tip position independent of orientation when we keep
// the tip fixed & move the rest of the arm around) are in fact (4,-2,57).

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#include "arDataUtilities.h"
#include "arFaroDriver.h"

using namespace std;

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arFaroDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

const float INCHES_TO_FEET(1./12.);
const float DEGREES_TO_RADIANS(M_PI/180.);

void ar_FaroDriverEventTask(void* theDriver){
  arFaroDriver* faroDriver = (arFaroDriver*) theDriver;
  faroDriver->_stopped = false;
  faroDriver->_eventThreadRunning = true;
  while (faroDriver->_eventThreadRunning && !faroDriver->_stopped) {
    faroDriver->_eventThreadRunning = faroDriver->_getSendData(); 
  }
  faroDriver->_eventThreadRunning = false;
}

arFaroDriver::arFaroDriver() :
  _inited(false),
  _eventThreadRunning(false),
  _stopped(false)
{}

arFaroDriver::~arFaroDriver() {
  _port.ar_close();  // OK even if not open.
}

bool arFaroDriver::init(arSZGClient& SZGClient) {
  float floatBuf[3];
  if (!SZGClient.getAttributeFloats("SZG_FARO","probe_dimensions",floatBuf,3)) {
    cerr << "arFaroDriver error: you _must_ set SZG_FARO/probe_dimensions (in inches).\n";
    return false;
  }
  _probeDimensions = arVector3( floatBuf );
  cerr << "arFaroDriver remark: probe dimensions = " << _probeDimensions << " inches.\n";
  if ((_probeDimensions[2] < 0.)||(_probeDimensions[2] > 120)) { 
    cerr << "arFaroDriver::init() warning: You have set a highly improbable probe length\n"
          << "   (SZG_FARO/probe_dimensions[2] == " << _probeDimensions[2] << " in.)" << endl;
  }
  _probeDimensions /= 12.; // convert to feet

  unsigned int portNum = static_cast<unsigned int>(
        SZGClient.getAttributeInt("SZG_FARO", "com_port"));
  if (!_port.ar_open( portNum, 9600, 8, 1, "none" )) {
    cerr << "arFaroDriver::init() error: failed to open serial port #" << portNum << endl;
    return false;
  }
  _port.setReadTimeout( 10 );  // 1 second
  _port.ar_write( "_G" );     // request FaroArm configuration data.
  int numRead = _port.ar_read( _inbuf, 4000 );
  if (numRead < 1) { // How long is this supposed to be?
    cerr << "arFaroDriver::init() error: failed to get FaroArm status (numRead = " << numRead << endl;
    return false;
  }
  _inbuf[numRead] = '\0';
  string inString( _inbuf );
  string::size_type n1 = inString.find("\n",0);
  string::size_type n2 = inString.find("\n",n1+2);
  if ((n1==string::npos)||(n2==string::npos)) {
    cerr << "arFaroDriver::init() error: FaroArm returned malformed status string.\n";
    return false;
  }
  inString = inString.substr( n1+2, n2-(n1+2) );
  if (inString.length()==0) {
    cerr << "arFaroDriver::init() error: Status string contains wrong number of elements.\n";
    return false;
  }

  istringstream inStream( inString );
  float nums[14];
  for (int i=0; i<14; i++)
    inStream >> nums[i];

  int probeNum = int(nums[12]); // what if nums[12] isn't an exact integer?
  if (probeNum==1)
    cerr << "arFaroDriver::init() remark: Already using probe 1\n";
  else {
    cerr << "arFaroDriver::init() remark: Setting probe number to 1\n";
    _port.ar_write( "_H1" );
    ar_usleep( 10000 );
  }

  _defaultProbeDimensions = arVector3( nums );
  cerr << "arFaroDriver remark: probe 1 dimensions = " << _defaultProbeDimensions << " inches." << endl;
  _defaultProbeDimensions/= 12.;
  _probeDimDiffs = _probeDimensions - _defaultProbeDimensions;
  cerr << "arFaroDriver remark: probe diff. = " << _probeDimDiffs << " feet.\n";
  _inited = true;
  _setDeviceElements( 1, 0, 1 );
  return true;
}

bool arFaroDriver::start(){
  if (!_inited) {
    cerr << "arFaroDriver::start() error: Not inited yet.\n";
    return false;
  }
//  int numWrit = _port.ar_write( "_T" );
//  if (numWrit != 2) {
//    cerr << "arFaroDriver error: write() failed in _getSendData().\n";
//    return false;
//  }
  const bool ok = _eventThread.beginThread(ar_FaroDriverEventTask,this);
  if (ok)
    cerr << "arFaroDriver started.\n";
  return ok;
}

bool arFaroDriver::stop(){
  _stopped = true;
  while (_eventThreadRunning) {
    ar_usleep(10000);
//    cerr << "arFaroDriver remark: waiting for event thread to exit.\n";
  }
  cerr << "arFaroDriver stopped.\n";
  return true;
}

bool arFaroDriver::_getSendData() {
  int numWrit = _port.ar_write( "_K" );
  if (numWrit != 2) {
    cerr << "arFaroDriver error: write() failed in _getSendData().\n";
    return false;
  }
  int numRead = _port.ar_read( _inbuf, 90 );
  if (numRead!=90) {
    cerr << "arFaroDriver() error: Only " << numRead << " bytes read in _getSendData()\n";
    return false;
  } else {
    string dataString( _inbuf );
    istringstream dataStream( dataString );
    float nums[9];
    for (int i=0; i<9; i++)
      dataStream >> nums[i];
    arVector3 initialPosition( nums+1 );
    arVector3 angles( nums+4 );
    initialPosition = initialPosition * INCHES_TO_FEET;
    angles = angles * DEGREES_TO_RADIANS;
//    cerr << nums[0] << endl;
    int switches = static_cast<int>( nums[0] );

    // Had to make this change because one of the buttons on our arm died
    // (the other is functionally inverted, too, i.e. it reports "on" when
    // off & vice versa. If you have a working FaroArm, also change the
    // call to _setDeviceElements() appropriately. 
    int button0 = !(0x1 & switches);
//    int button0 = 0x1 & switches;
//    int button1 = (0x2 & switches) >> 1;

    // NOTE: if the Faro manual were to be believed, the angles used here should
    // all be negated.  But it isn't. So they aren't.
    // Note also that this is the matrix you need to multiply the probe offset
    // by to get the tip position, but the probe orientation comes out in a
    // different coordinate system from that of its position...
    arMatrix4 rotMatrix( ar_rotationMatrix('z',angles[0]) 
                         * ar_rotationMatrix('x',angles[1])
                          * ar_rotationMatrix('z',angles[2]) );
//    arMatrix4 rotMatrix( ar_rotationMatrix('z',-angles[2]) 
//                         * ar_rotationMatrix('x',-angles[1])
//                          * ar_rotationMatrix('z',-angles[0]) );
    arVector3 position = initialPosition + rotMatrix * _probeDimDiffs;
    
//    float iDir(sin(angles[0])*sin(angles[1]));
//    float jDir(-sin(angles[1])*cos(angles[0]));
//    float kDir(cos(angles[0]));

//    cerr << "Direction cosines: " << iDir << ", " << jDir << ", " << kDir << endl;
    // ...so we'll tweak the orientation here after getting the position...
//    rotMatrix = rotMatrix*ar_rotationMatrix('z',ar_convertToRad(-90));
//    rotMatrix = ar_rotationMatrix('y',ar_convertToRad(180))*rotMatrix;
//    rotMatrix = ar_rotationMatrix('z',-angles[0]) 
//                         * ar_rotationMatrix('x',-angles[1])
//                          * ar_rotationMatrix('z',-angles[2]);
    arMatrix4 faroMatrix( ar_translationMatrix(position)*rotMatrix );

    queueMatrix( 0, faroMatrix );
    queueButton( 0, button0 );
//    queueButton( 1, button1 );
    sendQueue();
  }  
  return true;
}
