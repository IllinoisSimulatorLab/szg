//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputState.h"
#include "arLogStream.h"
#include "arSTLalgo.h"

void arInputState::_init() {
  _buttons.reserve(32);
  _axes.reserve(32);
  _matrices.reserve(16);
  _lastButtons.reserve(32);
}

arInputState::arInputState() :
  _l("INPUT_STATE") {
  _init();
}

arInputState::arInputState( const arInputState& x ) {
  arInputState& y = const_cast<arInputState&>(x);
  _init();
  arGuard _(y._l, "arInputState copy constructor");
  _buttons = y._buttons;
  _axes = y._axes;
  _matrices = y._matrices;
  _lastButtons = y._lastButtons;
  _buttonInputMap = y._buttonInputMap;
  _axisInputMap = y._axisInputMap;
  _matrixInputMap = y._matrixInputMap;
}

arInputState& arInputState::operator=( const arInputState& x ) {
  if (&x == this)
    return *this;
  arInputState& y = const_cast<arInputState&>(x);
  arGuard _(_l, "arInputState operator=");
  arGuard __(y._l);
  _buttons = y._buttons;
  _axes = y._axes;
  _matrices = y._matrices;
  _lastButtons = y._lastButtons;
  _buttonInputMap = y._buttonInputMap;
  _axisInputMap = y._axisInputMap;
  _matrixInputMap = y._matrixInputMap;
  return *this;
}

arInputState::~arInputState() {
  _buttons.clear();
  _axes.clear();
  _matrices.clear();
  _lastButtons.clear();
}

// Call-while-_unlocked public methods.

unsigned arInputState::getNumberButtons() const {
  arGuard _(_l, "arInputState::getNumberButtons");
  return _buttons.size();
}
unsigned arInputState::getNumberAxes() const {
  arGuard _(_l, "arInputState::getNumberAxes");
  return _axes.size();
}
unsigned arInputState::getNumberMatrices() const {
  arGuard _(_l, "arInputState::getNumberMatrices");
  return _matrices.size();
}

bool arInputState::getOnButton( const unsigned iButton ) const {
  arGuard _(_l, "arInputState::getOnButton");
  return _getOnButton(iButton);
}

bool arInputState::getOffButton( const unsigned iButton ) const {
  arGuard _(_l, "arInputState::getOffButton");
  return _getOffButton(iButton);
}

int arInputState::getButton( const unsigned iButton ) const {
  arGuard _(_l, "arInputState::getButton");
  return _getButton(iButton);
}

float arInputState::getAxis( const unsigned iAxis ) const {
  arGuard _(_l, "arInputState::getAxis");
  return _getAxis(iAxis);
}

arMatrix4 arInputState::getMatrix( const unsigned iMatrix ) const {
  arGuard _(_l, "arInputState::getMatrix");
  return _getMatrix(iMatrix);
}

bool arInputState::setButton( const unsigned iButton, const int value ) {
  arGuard _(_l, "arInputState::setButton");
  return _setButton(iButton, value);
}

bool arInputState::setAxis( const unsigned iAxis, const float value ) {
  arGuard _(_l, "arInputState::setAxis");
  return _setAxis(iAxis, value);
}

bool arInputState::setMatrix( const unsigned iMatrix, const arMatrix4& value) {
  arGuard _(_l, "arInputState::setMatrix");
  return _setMatrix(iMatrix, value);
}

// Mac OS X segfaults when constructors for global vars print to cout.
// The arEffector constructor does so, if including
// info about the signature.  So printing warnings is optional.
void arInputState::setSignature( const unsigned numButtons,
                                 const unsigned numAxes,
                                 const unsigned numMatrices,
                                 bool printWarnings) {
  arGuard _(_l, "arInputState::setSignature");
  _setSignature(numButtons, numAxes, numMatrices, printWarnings);
}


// Call-while-_locked private methods.

