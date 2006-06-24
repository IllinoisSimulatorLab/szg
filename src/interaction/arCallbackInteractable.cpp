//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arCallbackInteractable.h"

arCallbackInteractable::arCallbackInteractable(int ID) :
  arInteractable(),
  _id(ID),
  _touchCallback(0),
  _processCallback(0),
  _untouchCallback(0),
  _matrixCallback(0) {
}

arCallbackInteractable::arCallbackInteractable( const arCallbackInteractable& bi ) :
  arInteractable( bi ),
  _id( bi._id ),
  _touchCallback( bi._touchCallback ),
  _processCallback( bi._processCallback ),
  _untouchCallback( bi._untouchCallback ),
  _matrixCallback( bi._matrixCallback ) {
}

arCallbackInteractable& arCallbackInteractable::operator=( const arCallbackInteractable& bi ) {
  if (&bi == this)
    return *this;
  arInteractable::operator=( bi );
  _id = bi._id;
  _touchCallback = bi._touchCallback;
  _processCallback = bi._processCallback;
  _untouchCallback = bi._untouchCallback;
  _matrixCallback = bi._matrixCallback;
  return *this;
}

bool arCallbackInteractable::_processInteraction( arEffector& effector ) {
  if (_processCallback != 0)
    return (_processCallback)( this, &effector );
  return true;
}

bool arCallbackInteractable::_touch( arEffector& effector ) {
  if (_touchCallback != 0)
    return (_touchCallback)( this, &effector );
  return true;
}

bool arCallbackInteractable::_untouch( arEffector& effector ) {
  if (_untouchCallback != 0)
    return (_untouchCallback)( this, &effector );
  return true;
}

void arCallbackInteractable::setMatrix( const arMatrix4& matrix ) {
  arInteractable::setMatrix( matrix );
  if (_matrixCallback != 0)
    (_matrixCallback)( this, matrix );
}
