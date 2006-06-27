//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
// The arIntersenseDriver was written by 
// Drew Dolgert ajd27@cornell.edu
//********************************************************

#include "arPrecompiled.h"
#include "arIntersenseDriver.h"

extern "C"{
  SZG_CALL void* factory()
    { return new arIntersenseDriver(); } 
  SZG_CALL void baseType(char* buffer, int size)
    { ar_stringToBuffer("arInputSource", buffer, size); }
}

const float METER_TO_FOOT = 3.280839895;
const Bool USE_VERBOSE = TRUE;

struct StationIDEquals : public unary_function<IsenseStation, bool> {
  StationIDEquals( const unsigned int id ): _id(id) {}
  bool operator()(const IsenseStation& s) { return s.getID()==_id; }
  unsigned int _id;
};


IsenseTracker::IsenseTracker() : 
  _id(1000),
  _imOK(true),
  _handle(0) {
  _stations.insert( _stations.begin(), ISD_MAX_STATIONS, IsenseStation() );
}

IsenseTracker::~IsenseTracker() {
  ar_close();
  _stations.clear();
}

/*!
    \param port The port can refer to an RS232 port or a USB port,
        numbered according to some Intersense-internal scheme not
        documented.
        Sending 0 indicates the first one found should be opened.
    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::ar_open( DWORD port = 0 ) {
  ar_close();
  _port = port;
  _handle = ISD_OpenTracker( (Hwnd) NULL, port, FALSE, USE_VERBOSE );
  bool success = _isValidHandle( _handle );
  if (success) {
    std::vector< IsenseStation >::iterator statIter;
    for (statIter = _stations.begin(); statIter != _stations.end(); ++statIter) {
      statIter->setTrackerHandle( _handle );
    }
  } else {   
    cerr << "IntersenseDriver::failed to open tracker on port "
         << port << ".  "
         << "Set verbosity to true to see reason for failure." << endl;
  }
  return success;
}


/*! If one tracker stops working, it should be possible to open
    it again.  If there were several trackers, opening just one
    would require that we specify its port, and the port is a
    magical number created by Intersense to enumerate over both
    RS232 and USB ports, so it is not known until after you can
    open the first time.
 
    \return False indicates failure of the driver.  This method
        will print problem to stderr.
   */
bool IsenseTracker::ar_reopen() {
  _imOK = ar_open( _port );
  return _imOK;
}


/*! \param handle The handle should have been returned from
        a call to ISD_OpenTracker or ISD_OpenAllTrackers.
    */
void IsenseTracker::setHandle( ISD_TRACKER_HANDLE handle ) {
  ar_close();
  _handle = handle;
  std::vector< IsenseStation >::iterator statIter;
  for (statIter = _stations.begin(); statIter != _stations.end(); ++statIter) {
    statIter->setTrackerHandle( _handle );
  }
}


/*! You must call Open() or SetHandle() before calling Init().

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::init() {
  if ( !_getTrackerConfig() ) {
    _imOK = false;
    return false;
  }
  _loadAllStationInfo();
  if ( !_resetAllAlignmentReferenceFrames() ) {
    _imOK = false;
    return false;
  }
  // This isn't appropriate for VR?
  //if ( !_enablePossibleCameraTracking() ) return false;
  return true;
}


bool IsenseTracker::configure( arSZGClient& client ) {
  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    if (!iter->configure( client, getID() )) {
      return false;
    }
  }
  return true;
}


/*! This method is fine to call on an already closed or
    never opened tracker.

    \return False on failure to close.  Nothing to be
        done if it fails unless you can unload the Dll.
    */
bool IsenseTracker::ar_close() {
  Bool success = TRUE;
  if ( _isValidHandle( _handle ) ) {
    success = ISD_CloseTracker( _handle );
  }
  if ( FALSE == success ) {
    return false;
  }
  _handle = 0;
  return true;
}


/*! This object uses both configuration information and
    the user's settings of the number of buttons and such in order
    to create an internal array of how to remap buttons, analogs
    and aux inputs to buttons and axes.

    All parameters are incremented upon return!!!! (matrixIndex by 1,
    the others by the amount set in setStationInputCounts()).

    \param matrixIndex Index of the tracker in the arInputSource array.
    \param buttonIndex Index of first button in arInputSource array.
    \param axisIndex Index of first axis in arInputSource array.
    \return Index of next available sensor, button, and axis.
    */