bool arInputState::_getOnButton( const unsigned iButton ) const {
    if ((iButton >= _buttons.size()) || (iButton >= _lastButtons.size()))
      return false;
    return (!_lastButtons[iButton]) && _buttons[iButton];
}

bool arInputState::_getOffButton( const unsigned iButton ) const {
  if ((iButton >= _buttons.size()) || (iButton >= _lastButtons.size()))
    return false;
  return _lastButtons[iButton] && !_buttons[iButton];
}

int arInputState::_getButton( const unsigned iButton ) const {
  return iButton < _buttons.size() ? _buttons[iButton] : 0;
}

float arInputState::_getAxis( const unsigned iAxis ) const {
  return iAxis < _axes.size() ? _axes[iAxis] : 0.;
}

arMatrix4 arInputState::_getMatrix( const unsigned iMatrix ) const {
  return iMatrix < _matrices.size() ?  _matrices[iMatrix] : arMatrix4();
}

// todo: factor out:
// if (x >= ~.size()) vector<> insert( ~.end(), x - ~.size()+1, 0)
// (Templatize _axes and _matrices, like arInputDeviceMap?)

bool arInputState::_setButton( const unsigned iButton, const int value ) {
  if (iButton >= _buttons.size()) {
    _buttons.insert( _buttons.end(), iButton - _buttons.size() + 1, 0 );
  }
  if (iButton >= _lastButtons.size()) {
    _lastButtons.insert( _lastButtons.end(), iButton - _lastButtons.size() + 1, 0 );
  }
  _lastButtons[iButton] = _buttons[iButton];
  _buttons[iButton] = value;
  return true;
}

bool arInputState::_setAxis( const unsigned iAxis, const float value ) {
  if (iAxis >= _axes.size()) {
    // ar_log_debug() << "arInputState grown to " << iAxis+1 << " axes.\n";
    _axes.insert( _axes.end(), iAxis+1 - _axes.size(), 0. );
  }
  _axes[iAxis] = value;
  return true;
}

bool arInputState::_setMatrix( const unsigned iMatrix, const arMatrix4& value ) {
  if (iMatrix >= _matrices.size()) {
    // ar_log_debug() << "arInputState grown to " << iMatrix+1 << " matrices.\n";
    _matrices.insert(_matrices.end(), iMatrix+1 - _matrices.size(), arMatrix4());
  }
  _matrices[iMatrix] = value;
  return true;
}

void arInputState::_setSignature( const unsigned numButtons,
    const unsigned numAxes, const unsigned numMatrices, const bool printWarnings ) {
  // It's possible to lose pending events if numXXX is decreased.
  bool changed = false;
  if (numButtons < _buttons.size()) {
    if (printWarnings) {
      ar_log_error() << "arInputState buttons reduced to " << numButtons << ".\n";
    }
    _buttons.erase( _buttons.begin()+numButtons, _buttons.end() );
    _lastButtons.erase( _lastButtons.begin()+numButtons, _lastButtons.end() );
    changed = true;
  } else if (numButtons > _buttons.size()) {
    _buttons.insert( _buttons.end(), numButtons-_buttons.size(), 0 );
    _lastButtons.insert( _lastButtons.end(), numButtons-_lastButtons.size(), 0 );
    changed = true;
  }

  if (numAxes < _axes.size()) {
    if (printWarnings) {
      ar_log_error() << "arInputState axes reduced to " << numAxes << ".\n";
    }
    _axes.erase( _axes.begin()+numAxes, _axes.end() );
    changed = true;
  } else if (numAxes > _axes.size()) {
    _axes.insert( _axes.end(), numAxes-_axes.size(), 0 );
    changed = true;
  }

  if (numMatrices < _matrices.size()) {
    if (printWarnings) {
      ar_log_error() << "arInputState matrices reduced to " << numMatrices << ".\n";
    }
    _matrices.erase( _matrices.begin()+numMatrices, _matrices.end() );
    changed = true;
  } else if (numMatrices > _matrices.size()) {
    _matrices.insert( _matrices.end(), numMatrices-_matrices.size(), arMatrix4() );
    changed = true;
  }

  if (changed && printWarnings) {
    ar_log_remark() << "arInputState signature (" << _buttons.size() << ", " <<
      _axes.size() << ", " << _matrices.size() << ").\n";
  }
}

