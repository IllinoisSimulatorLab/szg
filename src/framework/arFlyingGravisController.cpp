//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This is used only by obj/viewOBJ.cpp, and should be retired.  (4/8/2002)

#include "arPrecompiled.h"
#include "arFlyingGravisController.h"
#include "arDataUtilities.h"

void ar_handleFlyingGravisJoystick(void* input) {
  arFlyingGravisController* ctrl = (arFlyingGravisController*)input;
  // good for Xterminator gamepad
  while (true) {
    const float j0 = ctrl->_joystickClient.getAxis(0);
    const float j1 = ctrl->_joystickClient.getAxis(1);
    const float j2 = ctrl->_joystickClient.getAxis(2);
    const float j3 = ctrl->_joystickClient.getAxis(3);
    const arMatrix4 xRot(ar_rotationMatrix('x', j1/300000.));
    const arMatrix4 yRot(ar_rotationMatrix('y', j0/300000.));
    const arMatrix4 zRot(ar_rotationMatrix('z', ((j3 - j2))/1000000.));
    ctrl->setTransform(xRot * yRot * zRot * ctrl->getTransform());
    ar_usleep(10000);
  }
}

arFlyingGravisController::arFlyingGravisController() {
}

arFlyingGravisController::~arFlyingGravisController() {
  //todo: clean up threads
}

bool arFlyingGravisController::init(arSZGClient& szgClient) {
  _joystickClient.addInputSource(&_netInputSource, false);
  return _netInputSource.setSlot(0) && _joystickClient.init(szgClient);
}

bool arFlyingGravisController::start() {
  const bool ok = _joystickClient.start();
  _transformThread.beginThread(ar_handleFlyingGravisJoystick, this);
  return ok;
}
