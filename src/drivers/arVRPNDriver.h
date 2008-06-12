//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VRPN_DRIVER_H
#define AR_VRPN_DRIVER_H

#include "arInputSource.h"

#include "arDriversCalling.h"

#ifdef Enable_VRPN
#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#endif

// Driver for connecting to a VRPN server

class SZG_CALL arVRPNDriver: public arInputSource{
  friend void ar_VRPNDriverEventTask(void*);
#ifdef Enable_VRPN
  friend void ar_VRPNHandleTracker(void* data, const vrpn_TRACKERCB event);
  friend void ar_VRPNHandleButton(void* data, const vrpn_BUTTONCB event);
  friend void ar_VRPNHandleAnalog(void* data, const vrpn_ANALOGCB event);
#endif
 public:
  arVRPNDriver();
  ~arVRPNDriver();

  bool init(arSZGClient& client);
  bool start();
  bool stop();
 private:
  arThread _eventThread;
  string vrpnDeviceName;
#ifdef Enable_VRPN
  int vrpnNumberButtons;
  int vrpnNumberAxes;
  int vrpnNumberMatrices;

  vrpn_Analog_Remote*  vrpnAnalogRemote;
  vrpn_Button_Remote*  vrpnButtonRemote;
  vrpn_Tracker_Remote* vrpnTrackerRemote;
#endif
};

#endif