void IsenseTracker::setStationIndices( unsigned int& matrixIndex,
                                       unsigned int& buttonIndex,
                                       unsigned int& axisIndex ) {
  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    iter->setIndices( matrixIndex, buttonIndex, axisIndex );
  }
}


/*! You must call init() before calling getData().
    The data is translated so that the Intersense position and
    orientation becomes a szg sensor, buttons become szg buttons,
    analogs become szg axes normalized (-1,1), and aux inputs 
    become szg axes normalized (0,1).  Not having seen an aux
    input or any documentation about it, I don't know whether
    this normalization is correct.

    \return False indicates we could not retrieve data.
    */
bool IsenseTracker::getData( arInputSource* inputSource ) {
  if ( FALSE == ISD_GetData( _handle, &_data ) ) {
    cerr << "IsenseTracker error: Failed to get data from tracker "
         << _handle << endl;
    _imOK = false;
    return false;
  }

  // We retrieve camera data, but we don't know what to
  // do with "ApertureEncoder, FocusEncoder, and ZoomEncoder."
//  if ( _bSupportsCameraData ) {
//    if (FALSE == ISD_GetCameraData( _handle, &_cameraData ) ) {
//      std::cerr << "IntersenseDriver::got no camera data from tracker "
//        << _handle << std::endl;
//      _imOK = false;
//      return false;
//    }
//  }

  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    iter->queueData( _data, _bSupportsPositionMeasurement, inputSource );
  }
  // Send results as an event queue.
  inputSource->sendQueue();
  return true;
}


/*! We load the configuration for the tracker in order
    to find out what model we have and its capabilities.
    It is here that we find out what the actual port is.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::_getTrackerConfig() {
  Bool success = ISD_GetTrackerConfig( _handle, &_trackerInfo, USE_VERBOSE );
  if ( FALSE == success ) {
    std::cerr << "Intersense::got no tracker information on " <<
      "device port " << _port << std::endl;
    return false;
  }

  // Set the local variables that derive from tracker info.
  _port = _trackerInfo.Port;
  DWORD model = _trackerInfo.TrackerModel;
  if ( (ISD_IS600 == model) || (ISD_IS900 == model) || (ISD_IS1200 == model) ) {
    _bSupportsPositionMeasurement = true;
  } else {
    _bSupportsPositionMeasurement = false;
  }

  // Tell the world our configuration
  std::cerr << "IntersenseDriver::Started tracker on port " << _port << "." << endl;
  return true;
}


/*! The station info struct holds the State variable,
    which tells us whether a device is connected.  It
    also holds the data to be read.  This queries
    the Intersense tracker in order to get its information,
    and the query can take a few seconds, so we do it once.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
void IsenseTracker::_loadAllStationInfo() {
  unsigned int stationID(1);
  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    iter->loadStationInfo( stationID++ );
  }
  // Hack, i.e. I'm not sure this is the right thing to do. Driver worked in ISL,
  // not in Frances' lab (DeviceServer crashes when resetting coord axes for
  // invalid stations). So here I prune the list of invalid stations.
  bool done(false);
  while (!done) {
    done = true;
    for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
      if (!iter->getStatus()) {
        _stations.erase(iter);
	done = false;
	break;
      }
    }
  }
}

// old version
//void IsenseTracker::_loadAllStationInfo() {
//  unsigned int stationID(1);
//  std::vector< IsenseStation >::iterator iter;
//  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
//    iter->loadStationInfo( stationID++ );
//  }
//}


/*! We ask only that this work for at least one of the stations
    because it is reasonable that it might not work for a station
    which is not connected.  We could use the A command to set
    the internal reference frame of the tracker, and that would 
    likely result in a faster driver, but what we have in our
    GetData works for now.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::_resetAllAlignmentReferenceFrames() {
  bool success = false;
  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    success |= iter->resetAlignmentReferenceFrame();
  }
  return success;
}


/*! The Intersense station will not, by default, have camera
    tracking enabled, so this method turns it on.  This method has
    not been tested with an actual camera device.

    \return False indicates the driver will not do camera tracking,
        which may not be a failure.
    */
