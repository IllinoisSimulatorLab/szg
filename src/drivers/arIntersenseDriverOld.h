//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
// The arIntersenseDriver was written by 
// Drew Dolgert ajd27@cornell.edu
//********************************************************

#ifndef AR_INTERSENSE_DRIVER_H
#define AR_INTERSENSE_DRIVER_H 1

#include "arInputSource.h"
#include "arThread.h"
#include "arInputHeaders.h"

#include "isense.h"

//! Responsible for an individual Intersense Tracker and its stations.
/*! The tracker is the external box connected to the PC,
    and the stations are the various devices (wand, glasses) attached to
    the tracker.
    
    This class calls the Intersense SDK, which is a set of functions
    in isense.cpp which then load and call isense.dll.  The DLL is
    not required in order to compile.
    
    There is the option to write our driver by copying code from the
    Intersense driver, isense.dll, because they provide the code.  We
    have gone with the simpler route where we write something that calls
    isense.dll.
    
    Notice that the interface does not provide us with the number of
    buttons, valuators, or AuxInputs on each station.
    */
class IsenseTracker
{
public:
  //! Constructor clears memory of structs.
  IsenseTracker();
  //! Destructor closes tracker handle.
  ~IsenseTracker();
  //! Ask Intersense dll to open a tracker on this port.
  bool Open( DWORD port );
  //! Open a tracker on the same port the one where it was found before.
  bool Reopen();
  //! Assign this object to an already opened tracker.
  void SetHandle( ISD_TRACKER_HANDLE handle );
  //! Initialize a tracker after it has been opened.
  bool Init();
  //! Set number of inputs on a station.
  bool SetStationInputCounts( int ID, size_t buttonCnt, size_t analogCnt,
    size_t auxCnt );
  //! What indices to use when reporting to arInputSource.
  bool SetDriverFirstIndices( size_t& sensor, size_t& button, size_t& axis );
  //! Print setup to stderr.
  bool PrintSetup( size_t trackerIdx );
  //! Set the 4x4 matrix to translate and rotate coordinates
  bool SetConversionMatrix( arMatrix4& convert );
  //! Send tracker data to an arInputSource.
  bool GetData( arInputSource* source );
  //! Ask Intersense dll to close a tracker and maybe unload.
  bool Close();
private:
  //! Query tracker for configuration information on all stations.
  bool LoadStationInfo();
  //! Fill struct with configuration of tracker.
  bool GetTrackerConfig();
  //! Tell the tracker to use native axes.
  bool ResetAlignmentReferenceFrame();
  //! Enable possible use of stations with camera tracking.
  bool EnablePossibleCameraTracking();
  //! Determine whether this tracker's handle is good.
  bool IsValidHandle( ISD_TRACKER_HANDLE handle ) {
    return handle > 0;
  }

  //! Handle to the tracker.
  ISD_TRACKER_HANDLE m_handle;
  DWORD m_port; //!< Actual port where tracker was found.

  ISD_TRACKER_DATA_TYPE m_data;  //!< Single data block for all stations.
  ISD_CAMERA_DATA_TYPE m_cameraData; //!< Single data block for all cameras.
  static const float c_meterToFoot;  //!< Could put in conversion matrix.
  arMatrix4 m_conversionMatrix;  //! Translates and rotates input.

  // This type, Bool, comes from isense.h.
  Bool m_isVerbose; //!< Whether the tracker prints messages.
  bool m_bSupportsPositionMeasurement; //!< Whether tracker yields positions.
  bool m_bSupportsCameraData; //!< Whether tracker supports new camera devices.
  ISD_TRACKER_INFO_TYPE m_trackerInfo; //!< Configuration of the tracker.
  //! Configuration of stations.  These take seconds to load.
  ISD_STATION_INFO_TYPE m_station[ISD_MAX_STATIONS]; 

  struct StationSettings {
    size_t buttonCnt; //!< Number of buttons set by user
    size_t analogCnt; //!< Number of valuators set by user.
    size_t auxCnt;    //!< Number of aux inputs set by user.
    
    size_t sensorIdx;     //!< Index into szg driver's list of sensors.
    size_t firstButtonIdx;//!< Index into szg driver's list of buttons.
    size_t firstAnalogIdx;//!< Index into szg driver's list of axes.
    size_t firstAuxIdx;   //!< Index into szg driver's list of axes.
  };
  StationSettings m_stationSettings[ISD_MAX_STATIONS];
};


//! Driver for Intersense devices
/*! This driver can handle the IS-300, IS-600,
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
  //! Constructor does nothing.
  arIntersenseDriver();
  //! Destructor closes trackers.
  ~arIntersenseDriver();

  bool init(arSZGClient&);
  bool start();
private:
  arThread _eventThread;

  //! Called by driver's ar_intersenseDriverEventTask to send input event.
  bool GetData();
  //! Called by driver's ar_intersenseDriverEventTask to wait for more data.
  bool WaitForData();
  //! Re-open failed trackers.
  bool Reacquire();
  //! Open all Intersense trackers.
  bool Open( DWORD port=0 );
  //! Makes all calls to retrieve client's group attributes.
  bool GetStationSettings( arSZGClient& client );
  size_t m_trackerCnt; //!< Number of Intersense tracker stations attached.
  IsenseTracker* m_tracker; //!< Pointer to list of trackers.
  bool *m_trackerFailed; //!< Whether the tracker has failed.
  int m_sleepTime; //!< How many ticks to pause at each polling of trackers.
  Bool m_isVerbose; //!< Whether the tracker should print.
};


#endif // AR_INTERSENSE_DRIVER_H
