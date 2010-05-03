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

// Modes.
enum arHeadWandSimState {
  AR_SIM_BOX_ROTATE = 0,
  AR_SIM_HEAD_TRANSLATE_HORIZONTAL,
  AR_SIM_HEAD_TRANSLATE_VERTICAL,
  AR_SIM_HEAD_ROTATE,
  AR_SIM_WAND_TRANSLATE_HORIZONTAL,
  AR_SIM_WAND_TRANSLATE_VERTICAL,
  AR_SIM_WAND_ROTATE,
  AR_SIM_USE_JOYSTICK,
  AR_SIM_HEAD_ROLL,
  AR_SIM_WAND_ROLL
};
// AR_SIM_BOX_ROLL deliberately unimplemented.

class SZG_CALL arMatrix4Factored {
 public:
  arMatrix4Factored(const arVector3& pos = arVector3(0, 0, 0)) :
    eulers(AR_YXZ),
    _pos(pos),
    _posReset(pos)
  {}

  void resetPosition() { _pos = _posReset; }
  void setPosition(const arVector3& v) { _pos = v; }
  void translate(const arVector3& v) { _pos += v; }
  void rotate(const float y, const float x, const float z)
    { eulers.addAngles(y, x, z); }
  void extractRotation( const arMatrix4& mat ) { eulers.extract(mat); }
  operator arMatrix4() const { return ar_TM(_pos) * eulers.toMatrix(); }

 protected:
  arEulerAngles eulers;

 private:
  arVector3 _pos;
  arVector3 _posReset;
};

class SZG_CALL arAziEle : public arMatrix4Factored {
 public:
  arAziEle(const arVector3& pos = arVector3(0, 0, 0)) :
    arMatrix4Factored(pos) {}

  void setAziEle(const float azi, const float ele) {
    arVector3 a(eulers);
    eulers.setAngles(azi, ele, a[2]);
  }
  void setRoll(const float roll) {
    arVector3 a(eulers);
    eulers.setAngles(a[0], a[1], roll);
  }
};

class SZG_CALL arInputSimulator: public arFrameworkObject {
 public:
  arInputSimulator();
  virtual ~arInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );
  void registerInputNode(arInputNode* node);

  virtual void draw();
  virtual void drawWithComposition();
  virtual void advance();

  // Mouse/keyboard input.
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( vector<unsigned>& mouseButtons );
  vector<unsigned> getMouseButtons() const;
  void setNumberButtonEvents( unsigned numButtonEvents );
  unsigned getNumberButtonEvents() const { return _numButtonEvents; }

 protected:
  bool _fInit;
  int _xMouse, _yMouse;
  unsigned _numButtonEvents;
  map< unsigned, int > _mouseButtons;
  vector<char> _buttonLabels;

  // Cycle the few mousebuttons through many wandbuttons.
  unsigned _buttonSelector;

  arHeadWandSimState _interfaceState;

  // Signature of the simulated headtracker+wand. 2 matrices, 2 axes, 6 buttons.
  arAziEle _head;
  arAziEle _wand;
  float _axis0, _axis1;
  vector<int> _lastButtonEvents;
  vector<int> _newButtonEvents;

  // Communicate with the registered arInputNode.
  arGenericDriver _driver;

  void _wireCube(const float size) const;
  void _drawGamepad() const;
  void _drawHead() const;
  void _drawWand() const;
  void _drawHint() const;

 private:
  arAziEle _box;
  bool _fDragNeedsButton;
  unsigned _numRows;
  unsigned _rowLength() const { return _mouseButtons.size(); }
  bool _updateButtonGrid();
};

#endif