void IsenseTracker::_enablePossibleCameraTracking() {
  // The sample code has the line below, but CAMERA_TRACKER
  // must just be a define.  We'll turn it on by default.
  // ( CAMERA_TRACKER && _trackerInfo.TrackerModel == ISD_IS900)
  _bSupportsCameraData = false;
  if (!( (ISD_PRECISION_SERIES == _trackerInfo.TrackerType ) &&
                        ( _trackerInfo.TrackerModel == ISD_IS900) )) {
    return;
  }
  std::vector< IsenseStation >::iterator iter;
  for (iter = _stations.begin(); iter != _stations.end(); ++iter) {
    if (iter->enableCameraTracking()) {
      _bSupportsCameraData = true;
    }
  }
}



/*! Prints to stderr all the setup information.
    \param trackerIndex Index of this tracker in arInputSource
        array.
    \return true always.
    */
ostream& operator<<(ostream& s, const IsenseTracker& tc) {
  s << endl;
  s << "Intersense Tracker on port " << tc.getPort() << endl;
  std::vector< IsenseStation >::iterator iter;
  IsenseTracker& t = const_cast<IsenseTracker&>(tc);
  std::vector< IsenseStation >& stations = t.getStations();
  for (iter = stations.begin(); iter != stations.end(); ++iter) {
    s << *iter;
  }
  s << endl;
  return s;
}


IsenseStation::IsenseStation() :
  _matrixIndex(1000),
  _numButtons(0),
  _numAnalogInputs(0),
  _numAuxInputs(0),
  _firstButtonIndex(0),
  _firstAnalogIndex(0),
  _firstAuxIndex(0) {
}

IsenseStation::~IsenseStation() {
}

bool IsenseStation::getStatus() const {
  return _stationConfig.State != FALSE;
}


unsigned int IsenseStation::getID() const {
  return static_cast<unsigned int>(_stationConfig.ID);
}


bool IsenseStation::configure( arSZGClient& client, unsigned int trackerIndex ) {
  const unsigned int charCnt = 200;
  char chStation[ charCnt ];

  int sig[3] = {0,0,0};
  sprintf( chStation, "station%d_%d", trackerIndex, getID() );
  if (client.getAttributeInts( "SZG_INTERSENSE", chStation, sig, 3 ) ) {
    _setInputCounts( sig[0], sig[1], sig[2] );
    cerr << "IsenseStation remark: Set station " << getID() << " to "
         << sig[0] << ":" << sig[1] << ":" << sig[2] << endl;
  }

  // Get value of e.g. SZG_INTERSENSE/compass0_1 (should be 0=off, 1=partial, or 2=full).
  sprintf( chStation, "compass%d_%d", trackerIndex, getID() );
  int useCompass;
  if (client.getAttributeInts( "SZG_INTERSENSE", chStation, &useCompass, 1 ) ) {
    if (!_setUseCompass( (Bool)useCompass )) {
      cerr << "IsenseStation error: Failed to set compass usage for station " 
           << getID() << " to " << (Bool)useCompass << endl;
      return false;
    }
    cerr << "IsenseStation remark: Set compass usage for station " 
         << getID() << " to " << (Bool)useCompass << endl;
  }
  return true;
}



/*! The station info struct holds the State variable,
    which tells us whether a device is connected.  It
    also holds the data to be read.  This queries
    the Intersense tracker in order to get its information,
    and the query can take a few seconds, so we do it once.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
void IsenseStation::loadStationInfo( unsigned int stationID ) {
  _stationConfig.State = ISD_GetStationConfig( _trackerHandle, 
                                               &_stationConfig,
                                               static_cast<WORD>(stationID),
                                               USE_VERBOSE ) != FALSE;
}


bool IsenseStation::resetAlignmentReferenceFrame() {
  char chReset[10];
  sprintf( chReset, "R%d\n", getID() );
  Bool bSent = ISD_SendScript( _trackerHandle, chReset );
  return bSent == TRUE;
}


/*! The Intersense station will not, by default, have camera
    tracking enabled, so this method turns it on.  This method has
    not been tested with an actual camera device.

    \return False indicates the driver will not do camera tracking,
        which may not be a failure.
    */
