//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arInputState.h"
#include "arSTLalgo.h"

arInputState::arInputState() {
  // Grab some memory.
  _buttons.reserve(256);
  _axes.reserve(256);
  _matrices.reserve(32);
  _lastButtons.reserve(256);
  // init the mutex
  ar_mutex_init(&_accessLock);
}

arInputState::arInputState( const arInputState& x ) {
  arInputState& y = const_cast<arInputState&>(x);
  _buttons.reserve(256);
  _axes.reserve(256);
  _matrices.reserve(32);
  _lastButtons.reserve(256);
  ar_mutex_init(&_accessLock);
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
}

arInputState& arInputState::operator=( const arInputState& x ) {
  if (&x == this)
    return *this;
  arInputState& y = const_cast<arInputState&>(x);
  ar_mutex_init(&_accessLock);
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

void arInputState::_lock() {
  ar_mutex_lock( &_accessLock );
}

void arInputState::_unlock() {
  ar_mutex_unlock( &_accessLock );
}

int arInputState::_getButtonNoLock( const unsigned int buttonNumber ) const {
  return buttonNumber >= _buttons.size() ? 0 : _buttons[buttonNumber];
}

int arInputState::getButton( const unsigned int buttonNumber ){
  _lock();
  int result = _getButtonNoLock(buttonNumber);
  _unlock();
  return result;
}

float arInputState::_getAxisNoLock( const unsigned int axisNumber ) const {
  return axisNumber >= _axes.size() ? 0. : _axes[axisNumber];
}

float arInputState::getAxis( const unsigned int axisNumber ){
  _lock();
  float result = _getAxisNoLock(axisNumber);
  _unlock();
  return result;
}

arMatrix4 arInputState::_getMatrixNoLock( const unsigned int matrixNumber ) const {
  return matrixNumber >= _matrices.size() ?
    ar_identityMatrix() : _matrices[matrixNumber];
}

arMatrix4 arInputState::getMatrix( const unsigned int matrixNumber ){
  _lock();
  arMatrix4 result = _getMatrixNoLock(matrixNumber);
  _unlock();
  return result;
}

bool arInputState::_getOnButtonNoLock( const unsigned int buttonNumber ) const {
  if (buttonNumber >= _buttons.size())
    return false;
//  cerr << "#: " << buttonNumber << ", (" << _buttons[buttonNumber]
//       << "," << _lastButtons[buttonNumber] << ").\n";
  return _buttons[buttonNumber] && !_lastButtons[buttonNumber];
}

bool arInputState::getOnButton( const unsigned int buttonNumber ){
  _lock();
  const bool result = _getOnButtonNoLock(buttonNumber);
  _unlock();
  return result;
}

bool arInputState::_getOffButtonNoLock( const unsigned int buttonNumber ) const {
  if (buttonNumber >= _buttons.size())
    return false;
  return _lastButtons[buttonNumber] && !_buttons[buttonNumber];
}

bool arInputState::getOffButton( const unsigned int buttonNumber ){
  _lock();
  const bool result = _getOffButtonNoLock(buttonNumber);
  _unlock();
  return result;
}
  
bool arInputState::_setButtonNoLock( const unsigned int buttonNumber, 
                                     const int value ) {
  if (buttonNumber >= _buttons.size()) {
    //cout << "arInputState remark: growing button array to size "
    //     << buttonNumber+1 << endl;
    _buttons.insert( _buttons.end(), buttonNumber - _buttons.size() + 1, 0 );
    _lastButtons.insert( _lastButtons.end(), buttonNumber - _lastButtons.size() + 1, 0 );
  }
  _lastButtons[buttonNumber] = _buttons[buttonNumber];
  _buttons[buttonNumber] = value;
  return true;
}

bool arInputState::setButton( const unsigned int buttonNumber, 
                              const int value ) {
  _lock();
  const bool result = _setButtonNoLock(buttonNumber, value);
  _unlock();
  return result;
}

bool arInputState::_setAxisNoLock( const unsigned int axisNumber, 
                                   const float value ) {
  if (axisNumber >= _axes.size()) {
    //cout << "arInputState remark: growing axis array to size "
    //     << axisNumber+1 << endl;
    _axes.insert( _axes.end(), axisNumber - _axes.size() + 1, 0. );
  }
  _axes[axisNumber] = value;
  return true;
}

bool arInputState::setAxis( const unsigned int axisNumber, 
                            const float value ) {
  _lock();
  const bool result = _setAxisNoLock(axisNumber, value);
  _unlock();
  return result;
}

bool arInputState::_setMatrixNoLock( const unsigned int matrixNumber,
                                    const arMatrix4& value ) {
  if (matrixNumber >= _matrices.size()) {
    //cout << "arInputState remark: growing matrix array to size "
    //     << matrixNumber+1 << endl;
    _matrices.insert( _matrices.end(), matrixNumber - _matrices.size() + 1,
                      ar_identityMatrix() );
  }
  _matrices[matrixNumber] = value;
  return true;
}

bool arInputState::setMatrix( const unsigned int matrixNumber,
			      const arMatrix4& value){
  _lock();
  const bool result = _setMatrixNoLock(matrixNumber, value);
  _unlock();
  return result;
}

bool arInputState::update( const arInputEvent& event ) {
  // no locking is needed since the only access to internal storage is
  // through other, locked, methods
  const int eventType = event.getType();
  switch (eventType) {
    case AR_EVENT_BUTTON:
      return setButton( event.getIndex(), event.getButton() );
    case AR_EVENT_AXIS:
      return setAxis( event.getIndex(), event.getAxis() );
    case AR_EVENT_MATRIX:
      return setMatrix( event.getIndex(), event.getMatrix() );
  }
  cerr << "arInputState warning: ignoring invalid event type "
       << eventType << ".\n";
  return false;
}

// Mac OS X is upset by constructors for global vars that print things to
// cout (causes a segfault). The arEffector constructor (if including
// info about the signature) actually prints something via this statement.
// This causes cubes and atlantis to segfault on start! Consequently,
// printing the warning messages is optional and the default is not to do so.
void arInputState::setSignature( const unsigned int numButtons,
				 const unsigned int numAxes,
				 const unsigned int numMatrices,
                                 bool printWarnings){
  _lock();
  _setSignatureNoLock(numButtons, numAxes, numMatrices, printWarnings);
  _unlock();
}

void arInputState::_setSignatureNoLock( const unsigned int numButtons,
                                        const unsigned int numAxes,
                                        const unsigned int numMatrices,
                                        bool printWarnings ) {
  // It's possible to lose pending events if numXXX is decreased,
  // therefore we warn when this happens.
  bool changed = false;
  if (numButtons < _buttons.size()) {
    if (printWarnings){
      cerr << "arInputState warning: buttons reduced to "
           << numButtons << ".\n";
    }
    _buttons.erase( _buttons.begin()+numButtons, _buttons.end() );
    _lastButtons.erase( _lastButtons.begin()+numButtons, _lastButtons.end() );
    changed = true;
  } else if (numButtons > _buttons.size()) {
    _buttons.insert( _buttons.end(), numButtons-_buttons.size(), 0 );
    _lastButtons.insert( _lastButtons.end(), 
                         numButtons-_lastButtons.size(), 
                         0 );
    changed = true;
  }
    
  if (numAxes < _axes.size()) {
    if (printWarnings){
      cerr << "arInputState warning: axes reduced to "
           << numAxes << ".\n";
    }
    _axes.erase( _axes.begin()+numAxes, _axes.end() );
    changed = true;
  } else if (numAxes > _axes.size()) {
    _axes.insert( _axes.end(), numAxes-_axes.size(), 0 );
    changed = true;
  }
    
  if (numMatrices < _matrices.size()) {
    if (printWarnings){
      cerr << "arInputState warning: matrices reduced to "
           << numMatrices << ".\n";
    }
    _matrices.erase( _matrices.begin()+numMatrices, _matrices.end() );
    changed = true;
  } else if (numMatrices > _matrices.size()) {
    _matrices.insert( _matrices.end(), numMatrices-_matrices.size(),
                      ar_identityMatrix() );
    changed = true;
  }
  
  if (changed && printWarnings){
    cout << "arInputState remark: signature ("
         << _buttons.size() << "," << _axes.size() << "," << _matrices.size()
         << ").\n";
  }
}

void arInputState::addInputDevice( const unsigned int numButtons,
                                   const unsigned int numAxes,
                                   const unsigned int numMatrices ) {
  // IMPORTANT NOTE: we cannot chain some methods together because use
  // of locks would deadlock... For instance, below we must use
  // setSignatureNoLock instead of setSignature.
  _lock();
  unsigned int oldButtons = _buttons.size();
  unsigned int oldAxes = _axes.size();
  unsigned int oldMatrices = _matrices.size();
  _buttonInputMap.addInputDevice( numButtons, _buttons );
  _axisInputMap.addInputDevice( numAxes, _axes );
  _matrixInputMap.addInputDevice( numMatrices, _matrices );
  _setSignatureNoLock( oldButtons+numButtons, 
                      oldAxes+numAxes, 
                      oldMatrices+numMatrices );
  cout << "arInputState remark: added device "
       << _buttonInputMap.getNumberDevices()-1
       << " (" << numButtons << "," << numAxes << "," << numMatrices
       << ").\n";
  _unlock();
}
                         
void arInputState::remapInputDevice( const unsigned int deviceNum,
                                     const unsigned int numButtons,
                                     const unsigned int numAxes,
                                     const unsigned int numMatrices ) {
  // IMPORTANT NOTE: we cannot chain some methods together because use
  // of locks would deadlock... For instance, below we must use
  // setSignatureNoLock instead of setSignature.
  _lock();
  const int buttonDiff = numButtons -
    _buttonInputMap.getNumberDeviceEvents( deviceNum );
  const int axisDiff = numAxes -
    _axisInputMap.getNumberDeviceEvents( deviceNum );
  const int matrixDiff = numMatrices -
    _matrixInputMap.getNumberDeviceEvents( deviceNum );
  if ((buttonDiff != 0)||(axisDiff != 0)||(matrixDiff != 0)) {
    if (buttonDiff < 0)
      cerr << "arInputState warning: decreasing maximum button"
           << " event for device " << deviceNum << endl;
    if (axisDiff < 0)
      cerr << "arInputState warning: decreasing maximum axis"
           << " event for device " << deviceNum << endl;
    if (matrixDiff < 0)
      cerr << "arInputState warning: decreasing maximum matrix"
           << " event for device " << deviceNum << endl;
    //cerr << buttonDiff << " " << axisDiff << " " << matrixDiff << endl;
    unsigned int oldButtons = _buttons.size();
    unsigned int oldAxes = _axes.size();
    unsigned int oldMatrices = _matrices.size();
    _buttonInputMap.remapInputEvents( deviceNum, numButtons, _buttons );
    _axisInputMap.remapInputEvents( deviceNum, numAxes, _axes );
    _matrixInputMap.remapInputEvents( deviceNum, numMatrices, _matrices );
    _setSignatureNoLock( oldButtons+buttonDiff, 
                        oldAxes+axisDiff, 
                        oldMatrices+matrixDiff );
    //cout << "arInputState remark: signature ("
    //     << _buttons.size() << ", " << _axes.size() << ", " << _matrices.size()
    //     << ").\n";
  }
  _unlock();
}

bool arInputState::setFromBuffers( const int* const buttonData,
                                   const unsigned int numButtons,
                                   const float* const axisData,
                                   const unsigned int numAxes,
                                   const float* const matrixData,
                                   const unsigned int numMatrices ) {
  _lock();
  if (!buttonData || !axisData || !matrixData) {
    cerr << "arInputState warning: invalid buffer.\n";
    _unlock();
    return false;
  }

  unsigned int i = 0;
  for (i=0; i<numButtons; i++)
    _setButtonNoLock( i, buttonData[i] );
  for (i=0; i<numAxes; i++)
    _setAxisNoLock( i, axisData[i] );
  for (i=0; i<numMatrices; i++)
    _setMatrixNoLock( i, matrixData + i*16 );
  _unlock();
  return true;
}

bool arInputState::saveToBuffers( int* const buttonBuf,
                                  float* const axisBuf,
                                  float* const matrixBuf ){
  _lock();
  if (!buttonBuf || !axisBuf || !matrixBuf) {
    cerr << "arInputState warning: invalid buffer.\n";
    _unlock();
    return false;
  }

  unsigned int i = 0;
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

void arInputState::updateLastButton( const unsigned int index ) {
  _lock();
  if (index >= _buttons.size()) {
    _unlock();
    cerr << "arInputState error: index passed to updateButton() (" << index
         << ") out of range.\n";
    return;
  }
  _lastButtons[index] = _buttons[index];
  _unlock();
}

ostream& operator<<(ostream& os, const arInputState& cinp ) {
  arInputState* inp = (arInputState*)&cinp;
  os << "buttons: " << inp->getNumberButtons() << ", "
       << "axes: " << inp->getNumberAxes() << ", "
       << "matrices: " << inp->getNumberMatrices() << "\n";
  os << "buttons: ";
  unsigned int i = 0;
  for (i=0; i<inp->getNumberButtons(); i++){
    os << inp->getButton(i) << " ";
  }
  os << "\n";
  os << "axes: ";
  for (i=0; i<inp->getNumberAxes(); i++){
    os << inp->getAxis(i) << " ";
  }
  os << "\n";
  os << "matrices:\n";
  for (i=0; i<inp->getNumberMatrices(); i++){
    os << inp->getMatrix(i) << endl;
  }
  os << "\n";
  os << "*************************************************\n";
  return os;
}