bool arInputState::update( const arInputEvent& event ) {
  const int eventType = event.getType();
  switch (eventType) {
    case AR_EVENT_BUTTON:
      return setButton( event.getIndex(), event.getButton() );
    case AR_EVENT_AXIS:
      return setAxis( event.getIndex(), event.getAxis() );
    case AR_EVENT_MATRIX:
      return setMatrix( event.getIndex(), event.getMatrix() );
  }
  ar_log_error() << "arInputState ignoring invalid event type " << eventType << ".\n";
  return false;
}

void arInputState::addInputDevice( const unsigned numButtons,
                                   const unsigned numAxes,
                                   const unsigned numMatrices ) {
  arGuard _(_l, "arInputState::addInputDevice");
  const unsigned bPrev = _buttons.size();
  const unsigned aPrev = _axes.size();
  const unsigned mPrev = _matrices.size();
  _buttonInputMap.addInputDevice( numButtons, _buttons );
  _axisInputMap.addInputDevice( numAxes, _axes );
  _matrixInputMap.addInputDevice( numMatrices, _matrices );
  _setSignature( bPrev+numButtons, aPrev+numAxes, mPrev+numMatrices );
  ar_log_remark() << "arInputState added device " <<
    _buttonInputMap.getNumberDevices()-1 << " (" << numButtons << "," <<
    numAxes << "," << numMatrices << ").\n";
}

void arInputState::remapInputDevice( const unsigned iDevice,
                                     const unsigned numButtons,
                                     const unsigned numAxes,
                                     const unsigned numMatrices ) {
  arGuard _(_l, "arInputState::remapInputDevice");
  const int dbutton = numButtons - _buttonInputMap.getNumberDeviceEvents( iDevice );
  const int daxis = numAxes - _axisInputMap.getNumberDeviceEvents( iDevice );
  const int dmatrix = numMatrices - _matrixInputMap.getNumberDeviceEvents( iDevice );

  if (dbutton == 0 && daxis == 0 && dmatrix == 0) {
    // Signature didn't change.
    return;
  }

  if (dbutton < 0 && daxis < 0 && dmatrix < 0) {
    if (numButtons==0 && numAxes==0 && numMatrices==0)
      ar_log_warning() << "arInputState zeroing device " << iDevice << "\n";
    else
      ar_log_warning() << "arInputState shrinking device " << iDevice << "\n";
  }
  else {
    if (dbutton < 0)
      ar_log_warning() << "arInputState: fewer buttons for device " << iDevice << "\n";
    if (daxis < 0)
      ar_log_warning() << "arInputState: fewer axes for device " << iDevice << "\n";
    if (dmatrix < 0)
      ar_log_warning() << "arInputState: fewer matrices for device " << iDevice << "\n";
  }

  const unsigned oldButtons = _buttons.size();
  const unsigned oldAxes = _axes.size();
  const unsigned oldMatrices = _matrices.size();
  _buttonInputMap.remapInputEvents( iDevice, numButtons, _buttons );
  _axisInputMap.remapInputEvents( iDevice, numAxes, _axes );
  _matrixInputMap.remapInputEvents( iDevice, numMatrices, _matrices );
  _setSignature( oldButtons + dbutton, oldAxes + daxis, oldMatrices + dmatrix );
  ar_log_remark() << "arInputState device " << iDevice << " has signature (" << _buttons.size() << ", " <<
    _axes.size() << ", " << _matrices.size() << ").\n";
}

