//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This is used only by obj/viewOBJ.cpp, and should be retired.  (4/8/2002)

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFlyingGravisController.h"
#include "arDataUtilities.h"
#include "arThread.h"

void ar_handleFlyingGravisJoystick(void* input){
  arFlyingGravisController* ctrl = (arFlyingGravisController*)input;
  // good for Xterminator gamepad
  while (1){
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

arFlyingGravisController::arFlyingGravisController(){
  // does nothing so far!
}

arFlyingGravisController::~arFlyingGravisController(){
  //eventually, I'll want to clean up the threads in here
}

bool arFlyingGravisController::init(arSZGClient& szgClient){
  _joystickClient.addInputSource(&_netInputSource,false);
  _netInputSource.setSlot(0);
  return _joystickClient.init(szgClient);
}

bool arFlyingGravisController::start(){
  bool state = _joystickClient.start();
  _transformThread.beginThread(ar_handleFlyingGravisJoystick,this);
  return state;
}
