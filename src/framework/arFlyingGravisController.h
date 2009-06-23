//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FLYING_GRAVIS_CONTROLLER_H
#define AR_FLYING_GRAVIS_CONTROLLER_H

#include "arController.h"
#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arSZGClient.h"
#include "arFrameworkCalling.h"

SZG_CALL void ar_connectionFlyingGravisJoystick(void*);
SZG_CALL void ar_handleFlyingGravisJoystick(void*);

class SZG_CALL arFlyingGravisController:public arController{
  friend void ar_handleFlyingGravisJoystick(void*);
 public:
  arFlyingGravisController();
  ~arFlyingGravisController();

  bool init(arSZGClient&);
  bool start();

 private:
  arThread         _connectionThread;
  arThread         _transformThread;
  arInputNode      _joystickClient;
  arNetInputSource _netInputSource;
};

#endif