bool IsenseStation::enableCameraTracking() {
  // The sample code has the line below, but CAMERA_TRACKER
  // must just be a define.  We'll turn it on by default.
  if (!getStatus()) {
    return false;
  }
  if (_stationConfig.GetCameraData == TRUE) {
    return true;
  }
  _stationConfig.GetCameraData = TRUE;
  bool stat = ISD_SetStationConfig( _trackerHandle,
                                    &_stationConfig,
                                    static_cast<WORD>(getID()),
                                    USE_VERBOSE ) != FALSE;
  if (!stat) {
    cerr << "IntersenseDriver::failed to set configuration "
         << "for station " << getID() << endl;
  }
  return stat;
}


/*! \param buttonCnt Number of buttons the station should have.
    \param analogCnt Number of analogs (axes) the station should have.
    \param auxCnt Number of aux inputs the station should have.  No 
        indication what an aux input might be, but it returns type BYTE.
    */
void IsenseStation::_setInputCounts( unsigned int buttonCnt,
                                    unsigned int analogCnt,
                                    unsigned int auxCnt ) {
  _numButtons = buttonCnt;
  _numAnalogInputs = analogCnt;
  _numAuxInputs = auxCnt;
}



bool IsenseStation::_setUseCompass( unsigned int compassVal ) {
  _stationConfig.Compass = (Bool)compassVal;
  bool stat = (ISD_SetStationConfig( _trackerHandle,
                                     &_stationConfig,
                                     static_cast<WORD>(getID()),
                                     USE_VERBOSE )!= FALSE);
  if (stat) {
    cerr << "IsenseStation remark: Set useCompass "
         << "for station " << getID() << endl;
  } else {
    cerr << "IsenseStation error: Failed to set useCompass "
         << "for station " << getID() << endl;
  }
  return stat;
}



/*! This object uses both configuration information and
    the user's settings of the number of buttons and such in order
    to create an internal array of how to remap buttons, analogs
    and aux inputs to buttons and axes.

    All parameters are incremented upon return.

    \param matrixIndex Index of the tracker in the arInputSource array.
    \param buttonIndex Index of first button in arInputSource array.
    \param axisIndex Index of first axis in arInputSource array.
    \return Index of next available sensor, button, and axis.
    */
void IsenseStation::setIndices( unsigned int& matrixIndex,
                                unsigned int& buttonIndex,
                                unsigned int& axisIndex ) {
  if (!getStatus()) {
    return;
  }
  _matrixIndex = matrixIndex++;

  _firstButtonIndex = buttonIndex;
  buttonIndex += _numButtons;

  _firstAnalogIndex = axisIndex;
  axisIndex += _numAnalogInputs;

  _firstAuxIndex = axisIndex;
  axisIndex += _numAuxInputs;
}



/*! The data is translated so that the Intersense position and
    orientation becomes a szg sensor, buttons become szg buttons,
    analogs become szg axes normalized (-1,1), and aux inputs 
    become szg axes normalized (0,1).  Not having seen an aux
    input or any documentation about it, I don't know whether
    this normalization is correct.

    \return False indicates we could not retrieve data.
    */
