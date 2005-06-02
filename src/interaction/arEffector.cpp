//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arEffector.h"
#include "arInteractable.h"
#include "arNavigationUtilities.h"

arEffector::arEffector() :
  _inputState(),
  _matrix(),
  _centerMatrix(),
  _offsetMatrix(),
  _tipMatrix(),
  _matrixIndex( 0 ),
  _numButtons( 0 ),
  _loButton( 0 ),
  _buttonOffset( 0 ),
  _numAxes( 0 ),
  _loAxis( 0 ),
  _axisOffset( 0 ),
  _unitConversion(1.),
  _selector(NULL),
  _touchedObject(NULL),
  _grabbedObject(NULL),
  _dragManager(),
  _framework(NULL),
  _drawCallback(NULL) {
}

arEffector::arEffector( const unsigned int matrixIndex,
                        const unsigned int numButtons,
                        const unsigned int loButton,
                        const unsigned int numAxes,
                        const unsigned int loAxis ) :
  _inputState(),
  _matrix(),
  _centerMatrix(),
  _offsetMatrix(),
  _tipMatrix(),
  _matrixIndex( matrixIndex ),
  _numButtons( numButtons ),
  _loButton( loButton ),
  _buttonOffset( 0 ),
  _numAxes( numAxes ),
  _loAxis( loAxis ),
  _axisOffset( 0 ),
  _unitConversion(1.),
  _selector(NULL),
  _touchedObject(NULL),
  _grabbedObject(NULL),
  _dragManager(),
  _framework(NULL),
  _drawCallback(NULL) {
  _inputState.setSignature( _numButtons, _numAxes, 0 );
}

arEffector::arEffector( const unsigned int matrixIndex,
                        const unsigned int numButtons,
                        const unsigned int loButton,
                        const unsigned int buttonOffset,
                        const unsigned int numAxes,
                        const unsigned int loAxis,
                        const unsigned int axisOffset ) :
  _inputState(),
  _matrix(),
  _centerMatrix(),
  _offsetMatrix(),
  _tipMatrix(),
  _matrixIndex( matrixIndex ),
  _numButtons( numButtons ),
  _loButton( loButton ),
  _buttonOffset( buttonOffset ),
  _numAxes( numAxes ),
  _loAxis( loAxis ),
  _axisOffset( axisOffset ),
  _unitConversion(1.),
  _selector(NULL),
  _touchedObject(NULL),
  _grabbedObject(NULL),
  _dragManager(),
  _framework(NULL),
  _drawCallback(NULL) {
  _inputState.setSignature( _numButtons, _numAxes, 0 );
}

arEffector::arEffector( const arEffector& e ) :
  _inputState( e._inputState ),
  _matrix( e._matrix ),
  _centerMatrix( e._centerMatrix ),
  _offsetMatrix( e._offsetMatrix ),
  _tipMatrix( e._tipMatrix ),
  _matrixIndex( e._matrixIndex ),
  _numButtons( e._numButtons ),
  _loButton( e._loButton ),
  _buttonOffset( e._buttonOffset ),
  _numAxes( e._numAxes ),
  _loAxis( e._loAxis ),
  _axisOffset( e._axisOffset ),
  _unitConversion( e._unitConversion ),
  _touchedObject(NULL),
  _grabbedObject(NULL),
  _dragManager( e._dragManager ),
  _framework( e._framework ),
  _drawCallback( e._drawCallback ) {
}

arEffector& arEffector::operator=( const arEffector& e ) {
  if (&e == this)
    return *this;
  _inputState = e._inputState;
  _matrix = e._matrix;
  _centerMatrix = e._centerMatrix;
  _offsetMatrix = e._offsetMatrix;
  _tipMatrix = e._tipMatrix;
  _matrixIndex = e._matrixIndex;
  _numButtons = e._numButtons;
  _loButton = e._loButton;
  _buttonOffset = e._buttonOffset;
  _numAxes = e._numAxes;
  _loAxis = e._loAxis;
  _axisOffset = e._axisOffset;
  _unitConversion = e._unitConversion;
  if (_touchedObject)
    _touchedObject->untouch( *this ); // also unlocks if necessary
  _touchedObject = NULL;
  _grabbedObject = NULL;
  _dragManager = e._dragManager;
  _framework = e._framework;
  _drawCallback = e._drawCallback;
  return *this;
}

arEffector::~arEffector() {
  // this should also ungrab() if necessary
  if (_touchedObject)
    _touchedObject->untouch( *this );
}

