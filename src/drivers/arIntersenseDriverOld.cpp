//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
// The arIntersenseDriver was written by 
// Drew Dolgert ajd27@cornell.edu
//********************************************************

#include "arPrecompiled.h"
#include "arIntersenseDriver.h"

DriverFactory(arIntersenseDriver, "arInputSource")

#define ISENSE_GROUP_NAME "SZG_INTERSENSE"

const float IsenseTracker::c_meterToFoot = 3.280839895;

IsenseTracker::IsenseTracker() : m_handle(0)
{
  ::memset( (void*) m_station, 0, sizeof( m_station ) );

  // Set the button counts now with maximum defaults.
  for (
    int stationIdx = 1;
    stationIdx <= ISD_MAX_STATIONS;
    stationIdx++
    )
  {
      m_stationSettings[stationIdx-1].buttonCnt = ISD_MAX_BUTTONS;
      m_stationSettings[stationIdx-1].analogCnt = ISD_MAX_CHANNELS;
      m_stationSettings[stationIdx-1].auxCnt    = ISD_MAX_AUX_INPUTS;
  }
}

IsenseTracker::~IsenseTracker()
{
  Close();
}

/*! We load the configuration for the tracker in order
    to find out what model we have and its capabilities.
    It is here that we find out what the actual port is.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::GetTrackerConfig()
{
  Bool success =
    ISD_GetTrackerConfig( m_handle, &m_trackerInfo, m_isVerbose );
  if ( FALSE == success ) {
    std::cerr << "Intersense::got no tracker information on " <<
      "device port " << m_port << "\n";
    return false;
  }

  // Set the local variables that derive from tracker info.
  m_port = m_trackerInfo.Port;
  DWORD model = m_trackerInfo.TrackerModel;
  if ( (ISD_IS600 == model) || (ISD_IS900 == model) ||
    (ISD_IS1200 == model) )
    m_bSupportsPositionMeasurement = true;
  else m_bSupportsPositionMeasurement = false;

  // Tell the world our configuration
  std::cerr << "IntersenseDriver::Started tracker on port " << m_port << 
    "." << "\n";
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
bool IsenseTracker::LoadStationInfo()
{
  for ( int statIdx = 1; statIdx <= ISD_MAX_STATIONS; statIdx++ ) {
    Bool gotConfig = ISD_GetStationConfig(
      m_handle,
      &m_station[ statIdx-1 ],
      statIdx,
      m_isVerbose
      );
    if ( false == gotConfig ) {
      m_station[statIdx-1].State = 0;
    }
  }
  return true;
}


/*! We ask only that this work for at least one of the stations
    because it is reasonable that it might not work for a station
    which is not connected.  We could use the A command to set
    the internal reference frame of the tracker, and that would 
    likely result in a faster driver, but what we have in our
    GetData works for now.

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::ResetAlignmentReferenceFrame()
{
  bool success = false;
  char chReset[10];
  for ( int statIdx = 1; statIdx <= ISD_MAX_STATIONS; statIdx++ ) {
    sprintf( chReset, "R%d\n", statIdx );
    Bool bSent = ISD_SendScript( m_handle, chReset );
    if ( TRUE == bSent ) success = true;
  }
  return success;
}


/*! The Intersense station will not, by default, have camera
    tracking enabled, so this method turns it on.  This method has
    not been tested with an actual camera device.

    \return False indicates the driver will not do camera tracking,
        which may not be a failure.
    */