void IsenseStation::queueData( ISD_TRACKER_DATA_TYPE& data,
                               bool usePositionMeasurement,
                               arInputSource* inputSource ) {

  if (!getStatus()) {
    return;
  }

  ISD_STATION_STATE_TYPE& myData = data.Station[_matrixIndex];

  arMatrix4 theTransMatrix;
  if ( usePositionMeasurement ) {
      // Rotate the position separately.
    theTransMatrix =
      ar_translationMatrix( arVector3(
                             METER_TO_FOOT*myData.Position[0],
                             METER_TO_FOOT*myData.Position[1],
                             METER_TO_FOOT*myData.Position[2]
                             ) );
  } // else just use identity translation matrix.

  arMatrix4 theRotMatrix;
  if ( ISD_QUATERNION == _stationConfig.AngleFormat ) {
    theRotMatrix =
      ar_transrotMatrix(arVector3(0, 0, 0),
      arQuaternion( myData.Orientation[0],
                    myData.Orientation[1],
                    myData.Orientation[2],
                    myData.Orientation[3]));
  } else { // Euler matrices
    // The Intersense Euler angles are presented as yaw, pitch, roll,
    // which are a rotation about z for yaw, y for pitch, and x for roll.
    // Documentation shows right-handed coordinate system and positive
    // angles.
    // The rotation is in the old coordinate system.
    theRotMatrix =
      ar_rotationMatrix('z', ar_convertToRad( myData.Orientation[0]))*
      ar_rotationMatrix('y', ar_convertToRad( myData.Orientation[1]))*
      ar_rotationMatrix('x', ar_convertToRad( myData.Orientation[2]));
  }
  inputSource->queueMatrix( _matrixIndex, theTransMatrix*theRotMatrix );

  const int analogCenter = 127;
  // Normalize axis from (0,255) to (-1,1).
  const float axisMultiple = 1.0f/128.0f;
  if ( TRUE == _stationConfig.GetInputs ) {
    // Get buttons
    for ( unsigned int buttonIndex=0; buttonIndex < _numButtons; ++buttonIndex ) {
      inputSource->queueButton( _firstButtonIndex+buttonIndex, 
                                myData.ButtonState[buttonIndex] );
    }
    // Get analogs
    for ( unsigned int analogIndex=0; analogIndex < _numAnalogInputs; ++analogIndex ) {
       inputSource->queueAxis( _firstAnalogIndex+analogIndex,
                 axisMultiple*( myData.AnalogData[ analogIndex ]-analogCenter ) );
    }
  }

  // Get aux inputs, whatever they are.
  const BYTE auxCenter = 0;
  // Normalize BYTE from (0,255) to (0,1).
  const float auxMultiple = 1.0f/255.0f;
  if ( TRUE == _stationConfig.GetAuxInputs ) {
    for ( unsigned int auxIndex = 0; auxIndex < _numAuxInputs; auxIndex++ ) {
      inputSource->queueAxis( _firstAuxIndex+auxIndex,
                 auxMultiple*( myData.AuxInputs[ auxIndex ]-auxCenter ) );
    }
  }
}



ostream& operator<<(ostream& s, const IsenseStation& t) {
    if (!t.getStatus()) {
      return s;
    }
    s << endl;
    s << "\tStation ID: " << t.getID() << endl;
    s << "\tMatrix index: " << t.getMatrixIndex() << endl;
    s << "\tCompass usage: ";
    switch (t.getCompass()) {
      case 0:
        s << "None.\n";
        break;
      case 1:
        s << "Partial.\n";
        break;
      case 2:
        s << "Full.\n";
        break;
      default:
        s << "??Unrecognized value.\n";
    }
    s << "\tOther inputs:\n";
    s << "\t\tbutton\tanalog\taux\n";
    s << "\tFirst:\t" << t.getFirstButtonIndex() << "\t" << t.getFirstAnalogIndex()
      << "\t" << t.getFirstAuxIndex() << endl;
    s << "\tTotal:\t" << t.getNumButtons() << "\t" << t.getNumAnalogInputs() << "\t"
      << t.getNumAuxInputs() << endl;
  return s;
}



/***********************************************************/
/*                     ar_intersenseDriverEventTask        */
/***********************************************************/

/*! \brief Thread function to poll the Intersense driver

    This function asks the driver to send events through
    its arInputSource.  This method fails to recover
    from reboots of the tracker because the Intersense
    driver does not report an error when its tracker is
    disconnected.
    */
void ar_intersenseDriverEventTask(void* intersenseDriver){
  arIntersenseDriver* isense =
    (arIntersenseDriver*) intersenseDriver;

  for (;;) {
//  I'm not sure if this is necessary...
//  Turns out it is. Doesn't seem to matter for an InertiaCube, but down
//  at the Duke DiVE, where they have an IS-9000, master/slave apps
//  would slow to a crawl after the tracking started up unless we put
//  a sleep in here.
    isense->_waitForData();
    bool bSuccess = isense->_getData();
    if ( false == bSuccess ) {
      // Never get here b/c Intersense driver always 
      // returns a cheery success.
      while ( false == isense->_reacquire() )
        ar_usleep( 15*1000 );
    }
  }
}


/***********************************************************/
/*                     ar_intersenseDriver                 */
/***********************************************************/

arIntersenseDriver::arIntersenseDriver() {
  // does nothing yet
}


arIntersenseDriver::~arIntersenseDriver() {
  _trackers.clear();
}