bool arInputState::getButtonOffset( unsigned iDevice, unsigned& offset ) {
  arGuard _(_l, "arInputState::getButtonOffset");
  return _buttonInputMap.getEventOffset( iDevice, offset );
}
bool arInputState::getAxisOffset(   unsigned iDevice, unsigned& offset ) {
  arGuard _(_l, "arInputState::getAxisOffset");
  return _axisInputMap.getEventOffset( iDevice, offset );
}
bool arInputState::getMatrixOffset( unsigned iDevice, unsigned& offset ) {
  arGuard _(_l, "arInputState::getMatrixOffset");
  return _matrixInputMap.getEventOffset( iDevice, offset );
}

bool arInputState::setFromBuffers( const int* const buttonData,
                                   const unsigned numButtons,
                                   const float* const axisData,
                                   const unsigned numAxes,
                                   const float* const matrixData,
                                   const unsigned numMatrices ) {
  if (!buttonData || !axisData || !matrixData) {
    ar_log_error() << "arInputState: null buffer.\n";
    return false;
  }

  arGuard _(_l, "arInputState::setFromBuffers");
  int i = 0;
  for (i=numButtons-1; i>=0; --i)
    _setButton( unsigned(i), buttonData[i] );
  for (i=numAxes-1; i>=0; --i)
    _setAxis( unsigned(i), axisData[i] );
  for (i=numMatrices-1; i>=0; --i)
    _setMatrix( unsigned(i), matrixData + i*16 );
  return true;
}

bool arInputState::saveToBuffers( int* const buttonBuf,
                                  float* const axisBuf,
                                  float* const matrixBuf ) const {
  if (!buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_error() << "arInputState: null buffer.\n";
    return false;
  }

  arGuard _(_l, "arInputState::saveToBuffers");
  unsigned i = 0;
  for (i=0; i<_buttons.size(); i++)
    buttonBuf[i] = _buttons[i];
  for (i=0; i<_axes.size(); i++)
    axisBuf[i] = _axes[i];
  for (i=0; i<_matrices.size(); i++)
    memcpy( matrixBuf + i*16, _matrices[i].v, 16*sizeof(float) );
  return true;
}

void arInputState::updateLastButtons() {
  arGuard _(_l, "arInputState::updateLastButtons");
  if (_lastButtons.size() == _buttons.size()) { // almost always true.
    std::copy( _buttons.begin(), _buttons.end(), _lastButtons.begin() );
  } else {
    _lastButtons.clear();
    std::copy( _buttons.begin(), _buttons.end(), std::back_inserter(_lastButtons) );
  }
}

void arInputState::updateLastButton( const unsigned i ) {
  arGuard _(_l, "arInputState::updateLastButton");
  if (i >= _buttons.size()) {
    ar_log_error() << "arInputState updateLastButton's index out of range (" << i << ").\n";
    return;
  }
  _lastButtons[i] = _buttons[i];
}

ostream& operator<<(ostream& os, const arInputState& cinp ) {
  const int cb = cinp.getNumberButtons();
  const int ca = cinp.getNumberAxes();
  const int cm = cinp.getNumberMatrices();
  os << "buttons, axes, matrices: " << cb << ", " << ca << ", " << cm << "\n";
  arInputState* inp = (arInputState*)&cinp;
  unsigned i = 0;
  if (cb > 0) {
    os << "buttons: ";
    for (i=0; i<inp->getNumberButtons(); i++)
      os << inp->getButton(i) << " ";
  }
  if (ca > 0) {
    os << "\naxes: ";
    for (i=0; i<inp->getNumberAxes(); i++)
      os << inp->getAxis(i) << " ";
  }
  if (cm > 0) {
    os << "\nmatrices:\n";
    for (i=0; i<inp->getNumberMatrices(); i++)
      os << inp->getMatrix(i) << "\n";
    os << "\n\n";
  }
  return os;
}
