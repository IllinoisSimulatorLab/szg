//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_HEAD_WAND_SIMULATOR_H
#define AR_HEAD_WAND_SIMULATOR_H

#include "arSZGClient.h"
#include "arGenericDriver.h"
#include "arInputNode.h"
#include "arFrameworkObject.h"
#include <vector>
#include <map>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arFrameworkCalling.h"

// The simulated interface is modal. The states follow.
enum arHeadWandSimState{
  AR_SIM_HEAD_TRANSLATE = 0,
  AR_SIM_HEAD_ROTATE,
  AR_SIM_WAND_TRANSLATE,
  AR_SIM_WAND_TRANS_BUTTONS,
  AR_SIM_WAND_ROTATE_BUTTONS,
  AR_SIM_USE_JOYSTICK,
  AR_SIM_SIMULATOR_ROTATE
};

class SZG_CALL arInputSimulator: public arFrameworkObject{
 public:
  arInputSimulator();
  virtual ~arInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );
  void registerInputNode(arInputNode* node);

  virtual void draw() const;
  virtual void drawWithComposition();
  virtual void advance();

  // used to capture and process mouse/keyboard data
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( std::vector<unsigned>& mouseButtons );
  std::vector<unsigned> getMouseButtons();
  void setNumberButtonEvents( unsigned numButtonEvents ); 
  unsigned getNumberButtonEvents() const { return _numButtonEvents; }

 protected:
  // The physical input device (i.e. mouse)
  int _mousePosition[2];
  std::map< unsigned, int > _mouseButtons;
  unsigned _numButtonEvents;
  std::vector<char> _buttonLabels;
  
  // Virtual input device, driven by the mouse, rotates the wand.
  float _rotator[2];
  // Rotate the simulator display
  float _simulatorRotation;

  // There are more buttons on the simulated device than there are on the
  // mouse. We have 6 simulated buttons so far, which makes sense since we
  // tend to use gamepads w/ 6 DOF sensors attached for wands. At some point,
  // it would be a good idea to increase the number of simulated buttons to 9!
  // In any case, the "button selector" toggles between sets of buttons.
  unsigned _buttonSelector;

  // Overall state of the simulator.
  arHeadWandSimState _interfaceState;

  // State of the simulated device. Two 4x4 matrices. 6 buttons. 2 axes.
  arMatrix4 _matrix[2]; // head then wand
  std::vector<int> _lastButtonEvents;
  std::vector<int> _newButtonEvents;
  float     _axis[2];

  // Used to communicate with the registered arInputNode
  arGenericDriver _driver;

  // Used to draw the interface
  void _wireCube(float size) const;
  void _drawGamepad() const;
  void _drawHead() const;
  void _drawWand() const;
  void _drawTextState() const;
};

#endif
