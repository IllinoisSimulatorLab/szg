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

arInputState::arInputState() {
  _init();
}

arInputState::arInputState( const arInputState& x ) {
  arInputState& y = const_cast<arInputState&>(x);
  _init();
  y._lock();
    _buttons = y._buttons;
    _axes = y._axes;
    _matrices = y._matrices;
    _lastButtons = y._lastButtons;
    _buttonInputMap = y._buttonInputMap;
    _axisInputMap = y._axisInputMap;
    _matrixInputMap = y._matrixInputMap;
  y._unlock();
}

arInputState& arInputState::operator=( const arInputState& x ) {
  if (&x == this)
    return *this;
  arInputState& y = const_cast<arInputState&>(x);
  _lock();
  y._lock();
  _buttons = y._buttons;
  _axes = y._axes;
  _matrices = y._matrices;
  _lastButtons = y._lastButtons;
  _buttonInputMap = y._buttonInputMap;
  _axisInputMap = y._axisInputMap;
  _matrixInputMap = y._matrixInputMap;
  y._unlock();
  _unlock();
  return *this;
}

arInputState::~arInputState() {
  _buttons.clear();
  _axes.clear();
  _matrices.clear();
  _lastButtons.clear();
}

// Call-while-_unlock()'d public methods.

unsigned arInputState::getNumberButtons() {
  _lock();
  const unsigned result = _buttons.size();
  _unlock();
  return result;
}
unsigned arInputState::getNumberAxes() {
  _lock();
  const unsigned result = _axes.size();
  _unlock();
  return result;
}
unsigned arInputState::getNumberMatrices() {
  _lock();
  const unsigned result = _matrices.size();
  _unlock();
  return result;
}

bool arInputState::getOnButton( const unsigned iButton ) {
  _lock();
    const bool result = _getOnButton(iButton);
  _unlock();
  return result;
}

bool arInputState::getOffButton( const unsigned iButton ) {
  _lock();
    const bool result = _getOffButton(iButton);
  _unlock();
  return result;
}

int arInputState::getButton( const unsigned iButton ) {
  _lock();
    const int result = _getButton(iButton);
  _unlock();
  return result;
}

float arInputState::getAxis( const unsigned iAxis ) {
  _lock();
    const float result = _getAxis(iAxis);
  _unlock();
  return result;
}

arMatrix4 arInputState::getMatrix( const unsigned iMatrix ) {
  _lock();
    const arMatrix4 result = _getMatrix(iMatrix);
  _unlock();
  return result;
}

bool arInputState::setButton( const unsigned iButton, const int value ) {
  _lock();
    const bool ok = _setButton(iButton, value);
  _unlock();
  return ok;
}

bool arInputState::setAxis( const unsigned iAxis, const float value ) {
  _lock();
    const bool ok = _setAxis(iAxis, value);
  _unlock();
  return ok;
}

bool arInputState::setMatrix( const unsigned iMatrix, const arMatrix4& value) {
  _lock();
    const bool ok = _setMatrix(iMatrix, value);
  _unlock();
  return ok;
}

// Mac OS X segfaults when constructors for global vars print to cout.
// The arEffector constructor does so, if including
// info about the signature.  So printing warnings is optional.
void arInputState::setSignature( const unsigned numButtons,
				 const unsigned numAxes,
				 const unsigned numMatrices,
                                 bool printWarnings){
  _lock();
    _setSignature(numButtons, numAxes, numMatrices, printWarnings);
  _unlock();
}


// Call-while-_lock()'d private methods.

bool arInputState::_getOnButton( const unsigned iButton ) const {
  return (iButton < _buttons.size()) &&
    !_lastButtons[iButton] && _buttons[iButton];
}

bool arInputState::_getOffButton( const unsigned iButton ) const {
  return (iButton < _buttons.size()) &&
    _lastButtons[iButton] && !_buttons[iButton];
}

int arInputState::_getButton( const unsigned iButton ) const {
  return iButton < _buttons.size() ? _buttons[iButton] : 0;
}

float arInputState::_getAxis( const unsigned iAxis ) const {
  return iAxis < _axes.size() ? _axes[iAxis] : 0.;
}

arMatrix4 arInputState::_getMatrix( const unsigned iMatrix ) const {
  return iMatrix < _matrices.size() ?  _matrices[iMatrix] : ar_identityMatrix();
}

// todo: factor out:
// if (x >= ~.size()) vector<> insert( ~.end(), x - ~.size()+1, 0)
// (Templatize _axes and _matrices, like arInputDeviceMap?)

bool arInputState::_setButton( const unsigned iButton, const int value ) {
  if (iButton >= _buttons.size()) {
    _buttons.insert( _buttons.end(), iButton - _buttons.size() + 1, 0 );
  }
  if (iButton >= _lastButtons.size()){
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
    _matrices.insert(_matrices.end(), iMatrix+1 - _matrices.size(), ar_identityMatrix());
  }
  _matrices[iMatrix] = value;
  return true;
}

