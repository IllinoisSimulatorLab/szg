//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FLYING_GRAVIS_CONTROLLER_H
#define AR_FLYING_GRAVIS_CONTROLLER_H 

#include "arController.h"
#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arSZGClient.h"

void ar_connectionFlyingGravisJoystick(void*);
void ar_handleFlyingGravisJoystick(void*);

/// Gravis Xterminator Digital Game Pad.

class arFlyingGravisController:public arController{
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