void arEffector::setInteractionSelector( const arInteractionSelector& selector ) {
  _selector = selector.copy();
}

float arEffector::calcDistance( const arMatrix4& mat ) {
  if (_selector == NULL) {
    cerr << "arEffector error: NULL interaction selector.\n";
    return -1.;
  }
  return _selector->calcDistance( *this, mat );
}

void arEffector::setTipOffset( const arVector3& offset ) {
  _centerMatrix = ar_translationMatrix(.5*offset);
  _offsetMatrix = ar_translationMatrix(offset);
  _tipMatrix = _matrix * _offsetMatrix;
}

void arEffector::setMatrix( const arMatrix4& matrix ) {
  _inputMatrix = matrix;
  for (int i=12; i<15; i++)
    _inputMatrix.v[i] *= _unitConversion;
  _matrix = ar_matrixToNavCoords( _inputMatrix );
  _tipMatrix = _matrix * _offsetMatrix;
}

void arEffector::updateState( arInputEvent& event ) {
  int index = -1;
  switch (event.getType()) {
    case AR_EVENT_MATRIX:
      if (event.getIndex() != _matrixIndex)
        return;
      setMatrix( event.getMatrix() );
      break;
    case AR_EVENT_AXIS:
      index = event.getIndex()-_loAxis;
      if (index < 0 || index >= (int)_numAxes)
        return;
      _inputState.setAxis( index, event.getAxis() );
      break;
    case AR_EVENT_BUTTON:
      index = event.getIndex()-_loButton;
      if (index < 0 || index >= (int)_numButtons)
        return;
      _inputState.setButton( index, event.getButton() );
      break;
    default:
      cerr << "arEffector error: invalid event type.\n";
  }
}

void arEffector::updateState( arInputState* state ) {
#ifdef UNUSED
  const unsigned int numButs = _inputState.getNumberButtons();
  // Should this assign to _numButtons?
#endif
  unsigned int i=0,j=0;
  for (i=0,j=_loButton; i<_numButtons && j<state->getNumberButtons(); i++, j++)
    _inputState.setButton( i, state->getButton(j) );
  for (i=0,j=_loAxis; i<_numAxes && j<state->getNumberAxes(); i++, j++)
    _inputState.setAxis( i, state->getAxis(j) );
  setMatrix( state->getMatrix( _matrixIndex ) );
}

bool arEffector::requestGrab( arInteractable* grabee ) {
  if (_grabbedObject == grabee)
    return true;
  if (_grabbedObject)
    return false;
  if (_touchedObject != grabee) {
    if (_touchedObject)
      _touchedObject->untouch( *this );
    if (grabee)
      grabee->touch( *this );
  }
  _grabbedObject = grabee;
  return true;
}

void arEffector::requestUngrab( arInteractable* grabee ) {
  if (_grabbedObject == grabee)
    _grabbedObject = NULL;
}

void arEffector::forceUngrab() {
  if (_grabbedObject) {
    if (!_grabbedObject->untouch(*this)) {
      cerr << "arEffector warning: object didn't want to be ungrabbed.\n";
    }
    _grabbedObject = NULL;
  }
}

int arEffector::getButton( unsigned int index ){
  return _inputState.getButton( index - _buttonOffset );
}

float arEffector::getAxis( unsigned int index ){
  return _inputState.getAxis( index - _axisOffset );
}

bool arEffector::getOnButton( unsigned int index ){
  return _inputState.getOnButton( index - _buttonOffset );
}

bool arEffector::getOffButton( unsigned int index ){
  return _inputState.getOffButton( index - _buttonOffset );
}

arMatrix4 arEffector::getCenterMatrix() const {
  return _matrix * _centerMatrix;
}

void arEffector::setDrag( const arGrabCondition& cond,
                          const arDragBehavior& behave ) {
  _dragManager.setDrag( cond, behave );
}

void arEffector::deleteDrag( const arGrabCondition& cond ) {
  _dragManager.deleteDrag( cond );
}

const arDragManager* arEffector::getDragManager() const {
  return (const arDragManager*)&_dragManager;
}

const arInteractable* arEffector::getGrabbedObject() {
  if (_grabbedObject) {
    if (!_grabbedObject->enabled()) {
      _grabbedObject->untouch( *this );
      _grabbedObject = 0;
    }
  }
  return (const arInteractable*)_grabbedObject;
}

arInteractable* arEffector::getTouchedObject() {
  if (_touchedObject) {
    if (!_touchedObject->enabled()) {
      _touchedObject->untouch( *this );
      _touchedObject = 0;
    }
  }
  return _touchedObject;
}