bool IsenseTracker::EnablePossibleCameraTracking()
{
  // The sample code has the line below, but CAMERA_TRACKER
  // must just be a define.  We'll turn it on by default.
  // ( CAMERA_TRACKER && m_trackerInfo.TrackerModel == ISD_IS900)
  if ( (ISD_PRECISION_SERIES == m_trackerInfo.TrackerType ) &&
    ( m_trackerInfo.TrackerModel == ISD_IS900) )
  {
    for (
      int stationIdx = 1;
      stationIdx <= ISD_MAX_STATIONS;
      stationIdx++
      )
      {
        if ( FALSE == m_station[ stationIdx-1 ].State ) continue;
        if ( TRUE == m_station[ stationIdx-1 ].GetCameraData ) continue;

        m_station[ stationIdx-1 ].GetCameraData = TRUE;
        Bool setConfig = ISD_SetStationConfig(
          m_handle,
          &m_station[ stationIdx-1 ],
          stationIdx,
          m_isVerbose
          );
        if ( FALSE == setConfig ) {
          std::cerr << "IntersenseDriver::failed to set configuration "
            << "for station " << stationIdx << "\n";
          return false;
        }

        m_bSupportsCameraData = true;
      }
  } else {
    m_bSupportsCameraData = false;
  }
  return true;
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
bool IsenseTracker::Reopen( )
{
  return Open( m_port );
}


/*!
    \param port The port can refer to an RS232 port or a USB port,
        numbered according to some Intersense-internal scheme not
        documented.
        Sending 0 indicates the first one found should be opened.
    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::Open( DWORD port = 0 )
{
  Close();
  Bool isVerbose = TRUE;
  m_port = port;
  m_handle = ISD_OpenTracker( (Hwnd) NULL, port, FALSE, isVerbose );
  bool success = IsValidHandle( m_handle );
  if ( !success )
    std::cerr << "IntersenseDriver::failed to open tracker on port "
    << port << ".  " <<
    "Set verbosity to true to see reason for failure." << "\n";
  return success;
}


/*! \param handle The handle should have been returned from
        a call to ISD_OpenTracker or ISD_OpenAllTrackers.
    */
void IsenseTracker::SetHandle( ISD_TRACKER_HANDLE handle )
{
  Close();
  m_handle = handle;
}


/*! You must call Open() or SetHandle() before calling Init().

    \return False indicates failure of the driver.  This method
        will print problem to stderr.
    */
bool IsenseTracker::Init()
{
  if ( !GetTrackerConfig() )             return false;
  if ( !LoadStationInfo() )              return false;
  if ( !ResetAlignmentReferenceFrame() ) return false;
  // This isn't appropriate for VR?
  //if ( !EnablePossibleCameraTracking() ) return false;
  return true;
}


/*! This method is fine to call on an already closed or
    never opened tracker.

    \return False on failure to close.  Nothing to be
        done if it fails unless you can unload the Dll.
    */
bool IsenseTracker::Close()
{
  Bool success = TRUE;
  if ( IsValidHandle( m_handle ) )
    success = ISD_CloseTracker( m_handle );
  if ( FALSE == success ) return false;
  m_handle = 0;
  return true;
}


/*! This method will not find the station requested until the IDs have
    been read during LoadStationInfo().
  
    \param ID The Intersense station ID, usually 1-4.
    \param buttonCnt Number of buttons the station should have.
    \param analogCnt Number of analogs (axes) the station should have.
    \param auxCnt Number of aux inputs the station should have.  No 
        indication what an aux input might be, but it returns type BYTE.
    \return False indicates the station ID given could not be found.
    This method will print problem to stderr.
    */
bool IsenseTracker::SetStationInputCounts( int ID, size_t buttonCnt, size_t analogCnt,
    size_t auxCnt )
{
  bool success = false;
  for ( size_t statIdx = 0; statIdx < ISD_MAX_STATIONS; statIdx++ ) {
    if ( m_station[ statIdx ].ID == ID ) {
      m_stationSettings[ statIdx ].buttonCnt = buttonCnt;
      m_stationSettings[ statIdx ].analogCnt = analogCnt;
      m_stationSettings[ statIdx ].auxCnt    = auxCnt;
      success = true;
      break;
    }
  }
  if ( false == success )
    std::cerr << "IntersenseDriver::found no station with ID " <<
      ID << " in order to set its buttons, analogs, and auxes." << "\n";
  return success;
}


/*! This object uses both configuration information and
    the user's settings of the number of buttons and such in order
    to create an internal array of how to remap buttons, analogs
    and aux inputs to buttons and axes.

    All parameters are incremented upon return.

    \param sensor Index of the tracker in the arInputSource array.
    \param button Index of first button in arInputSource array.
    \param axis Index of first axis in arInputSource array.
    \return Index of next available sensor, button, and axis.
    */
bool IsenseTracker::SetDriverFirstIndices( size_t& sensor, size_t& button,
    size_t& axis )
{
  for ( size_t statIdx = 0; statIdx < ISD_MAX_STATIONS; statIdx++ ) {
    if ( FALSE == m_station[ statIdx ].State ) continue;

    m_stationSettings[ statIdx ].sensorIdx = sensor++;

    m_stationSettings[ statIdx ].firstButtonIdx = button;
    button += m_stationSettings[ statIdx ].buttonCnt;

    m_stationSettings[ statIdx ].firstAnalogIdx = axis;
    axis   += m_stationSettings[ statIdx ].analogCnt;

    m_stationSettings[ statIdx ].firstAuxIdx = axis;
    axis   += m_stationSettings[ statIdx ].auxCnt;
  }
  return true;
}


/*! Prints to stderr all the setup information.
    \param trackerIdx Index of this tracker in arInputSource
        array.
    \return true always.
    */
bool IsenseTracker::PrintSetup( size_t trackerIdx )
{
  std::cerr << "Intersense Tracker on port " << m_port << "\n";
  for ( size_t statIdx=0; statIdx < ISD_MAX_STATIONS; statIdx++ ) {
    if ( FALSE == m_station[ statIdx ].State ) continue;
    std::cerr << "\tStation ID " << m_station[ statIdx ].ID <<
      " station" << trackerIdx << "_" << m_station[statIdx].ID << "\n";
    std::cerr << "\t\tbuttons " << m_stationSettings[ statIdx ].buttonCnt << " ";
    std::cerr << "analogs " << m_stationSettings[ statIdx ].analogCnt << " ";
    std::cerr << "aux " << m_stationSettings[ statIdx ].auxCnt << "\n";
    std::cerr << "\t\tfirst button " <<
      m_stationSettings[ statIdx ].firstButtonIdx << " ";
    std::cerr << "first analog " << m_stationSettings[ statIdx ].firstAnalogIdx <<
      "\n";
  }
  return true;
}


/*! This conversion matrix sets a rotation and subsequent translation
    from the tracker's frame to the frame szg will use.
    \return true always.
    */
bool IsenseTracker::SetConversionMatrix( arMatrix4& convert )
{
  m_conversionMatrix = convert;
  std::cerr << "IntersenseDriver::Setting conversion matrix to " <<
    "\n" << m_conversionMatrix;
  return true;
}


/*! You must call Init() before calling GetData().
    The data is translated so that the Intersense position and
    orientation becomes a szg sensor, buttons become szg buttons,
    analogs become szg axes normalized (-1, 1), and aux inputs 
    become szg axes normalized (0, 1).  Not having seen an aux
    input or any documentation about it, I don't know whether
    this normalization is correct.

    \return False indicates we could not retrieve data.
    */
bool IsenseTracker::GetData( arInputSource* source )
{
  if ( FALSE == ISD_GetData( m_handle, &m_data ) ) {
    std::cerr << "IntersenseDriver::failed to get data from tracker " << 
      m_handle << "\n";
    return false;
  }

  // We retrieve camera data, but we don't know what to
  // do with "ApertureEncoder, FocusEncoder, and ZoomEncoder."
  if ( m_bSupportsCameraData ) {
    if (FALSE == ISD_GetCameraData( m_handle, &m_cameraData ) ) {
      std::cerr << "IntersenseDriver::got no camera data from tracker "
        << m_handle << "\n";
      return false;
    }
  }

  for ( int station=1; station <= ISD_MAX_STATIONS; station++ ) {
    ISD_STATION_INFO_TYPE*  stationInfo = &m_station[station-1];
    ISD_STATION_STATE_TYPE* stationData = &m_data.Station[station-1];
    StationSettings* stationSettings    = &m_stationSettings[station-1];

    if ( FALSE == stationInfo->State ) continue;

    arMatrix4 theTransMatrix;
    if ( m_bSupportsPositionMeasurement ) {
    	// Rotate the position separately.
      theTransMatrix =
        ar_translationMatrix(m_conversionMatrix*arVector3(
             c_meterToFoot*stationData->Position[0],
             c_meterToFoot*stationData->Position[1],
             c_meterToFoot*stationData->Position[2]));
    } // else just use identity translation matrix.

    arMatrix4 theRotMatrix;
    if ( ISD_QUATERNION == stationInfo->AngleFormat ) {
      theRotMatrix =
        ar_transrotMatrix(arVector3(0, 0, 0),
        arQuaternion(stationData->Orientation[0],
        stationData->Orientation[1],
        stationData->Orientation[2],
        stationData->Orientation[3]));
    } else { // Euler matrices
      // The Intersense Euler angles are presented as yaw, pitch, roll,
      // which are a rotation about z for yaw, y for pitch, and x for roll.
      // Documentation shows right-handed coordinate system and positive
      // angles.
      // The rotation is in the old coordinate system.
      theRotMatrix =
        ar_rotationMatrix('z', ar_convertToRad( stationData->Orientation[0]))*
        ar_rotationMatrix('y', ar_convertToRad( stationData->Orientation[1]))*
        ar_rotationMatrix('x', ar_convertToRad( stationData->Orientation[2]));
    }
    // We express the rotation in the new coordinate system.
    arMatrix4 transformedMatrix = (m_conversionMatrix)*theRotMatrix*
      (~m_conversionMatrix);
    source->sendMatrix( stationSettings->sensorIdx,
      theTransMatrix*transformedMatrix );

    const int analogCenter = 127;
    // Normalize axis from (0, 255) to (-1, 1).
    const float axisMultiple = 1.0f/128.0f;
    if ( TRUE == stationInfo->GetInputs ) {
      // Get buttons
      for ( size_t butIdx=0; butIdx < stationSettings->buttonCnt; butIdx++ ) {
        source->queueButton( stationSettings->firstButtonIdx+butIdx,
          stationData->ButtonState[butIdx] );
      }
      // Get analogs
      for ( size_t analogIdx=0;
            analogIdx < stationSettings->analogCnt;
            analogIdx++ ) {
         source->queueAxis( stationSettings->firstAnalogIdx+analogIdx,
           axisMultiple*(stationData->AnalogData[ analogIdx ]-analogCenter) );
      }
    }

    // Get aux inputs, whatever they are.
    const BYTE auxCenter = 0;
    // Normalize BYTE from (0, 255) to (0, 1).
    const float auxMultiple = 1.0f/255.0f;
    if ( TRUE == stationInfo->GetAuxInputs ) {
      for ( size_t auxIdx = 0; auxIdx < stationSettings->auxCnt; auxIdx++ ) {
        source->queueAxis( stationSettings->firstAuxIdx+auxIdx,
          auxMultiple*(stationData->AuxInputs[ auxIdx ]-auxCenter) );
      }
    }
    // Send results as an event.
    source->sendQueue();
  }
  return true;
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
void ar_intersenseDriverEventTask(void* intersenseDriver) {
  arIntersenseDriver* isense =
    (arIntersenseDriver*) intersenseDriver;

  for (;;) {
    isense->WaitForData();
    bool bSuccess = isense->GetData();
    if ( false == bSuccess ) {
      // Never get here b/c Intersense driver always 
      // returns a cheery success.
      while ( false == isense->Reacquire() )
        ar_usleep( 15*1000 );
    }
  }
}


/***********************************************************/
/*                     ar_intersenseDriver                 */
/***********************************************************/

arIntersenseDriver::arIntersenseDriver()
  :m_tracker(0), m_trackerFailed(0), m_isVerbose(TRUE) {
  // does nothing yet
}


arIntersenseDriver::~arIntersenseDriver() {
  if ( 0 != m_tracker )       delete[] m_tracker;
  if ( 0 != m_trackerFailed ) delete[] m_trackerFailed;
}


bool arIntersenseDriver::GetStationSettings( arSZGClient& client )
{
  const size_t charCnt = 200;
  char chStation[ charCnt ];

  int sig[3] = {0, 0, 0};
  for ( size_t trackIdx=0; trackIdx < m_trackerCnt; trackIdx++ ) {
    for ( size_t statIdx=0; statIdx <= ISD_MAX_STATIONS; statIdx++ ) {
      sprintf( chStation, "station%d_%d", trackIdx, statIdx );
      if ( client.getAttributeInts( ISENSE_GROUP_NAME, chStation, sig, 3 ) ) {
        m_tracker[trackIdx].SetStationInputCounts( statIdx, sig[0], sig[1],
          sig[2] );
        std::cerr << "IntersenseDriver::Set station " << statIdx << " to " <<
          sig[0] << ":" << sig[1] << ":" << sig[2] << "\n";
      }
    }
  }

  char chTracker[ charCnt ];
  float matElems[16] = { 0,0,-1,0, 1,0,0,0, 0,-1,0,0, 0,0,0,1 };
  for ( size_t matrixIdx=0; matrixIdx < m_trackerCnt; matrixIdx++ ) {
    size_t foundCnt = 0;
    for ( size_t vecIdx=0; vecIdx < 4; vecIdx++ ) {
      sprintf( chTracker, "convert%d_%d", matrixIdx, vecIdx );
      if ( client.getAttributeFloats( ISENSE_GROUP_NAME, chTracker,
        matElems+4*vecIdx, 4 ) ) {
          foundCnt++;
      }
    }
    if ( foundCnt > 0 ) {
      if ( 4 == foundCnt ) {
        arMatrix4 convMatrix( matElems );
        m_tracker[ matrixIdx ].SetConversionMatrix( convMatrix );
      } else {
        std::cerr << "IntersenseDriver::Found less than four vectors "
          "to make the matrix for tracker " << matrixIdx << "\n";
        return false;
      }
    }
  }
  return true;
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
    #  0  0 -1  1  Then translate (3, 1, -2) feet.
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
  int sig[2] = {10, 0};
  if (!client.getAttributeInts(ISENSE_GROUP_NAME, "sleep", sig, 2)) {
    m_sleepTime = 10;
    ar_log_remark() << "arIntersenseDriver: " << ISENSE_GROUP_NAME <<
      "/sleep defaulting to " << m_sleepTime << ".\n";
  } else {
    ar_log_remark() << "arIntersenseDriver: " << ISENSE_GROUP_NAME <<
      "/sleep set to " << sig[0] << ".\n";
    m_sleepTime = sig[0];
  }

  DWORD comPortID = static_cast<DWORD>(client.getAttributeInt( ISENSE_GROUP_NAME, "com_port"));
  // Start the device.
  if ( !Open( comPortID ) ) return false;

  if ( !this->GetStationSettings( client ) ) return false;

  // Construct a numbering scheme for sensors, buttons, and axes.
  size_t sensorIdx = 0, buttonIdx = 0, axisIdx = 0;
  for ( size_t trackIdx=0; trackIdx < m_trackerCnt; trackIdx++ ) {
    // sensorIdx, buttonIdx, axisIdx are incremented by each tracker.
    if ( !m_tracker[ trackIdx ].SetDriverFirstIndices( sensorIdx, buttonIdx,
      axisIdx ) ) return false;
  }
  // Tell arInputSource how many we have counted.
  std::cerr << "IntersenseDriver::Totals buttons " << buttonIdx <<
	  " axes " << axisIdx << " matrices " << sensorIdx << "\n";
  this->_setDeviceElements( buttonIdx, axisIdx, sensorIdx );

  // Print all information to stderr.
  for ( size_t printIdx=0; printIdx < m_trackerCnt; printIdx++ ) {
    m_tracker[ printIdx ].PrintSetup( printIdx );
  }
  return true;
}


bool arIntersenseDriver::Open( DWORD port )
{
  ISD_TRACKER_HANDLE trackerHandle[ ISD_MAX_TRACKERS ];

  if (port==0) { // scan all ports for any available trackers
    // Open every available unit.
    Bool isOpened = ISD_OpenAllTrackers(
      (Hwnd) NULL, trackerHandle, FALSE, m_isVerbose );
    if ( FALSE == isOpened ) {
      std::cerr << "IntersenseDriver::failed to open any trackers."
        << "\n";
      return false;
    }

    // Count the good handles and make struct to hold them.
    int trackerIdx;
    for ( trackerIdx = 0; trackerIdx < ISD_MAX_TRACKERS; trackerIdx++ ) {
      if ( trackerHandle[ trackerIdx ] < 1 ) break;
    }
    m_trackerCnt = trackerIdx;
  } else { // check for a single tracker on specified port
    Bool isVerbose = TRUE;
    trackerHandle[0] = ISD_OpenTracker( (Hwnd) NULL, port, FALSE, isVerbose );
    bool success = trackerHandle[0] > 0;
    if ( !success ) {
      std::cerr << "arIntersenseDriver error: failed to open tracker on port "
      << port << ".  " << "\n";
      return false;
    }
    m_trackerCnt = 1;
  }
  if ( 0 == m_trackerCnt ) {
	  std::cerr << "IntersenseDriver::found no trackers." <<
		  "\n";
	  return false;
  }
  m_tracker = new IsenseTracker[ m_trackerCnt ];
  m_trackerFailed = new bool[ m_trackerCnt ];

  // Execute initialization code for each tracker.
  bool created = true;
  for ( size_t createIdx = 0; createIdx < m_trackerCnt; createIdx++ ) {
    m_tracker[createIdx].SetHandle( trackerHandle[ createIdx ] );
    bool didInit = m_tracker[createIdx].Init();
    if ( didInit ) {
      m_trackerFailed = false;
    } else {
      // Don't set trackerFailed b/c the tracker never started.
      // If things don't start at first, then we just quit.
      created = false;
    }
  }

  // Close down if we cannot start.
  if ( !created ) {
    for ( size_t closeIdx = 0; closeIdx < m_trackerCnt; closeIdx++ ) {
      m_tracker[closeIdx].Close();
    }
  }
  return created;
}

bool arIntersenseDriver::WaitForData()
{
  ar_usleep( m_sleepTime );
  return true;
}

bool arIntersenseDriver::GetData()
{
  bool success = true;
  for ( size_t trackerIdx=0; trackerIdx<m_trackerCnt; trackerIdx++ ) {
    if ( !m_tracker[ trackerIdx ].GetData( this ) ) {
      success = false;
      m_trackerFailed[ trackerIdx ] = true;
    }
  }
  return success;
}

/*! Open all trackers which were open previously.
    \return False if not all failed trackers could be opened.
    */
bool arIntersenseDriver::Reacquire()
{
  bool bAllRunning = true;

  for ( size_t trackIdx = 0; trackIdx < m_trackerCnt; trackIdx++ ) {
    if ( m_trackerFailed[ trackIdx ] ) {
      if ( m_tracker[ trackIdx ].Reopen() ) {
        m_trackerFailed[ trackIdx ] = false;
      } else {
        bAllRunning = false;
      }
    }
  }
  return bAllRunning;
}

/*! Start a thread for ar_intersenseDriverEventTask to poll the
    driver and send events.
    */
bool arIntersenseDriver::start() {
  return _eventThread.beginThread(ar_intersenseDriverEventTask, this);
}
