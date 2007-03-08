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
#include "arFrameworkCalling.h"

#include <vector>
#include <map>

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

  virtual void draw();
  virtual void drawWithComposition();
  virtual void advance();

  // used to capture and process mouse/keyboard data
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( vector<unsigned>& mouseButtons );
  vector<unsigned> getMouseButtons() const;
  void setNumberButtonEvents( unsigned numButtonEvents ); 
  unsigned getNumberButtonEvents() const { return _numButtonEvents; }

 protected:
  bool _fInit;
  int _mouseXY[2];
  map< unsigned, int > _mouseButtons;
  unsigned _numButtonEvents;
  vector<char> _buttonLabels;
  
  float _rotWand[2];
  float _rotSim;

  // Cycle through subsets of buttons,
  // since mouse has fewer buttons than simulated wand.
  unsigned _buttonSelector;

  // Overall state of the simulator.
  arHeadWandSimState _interfaceState;

  // State of the simulated device. Two 4x4 matrices. 6 buttons. 2 axes.
  arMatrix4 _matrix[2]; // head then wand
  vector<int> _lastButtonEvents;
  vector<int> _newButtonEvents;
  float     _axis[2];

  // communicate with the registered arInputNode
  arGenericDriver _driver;

  void _wireCube(float size) const;
  void _drawGamepad() const;
  void _drawHead() const;
  void _drawWand() const;
  void _drawTextState() const;
};

#endif
