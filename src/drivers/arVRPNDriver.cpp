//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arVRPNDriver.h"

DriverFactory(arVRPNDriver, "arInputSource")

#ifdef Enable_VRPN
void ar_VRPNHandleTracker(void* data, const vrpn_TRACKERCB event) {
  arVRPNDriver* vrpn = (arVRPNDriver*) data;
  // Reorder from Syzygy's scalar-last quaternion to VRPN's scalar-first quaternion.
  const arMatrix4 rotMatrix(arQuaternion(
    event.quat[3], event.quat[0], event.quat[1], event.quat[2]));
  const arMatrix4 transMatrix(ar_translationMatrix(event.pos[0], event.pos[1], event.pos[2]));
  vrpn->sendMatrix(event.sensor, transMatrix*rotMatrix);
}

void ar_VRPNHandleButton(void* data, const vrpn_BUTTONCB event) {
  arVRPNDriver* vrpn = (arVRPNDriver*) data;
  vrpn->sendButton(event.button, event.state);
}

void ar_VRPNHandleAnalog(void* data, const vrpn_ANALOGCB event) {
  arVRPNDriver* vrpn = (arVRPNDriver*) data;
  for (int i=0; i<event.num_channel; i++) {
    vrpn->queueAxis(i, event.channel[i]);
  }
  vrpn->sendQueue();
}
#endif

void ar_VRPNDriverEventTask(void*
#ifdef Enable_VRPN
    VRPNDriver
#endif
    ) {
#ifdef Enable_VRPN
  arVRPNDriver* vrpn = (arVRPNDriver*) VRPNDriver;
  while (true) {
    if (vrpn->vrpnAnalogRemote) {
      // vrpn works via polling
      vrpn->vrpnAnalogRemote->mainloop();
      vrpn->vrpnButtonRemote->mainloop();
      vrpn->vrpnTrackerRemote->mainloop();
      ar_usleep(5000);
    }
  }
#endif
}

arVRPNDriver::arVRPNDriver() {
#ifdef Enable_VRPN
  vrpnNumberButtons = 0;
  vrpnNumberAxes = 0;
  vrpnNumberMatrices = 0;
  vrpnDeviceName = string("NULL");

  vrpnAnalogRemote = NULL;
  vrpnButtonRemote = NULL;
  vrpnTrackerRemote = NULL;
#endif
}

arVRPNDriver::~arVRPNDriver() {
}

bool arVRPNDriver::init(arSZGClient& client) {
  vrpnDeviceName = client.getAttribute("SZG_VRPN", "name");
#ifdef Enable_VRPN
  // it doesn't seem like vrpn propogates signatures forward...
  // so let's just set the ceiling pretty high
  _setDeviceElements(20, 20, 20);
  if (vrpnDeviceName == "NULL") {
    ar_log_remark() << "arVRPNDriver error: no SZG_VRPN/name.\n";
    return false;
  }
  // initialize vrpn device
  vrpnAnalogRemote = new vrpn_Analog_Remote(vrpnDeviceName.c_str());
  vrpnButtonRemote = new vrpn_Button_Remote(vrpnDeviceName.c_str());
  vrpnTrackerRemote = new vrpn_Tracker_Remote(vrpnDeviceName.c_str());
  if (!vrpnAnalogRemote || !vrpnButtonRemote || !vrpnTrackerRemote) {
    ar_log_remark() << "arVRPNDriver error: vrpn device failed to initialize.\n";
    return false;
  }
  vrpnAnalogRemote->register_change_handler(this, ar_VRPNHandleAnalog);
  vrpnButtonRemote->register_change_handler(this, ar_VRPNHandleButton);
  vrpnTrackerRemote->register_change_handler(this, ar_VRPNHandleTracker);
  return true;
#else
  _setDeviceElements(0, 0, 0);
  return true;
#endif
}

bool arVRPNDriver::start() {
  return _eventThread.beginThread(ar_VRPNDriverEventTask, this);
}

bool arVRPNDriver::stop() {
  // not implemented yet!
  return true;
}
