//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
// The arIntersenseDriver was written by
// Drew Dolgert ajd27@cornell.edu
//********************************************************

#ifndef AR_INTERSENSE_DRIVER_H
#define AR_INTERSENSE_DRIVER_H 1

#include "arInputSource.h"
#include "isense.h"

#include "arDriversCalling.h"

class IsenseStation;

//  Responsible for an individual Intersense Tracker and its stations.
/*  The tracker is the external box connected to the PC,
    and the stations are the various devices (wand, glasses) attached to
    the tracker.

    This class calls the Intersense SDK, which is a set of functions
    in isense.cpp which then load and call isense.dll.  The DLL is
    not required in order to compile.

    Notice that the interface does not provide us with the number of
    buttons, valuators, or AuxInputs on each station.
    */
class IsenseTracker {
public:
  // Constructor clears memory of structs.
  IsenseTracker();

  // Destructor closes tracker handle.
  ~IsenseTracker();

  // Ask Intersense dll to open a tracker on this port.
  bool ar_open( DWORD port );

  // Open a tracker on the same port the one where it was found before.
  bool ar_reopen();

  // Ask Intersense dll to close a tracker and maybe unload.
  bool ar_close();

  // Assign this object to an already opened tracker.
  void setHandle( ISD_TRACKER_HANDLE handle );
  // Return handle assigned to tracker.
  ISD_TRACKER_HANDLE getHandle() const { return _handle; }

  // Initialize a tracker after it has been opened.
  bool init();

  bool configure( arSZGClient& client );

  void setID( unsigned int id ) { _id = id; }
  unsigned int getID() const { return _id; }

  bool getStatus() const { return _imOK; }

  DWORD getPort() const { return _port; }

  // What indices to use when reporting to arInputSource.
  // Parameters are all passed by ref & altered in the function.
  void setStationIndices( unsigned int& matrixIndex,
                          unsigned int& buttonIndex,
                          unsigned int& axisIndex );

  // Send tracker data to an arInputSource.
  bool getData( arInputSource* source );

  vector< IsenseStation >& getStations() { return _stations; }

private:
  // Fill struct with configuration of tracker.
  bool _getTrackerConfig();

  // Query tracker for configuration information on all stations.
  void _loadAllStationInfo();

  // Tell the tracker to use native axes.
  bool _resetAllAlignmentReferenceFrames();

  // Enable possible use of stations with camera tracking.
  void _enablePossibleCameraTracking();

  // Determine whether this tracker's handle is good.
  bool _isValidHandle( ISD_TRACKER_HANDLE handle ) const { return handle>0; }

  unsigned int _id;
  bool _imOK;

  ISD_TRACKER_HANDLE _handle; // Handle to the tracker.
  ISD_TRACKER_INFO_TYPE _trackerInfo; // Tracker's configuration.
  ISD_TRACKER_DATA_TYPE _data;  // Single data block for all stations.

  // NOTE: unused
  ISD_CAMERA_DATA_TYPE _cameraData; // Single data block for all cameras.

  DWORD _port; // Actual port where tracker was found.

  bool _bSupportsPositionMeasurement; // Does tracker yields positions?
  bool _bSupportsCameraData; // Does tracker supports new camera devices?

  vector< IsenseStation > _stations;
};

ostream& operator<<(ostream&, const IsenseTracker& tc);

class IsenseStation {
  public:
    IsenseStation();
    ~IsenseStation();

    bool getStatus() const;
    unsigned int getID() const;
    unsigned int getMatrixIndex() const { return _matrixIndex; }
    Bool getCompass() const { return _stationConfig.Compass; }
    unsigned int getNumButtons() const { return _numButtons; }
    unsigned int getNumAnalogInputs() const { return _numAnalogInputs; }
    unsigned int getNumAuxInputs() const { return _numAuxInputs; }
    unsigned int getFirstButtonIndex() const { return _firstButtonIndex; }
    unsigned int getFirstAnalogIndex() const { return _firstAnalogIndex; }
    unsigned int getFirstAuxIndex() const { return _firstAuxIndex; }


    bool configure( arSZGClient& client, unsigned int trackerID );

    void setTrackerHandle( ISD_TRACKER_HANDLE handle ) { _trackerHandle = handle; }

    // Query tracker for configuration information on this station.
    void loadStationInfo( unsigned int stationID );

    bool resetAlignmentReferenceFrame();

    bool enableCameraTracking();

    void setIndices( unsigned int& matrixIndex,
                     unsigned int& buttonIndex,
                     unsigned int& axisIndex );

    void queueData( ISD_TRACKER_DATA_TYPE& data,
                    bool usePositionMeasurement,
                    arInputSource* inputSource );

  private:
    void _setInputCounts( unsigned int buttonCnt,
                          unsigned int analogCnt,
                          unsigned int auxCnt );

    bool _setUseCompass( unsigned int compassVal );

    // Intersense configuration info for this station.
    ISD_STATION_INFO_TYPE _stationConfig;

    ISD_TRACKER_HANDLE _trackerHandle; // handle for the tracker that owns this station.

    unsigned  int _matrixIndex; // szg matrix event index for pos/orient data from this sensor.
    unsigned  int _numButtons;      // Number of buttons set by user
    unsigned  int _numAnalogInputs; // Number of valuators set by user.
    unsigned  int _numAuxInputs;    // Number of aux inputs set by user.
    unsigned  int _firstButtonIndex; // Index into szg driver's list of buttons.
    unsigned  int _firstAnalogIndex; // Index into szg driver's list of axes.
    unsigned  int _firstAuxIndex;    // Index into szg driver's list of axes.

    // Only send button events when button state changes.
    Bool _lastButtonState[ISD_MAX_BUTTONS];
};

ostream& operator<<(ostream&, const IsenseStation&);

//  Driver for Intersense devices
/*  This driver can handle the IS-300, IS-600,
    IS-900, InertiaCube2, and all InterTrax.
    It has been tested only on the IS-900 VWT under Windows.

    All translational units are in feet.

    This driver requires isense.{dll,so} from Intersense in order to
    run.  If you do not find it in your original installation,
    the download it from http://www.isense.com/support/ as
    "InterSense Interface Libraries SDK Version 3.45".

    \author Drew Dolgert ajd27@cornell.edu
    \date 23 April 2003
    */

class arIntersenseDriver : public arInputSource {
  friend void ar_intersenseDriverEventTask(void*);
public:
  // Constructor does nothing.
  arIntersenseDriver();
  // Destructor closes trackers.
  ~arIntersenseDriver();

  bool init(arSZGClient&);
  bool start();

  virtual void handleMessage( const string& messageType, const string& messageBody );

private:
  arThread _eventThread;

  // Called by driver's ar_intersenseDriverEventTask to send input event.
  bool _getData();

  // Called by driver's ar_intersenseDriverEventTask to wait for more data.
  bool _waitForData();

  bool _reacquire(); // Re-open failed trackers.
  bool _open( DWORD port=0 ); // Open all Intersense trackers.

  // Makes all calls to retrieve client's group attributes.
  bool getStationSettings( arSZGClient& client );

  bool _resetHeading( unsigned  int trackerID, unsigned  int stationID );

  void _closeAll();

  vector< IsenseTracker > _trackers;

  int _sleepTime; // How many ticks to pause at each polling of trackers.
  Bool _isVerbose; // Should the tracker print?
};

#endif // AR_INTERSENSE_DRIVER_H