/*! Below is a sample set of parameters for this driver.  Replace
    trackmach with the name of the machine where the Intersense
    is attached.
    \verbatim
    trackmach SZG_INTERSENSE sleep 10/0
    # head tracker with only a sensor.
    trackmach SZG_INTERSENSE station0_1 0/0/0
    # wand has 4 buttons, 2 axes.
    trackmach SZG_INTERSENSE station0_2 4/2/0
    # Rotate isense axes to szg axes with
    #  0  1  0  3  (x -> -z, y ->  x, z -> -y)
    #  0  0 -1  1  Then translate (3,1,-2) feet.
    # -1  0  0 -2  
    #  0  0  0  1
    trackmach SZG_INTERSENSE convert0_0 0/0/-1.0/2.0
    trackmach SZG_INTERSENSE convert0_1 1.0/0/0/1.0
    trackmach SZG_INTERSENSE convert0_2 0/-1.0/0/-2.0
    trackmach SZG_INTERSENSE convert0_3 0/0/0/1.0
    \endverbatim

    \c sleep <em>tickcount / any integer</em> - The number of ticks for the
    driver thread to sleep between polling.  The second integer is there by 
    accident of how the parameters work.  Just put zero.

    \c station<tracker>_<stationID> <em>buttonCnt/analogCnt/auxCnt</em> - 
    Intersense trackers don't advertise how many inputs they have, so 
    you can set them.  In order to find the tracker index and station ID,
    run the DeviceServer once and look at the diagnostic output where
    the stations are listed.

    \c convert<tracker>_<columnIndex> These four entries construct a
    4x4 matrix expressing how to rotate and translate the sensor's 
    output.  Each entry is a column in the matrix.
    */
bool arIntersenseDriver::init(arSZGClient& client) {

  // Retrieve settings from client
  int sig[2] = {10,0};
  if (!client.getAttributeInts("SZG_INTERSENSE","sleep",sig,2)) {
    _sleepTime = 10000;
    cerr << "arIntersenseDriver remark: SZG_INTERSENSE/sleep not set, "
         << "defaulting to ( " << _sleepTime << ")." << std::endl;
  } else {
    cerr << "arIntersenseDriver remark: SZG_INTERSENSE/sleep set to " 
         << " ( " << sig[0] << " ).\n";
    _sleepTime = sig[0];
  }

  DWORD comPortID = static_cast<DWORD>(client.getAttributeInt( "SZG_INTERSENSE", "com_port"));
  // Start the device.
  if ( !_open( comPortID ) ) {
    cerr << "arIntersenseDriver error: _open(" << comPortID << ") failed.\n";
    return false;
  }

  if ( !getStationSettings( client ) ) {
    cerr << "arIntersenseDriver error: getStationSettings() failed.\n";
    return false;
  }

  // Construct a numbering scheme for sensors, buttons, and axes.
  unsigned int matrixIndex = 0, buttonIndex = 0, axisIndex = 0;
  std::vector< IsenseTracker >::iterator iter;
  cerr << "numTrackers: " << _trackers.size() << endl;
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    // sensorIndex, buttonIndex, axisIndex are incremented by each tracker.
    iter->setStationIndices( matrixIndex, buttonIndex, axisIndex );
  }
  // Tell arInputSource how many we have counted.
  std::cerr << "IntersenseDriver::Totals buttons " << buttonIndex <<
	  " axes " << axisIndex << " matrices " << matrixIndex << std::endl;
  this->_setDeviceElements( buttonIndex, axisIndex, matrixIndex );

  // Print all information to stderr.
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    cerr << *iter << endl;
  }
  // We have succeeded
  return true;
}


bool arIntersenseDriver::_open( DWORD port ) {
  ISD_TRACKER_HANDLE trackerHandle[ ISD_MAX_TRACKERS ];
  std::vector< IsenseTracker >::iterator iter;
  int numTrackers;
  
  if (port==0) { // scan all ports for any available trackers
    // Open every available unit.
    Bool isOpened = ISD_OpenAllTrackers(
      (Hwnd) NULL, trackerHandle, FALSE, _isVerbose );
    if ( FALSE == isOpened ) {
      std::cerr << "IntersenseDriver::failed to open any trackers."
        << std::endl;
      return false;
    }

    // Count the good handles and make struct to hold them.
    int trackerIndex;
    for ( trackerIndex = 0; trackerIndex < ISD_MAX_TRACKERS; trackerIndex++ ) {
      if ( trackerHandle[ trackerIndex ] < 1 ) break;
    }
    numTrackers = trackerIndex;
  } else { // check for a single tracker on specified port
    Bool isVerbose = TRUE;
    trackerHandle[0] = ISD_OpenTracker( (Hwnd) NULL, port, FALSE, isVerbose );
    bool success = trackerHandle[0] > 0;
    if ( !success ) {
      std::cerr << "arIntersenseDriver error: failed to open tracker on port "
      << port << ".  " << std::endl;
      return false;
    }
    numTrackers = 1;
  }
  if ( numTrackers == 0 ) {
    std::cerr << "IntersenseDriver::found no trackers." << endl;
    return false;
  }
  _trackers.insert( _trackers.begin(), (unsigned int)numTrackers, IsenseTracker() );

  // Execute initialization code for each tracker.
  bool created = true;
  unsigned int count(0);
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    iter->setID( count );
    iter->setHandle( trackerHandle[ count++ ] );
    bool didInit = iter->init();
    if ( !didInit ) {
      created = false;
    }
  }
  // Close down if we cannot start.
  if ( !created ) {
    for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
      iter->ar_close();
    }
  }
  return created;
}


