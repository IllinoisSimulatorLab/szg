//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_HEAD_WAND_SIMULATOR_H
#define AR_HEAD_WAND_SIMULATOR_H

#include "arSZGClient.h"
#include "arGenericDriver.h"
#include "arInputNode.h"
#include "arFrameworkObject.h"

// The simulated interface is modal. The states follow.
enum arHeadWandSimState{
  AR_SIM_EYE_TRANSLATE = 0,
  AR_SIM_ROTATE_VIEW,
  AR_SIM_WAND_TRANSLATE,
  AR_SIM_WAND_TRANS_BUTTONS,
  AR_SIM_WAND_ROTATE_BUTTONS,
  AR_SIM_JOYSTICK_MOVE,
  AR_SIM_WORLD_ROTATE
};

class SZG_CALL arHeadWandSimulator: public arFrameworkObject{
 public:
  arHeadWandSimulator();
  ~arHeadWandSimulator();

  void registerInputNode(arInputNode* node);

  virtual void draw();
  virtual void drawWithComposition();
  void advance();

  // used to capture and process mouse/keyboard data
  void keyboard(unsigned char key, int state, int x, int y);
  void mouseButton(int button, int state, int x, int y);
  void mousePosition(int x, int y);

 private:
  // The state of the simulator...
  // The physical input device (i.e. mouse)
  int _mousePosition[2];
  int _mouseButton[3];
  // A virtual input device, as driven by the mouse. Used to rotate the object
  // in 3D space.
  float _rotator[2];
  // We can rotate the simulator display
  float _worldRotation;
  // There are more buttons on the simulated device than there are on the
  // mouse. We have 6 simulated buttons so far, which makes sense since we
  // tend to use gamepads w/ 6 DOF sensors attached for wands. At some point,
  // it would be a good idea to increase the number of simulated buttons to 9!
  // In any case, the "button selector" toggles between sets of buttons.
  int _buttonSelector;
  // Overall state in which the simulator finds itself.
  arHeadWandSimState _interfaceState;

  // The state of the simulated device. Two 4x4 matrices. 6 buttons. 2 axes.
  // _matrix[0] = head matrix, _matrix[1] = wand matrix
  arMatrix4 _matrix[2];
  int       _button[6];
  int       _newButton[6];
  float     _axis[2];

  // Used to communicate with the registered arInputNode
  arGenericDriver _driver;


  // Used to draw the interface
  void _wireCube(float size);
  void _drawGamepad();
  void _drawHead();
  void _drawWand();
  void _drawTextState();
};

#endif