void arInputState::_setSignature( const unsigned numButtons,
    const unsigned numAxes, const unsigned numMatrices, const bool printWarnings ) {
  // It's possible to lose pending events if numXXX is decreased.
  bool changed = false;
  if (numButtons < _buttons.size()) {
    if (printWarnings){
      ar_log_warning() << "arInputState buttons reduced to " << numButtons << ".\n";
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
    if (printWarnings){
      ar_log_warning() << "arInputState axes reduced to " << numAxes << ".\n";
    }
    _axes.erase( _axes.begin()+numAxes, _axes.end() );
    changed = true;
  } else if (numAxes > _axes.size()) {
    _axes.insert( _axes.end(), numAxes-_axes.size(), 0 );
    changed = true;
  }

  if (numMatrices < _matrices.size()) {
    if (printWarnings){
      ar_log_warning() << "arInputState matrices reduced to " << numMatrices << ".\n";
    }
    _matrices.erase( _matrices.begin()+numMatrices, _matrices.end() );
    changed = true;
  } else if (numMatrices > _matrices.size()) {
    _matrices.insert( _matrices.end(), numMatrices-_matrices.size(),
                      ar_identityMatrix() );
    changed = true;
  }

  if (changed && printWarnings){
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
  ar_log_warning() << "arInputState ignoring invalid event type " << eventType << ".\n";
  return false;
}

void arInputState::addInputDevice( const unsigned numButtons,
                                   const unsigned numAxes,
                                   const unsigned numMatrices ) {
  _lock();
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
  _unlock();
}
                         
void arInputState::remapInputDevice( const unsigned iDevice,
                                     const unsigned numButtons,
                                     const unsigned numAxes,
                                     const unsigned numMatrices ) {
  _lock();
  const int dbutton = numButtons - _buttonInputMap.getNumberDeviceEvents( iDevice );
  const int daxis = numAxes - _axisInputMap.getNumberDeviceEvents( iDevice );
  const int dmatrix = numMatrices - _matrixInputMap.getNumberDeviceEvents( iDevice );

  if (dbutton == 0 && daxis == 0 && dmatrix == 0) {
    // Signature didn't change.
    _unlock();
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
  _unlock();
}

bool arInputState::getButtonOffset( unsigned iDevice, unsigned& offset ) {
  _lock();
    const bool ok = _buttonInputMap.getEventOffset( iDevice, offset );
  _unlock();
  return ok;
}
bool arInputState::getAxisOffset(   unsigned iDevice, unsigned& offset ) {
  _lock();
    const bool ok = _axisInputMap.getEventOffset( iDevice, offset );
  _unlock();
  return ok;
}
bool arInputState::getMatrixOffset( unsigned iDevice, unsigned& offset ) {
  _lock();
    const bool ok = _matrixInputMap.getEventOffset( iDevice, offset );
  _unlock();
  return ok;
}

bool arInputState::setFromBuffers( const int* const buttonData,
                                   const unsigned numButtons,
                                   const float* const axisData,
                                   const unsigned numAxes,
                                   const float* const matrixData,
                                   const unsigned numMatrices ) {
  if (!buttonData || !axisData || !matrixData) {
    ar_log_warning() << "arInputState: null buffer.\n";
    return false;
  }

  _lock();
  int i = 0;
  for (i=numButtons-1; i>=0; --i)
    _setButton( unsigned(i), buttonData[i] );
  for (i=numAxes-1; i>=0; --i)
    _setAxis( unsigned(i), axisData[i] );
  for (i=numMatrices-1; i>=0; --i)
    _setMatrix( unsigned(i), matrixData + i*16 );
  _unlock();
  return true;
}

bool arInputState::saveToBuffers( int* const buttonBuf,
                                  float* const axisBuf,
                                  float* const matrixBuf ){
  if (!buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_warning() << "arInputState: null buffer.\n";
    return false;
  }

  _lock();
  unsigned i = 0;
  for (i=0; i<_buttons.size(); i++)
    buttonBuf[i] = _buttons[i];
  for (i=0; i<_axes.size(); i++)
    axisBuf[i] = _axes[i];
  for (i=0; i<_matrices.size(); i++)
    memcpy( matrixBuf + i*16, _matrices[i].v, 16*sizeof(float) );
  _unlock();
  return true;
}

void arInputState::updateLastButtons() {
  _lock();
    std::copy( _buttons.begin(), _buttons.end(), _lastButtons.begin() );
  _unlock();
}

void arInputState::updateLastButton( const unsigned i ) {
  _lock();
  if (i >= _buttons.size()) {
    _unlock();
    ar_log_warning() << "arInputState updateLastButton's index out of range (" << i << ").\n";
    return;
  }
  _lastButtons[i] = _buttons[i];
  _unlock();
}

ostream& operator<<(ostream& os, arInputState& cinp ) {
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