bool arIntersenseDriver::getStationSettings( arSZGClient& client ) {
  std::vector< IsenseTracker >::iterator iter;
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    if (!iter->configure( client )) {
      return false;
    }
  }
  return true;
}


/*! This calls the ar_usleep().
    */
bool arIntersenseDriver::_waitForData() {
  ar_usleep( _sleepTime );
  return true;
}


bool arIntersenseDriver::_getData() {
  bool success = true;
  std::vector< IsenseTracker >::iterator iter;
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    if ( !iter->getData( this ) ) {
      success = false;
    }
  }
  return success;
}


/*! This method should open all trackers which were open previously.
    \return False if not all failed trackers could be opened.
    */
bool arIntersenseDriver::_reacquire() {
  bool bAllRunning = true;

  std::vector< IsenseTracker >::iterator iter;
  for (iter = _trackers.begin(); iter != _trackers.end(); ++iter) {
    if (!iter->getStatus()) {
      bool reopened = iter->ar_reopen();
      if ( !reopened ) {
        bAllRunning = false;
      }
    }
  }
  return bAllRunning;
}

/*! Start a thread for ar_intersenseDriverEventTask to poll the
    driver and send events.
    */
bool arIntersenseDriver::start(){
  return _eventThread.beginThread(ar_intersenseDriverEventTask,this);
}

void arIntersenseDriver::handleMessage( const string& messageType, const string& messageBody ) {
  if (messageType != "arIntersenseDriver") {
    cerr << "arIntersenseDriver warning: ignoring message of type "
         << messageType << endl;
    return;
  }
  arDelimitedString tokens( messageBody, ' ' );
  if (tokens.size() < 1) {
    cerr << "arIntersenseDriver warning: ignoring message with 0 tokens.\n";
    return;
  }
  string command( tokens[0] );
  if (command == "reset") { // reset heading
    if (tokens.size() != 2) {
      cerr << "arIntersenseDriver warning: reset usage: reset <tracker #>/<station #>.\n";
      return;
    }
    int items[2];
    int numItems = ar_parseIntString( tokens[1], items, 2 );
    if (numItems != 2) {
      cerr << "arIntersenseDriver warning: reset usage: reset <tracker #>/<station #>.\n";
      return;
    }
    if (_resetHeading((unsigned int)items[0],(unsigned int)items[1])) {
      cout << "arIntersenseDriver remark: reset heading for tracker #" << items[0]
           << ", station #" << items[1] << endl;
    } else {
      cerr << "arIntersenseDriver warning: failed to reset heading for tracker #" << items[0]
           << ", station #" << items[1] << endl;
    }
  } else {
    cerr << "arIntersenseDriver warning: unknown comand " << command << endl;
  }
}

bool arIntersenseDriver::_resetHeading( unsigned int trackerID, unsigned int stationID ) {
  if (trackerID >= _trackers.size()) {
    cerr << "arIntersenseDriver error: attempt to reset heading for tracker "
         << trackerID << ",\n    max = " << _trackers.size() << endl;
    return false;
  }
  if (stationID >= ISD_MAX_STATIONS) {
    cerr << "arIntersenseDriver error: attempt to reset heading for station "
         << stationID << ",\n    max = " << ISD_MAX_STATIONS << endl;
    return false;
  }
  return ISD_ResetHeading( _trackers[trackerID].getHandle(), stationID );
}
