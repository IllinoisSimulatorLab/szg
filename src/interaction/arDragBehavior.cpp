//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDragBehavior.h"
#include "arGrabCondition.h"
#include "arInteractable.h"
#include "arEffector.h"
#include "arNavigationUtilities.h"

arDragBehavior* arNullDragBehavior::copy() const {
  return (arDragBehavior*)new arNullDragBehavior();
}

void arCallbackDragBehavior::init( const arEffector* const effector,
                                   const arInteractable* const object ) {
  if (!_initCallback) {
    cerr << "arCallbackDragBehavior warning: no init callback.\n";
    return;
  }
  _initCallback( effector, object );
}

void arCallbackDragBehavior::update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const grabCondition ) {
  if (!_updateCallback) {
    cerr << "arCallbackDragBehavior warning: no update callback.\n";
    return;
  }
  _updateCallback( effector, object, grabCondition );
}

arDragBehavior* arCallbackDragBehavior::copy() const {
  return (arDragBehavior*)new arCallbackDragBehavior( _initCallback, _updateCallback );
}

arWandRelativeDrag::arWandRelativeDrag() :
  arDragBehavior(),
  _diffMatrix() {
}

arWandRelativeDrag::arWandRelativeDrag( const arWandRelativeDrag& wrd ) :
  arDragBehavior(),
  _diffMatrix( wrd._diffMatrix ) {
}

void arWandRelativeDrag::init( const arEffector* const effector,
                               const arInteractable* const object ) {
  _diffMatrix = effector->getMatrix().inverse() * object->getMatrix();
}

void arWandRelativeDrag::update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const /*grabCondition*/ ) {
  object->setMatrix( effector->getMatrix() * _diffMatrix );
}

arDragBehavior* arWandRelativeDrag::copy() const {
  return (arDragBehavior*)new arWandRelativeDrag( *this );
}

arWandTranslationDrag::arWandTranslationDrag( bool allowOffset ) :
  arDragBehavior(),
  _allowOffset( allowOffset ),
  _positionOffsetMatrix(),
  _objectOrientationMatrix() {
}

arWandTranslationDrag::arWandTranslationDrag( const arWandTranslationDrag& wtd ) :
  arDragBehavior(),
  _allowOffset( wtd._allowOffset ),
  _positionOffsetMatrix( wtd._positionOffsetMatrix ),
  _objectOrientationMatrix( wtd._objectOrientationMatrix ) {
}

void arWandTranslationDrag::init( const arEffector* const effector,
                                  const arInteractable* const object ) {
  const arMatrix4 effMatrix = effector->getMatrix();
  const arMatrix4 objMatrix = object->getMatrix();
  if (_allowOffset)
    _positionOffsetMatrix = ar_translationMatrix( ar_extractTranslation( objMatrix ) -
                                                ar_extractTranslation( effMatrix ) );
  else
    _positionOffsetMatrix = ar_identityMatrix();
  _objectOrientationMatrix = ar_extractRotationMatrix( objMatrix );
}

void arWandTranslationDrag::update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const /*grabCondition*/ ) {
  object->setMatrix( ar_extractTranslationMatrix( effector->getMatrix() ) *
                     _positionOffsetMatrix * _objectOrientationMatrix );
}

arDragBehavior* arWandTranslationDrag::copy() const {
  return (arDragBehavior*)new arWandTranslationDrag( *this );
}

arNavTransDrag::arNavTransDrag( const arVector3& displacement ) :
  _direction( displacement.normalize() ),
  _speed( displacement.magnitude() ) {
}

arNavTransDrag::arNavTransDrag( const arVector3& direction, const float speed ) :
  _direction( direction.normalize() ),
  _speed( speed ) {
}

arNavTransDrag::arNavTransDrag( const char axis, const float speed ) :
  _speed( speed ) {
  switch (axis) {
    case 'x': _direction = arVector3(1,0,0); break;
    case 'y': _direction = arVector3(0,1,0); break;
    // sign reversal, it's just more useful that way. OpenGL is brain-dead.
    case 'z': _direction = arVector3(0,0,-1); break;
    default:
      _direction = arVector3(0,0,0);
      cerr << "arNavTransDrag error: " << axis << " does not represent an axis.\n";
      break;
  }
}

arNavTransDrag::arNavTransDrag( const arNavTransDrag& ntd ) :
  arDragBehavior(),
  _direction( ntd._direction ),
  _speed( ntd._speed ),
  _lastTime( ntd._lastTime ) {
}

void arNavTransDrag::setSpeed( const float speed ) {
  _speed = speed;
}

void arNavTransDrag::setDirection( const arVector3& direction ) {
  _direction = direction.normalize();
}

void arNavTransDrag::init( const arEffector* const /*effector*/,
                           const arInteractable* const /*navigator*/ ) {
  _lastTime = ar_time();
}

void arNavTransDrag::update( const arEffector* const effector,
                         arInteractable* const navigator,
                         const arGrabCondition* const grabCondition ) {
  if (!grabCondition) {
    cerr << "arNavTransDrag error: NULL grabCondition pointer.\n";
    return;
  }
  float axisValue = grabCondition->value();
  if (axisValue > 1.)
    axisValue = 1.;
  else if (axisValue < -1.)
    axisValue = -1.;
  const arMatrix4 navMatrix = navigator->getMatrix();
  const arMatrix4 navTrans = ar_extractTranslationMatrix( navMatrix );
  const arMatrix4 navRot = ar_extractRotationMatrix( navMatrix );
  const arMatrix4 wandMatrix = effector->getMatrix();
  const arVector3 direction = ar_extractRotationMatrix(wandMatrix) * _direction;
  const ar_timeval currentTime = ar_time();
  const float amount = _speed * axisValue * ar_difftimeSafe( currentTime, _lastTime )/1.e6;
  _lastTime = currentTime;
  navigator->setMatrix( navTrans * ar_translationMatrix( direction*amount ) * navRot );
}

arDragBehavior* arNavTransDrag::copy() const {
  return (arDragBehavior*)new arNavTransDrag( *this );
}

arNavRotDrag::arNavRotDrag( const arVector3& axis, const float degreesPerSec ) :
  _axis( axis.normalize() ),
  _angleSpeed( ar_convertToRad( degreesPerSec ) ) {
}

arNavRotDrag::arNavRotDrag( const char axis, const float degreesPerSec ) :
  _angleSpeed( ar_convertToRad( degreesPerSec ) ) {
  switch (axis) {
    case 'x': _axis = arVector3(1,0,0); break;
    case 'y': _axis = arVector3(0,1,0); break;
    case 'z': _axis = arVector3(0,0,1); break;
    default:
      _axis = arVector3(0,0,1);
      cerr << "arNavTransDrag error: unknown axis '" << axis << "'.\n";
      break;
  }
}

arNavRotDrag::arNavRotDrag( const arNavRotDrag& nrd ) :
  arDragBehavior(),
  _axis( nrd._axis ),
  _angleSpeed( nrd._angleSpeed ),
  _lastTime( nrd._lastTime ) {
}

void arNavRotDrag::init( const arEffector* const /*effector*/,
                         const arInteractable* const /*navigator*/ ) {
  _lastTime = ar_time();
}

void arNavRotDrag::update( const arEffector* const /*effector*/,
                         arInteractable* const navigator,
                         const arGrabCondition* const grabCondition ) {
  if (grabCondition == 0) {
    cerr << "arNavRotDrag error: NULL grabCondition pointer.\n";
    return;
  }
  // get sign of [button,axis] event value
  float axisValue = grabCondition->value();
  if (axisValue > 1.)
    axisValue = 1.;
  else if (axisValue < -1.)
    axisValue = -1.;
  const ar_timeval currentTime = ar_time();
  const float angle = _angleSpeed * axisValue * ar_difftime( currentTime, _lastTime )/1.e6;
  _lastTime = currentTime;
  navigator->setMatrix( navigator->getMatrix() * ar_rotationMatrix( _axis, -angle ) );
}

arDragBehavior* arNavRotDrag::copy() const {
  return (arDragBehavior*)new arNavRotDrag( *this );
}

//arNavWorldRotDrag::arNavWorldRotDrag() :
//  arDragBehavior(),
//  _effMatrix(),
//  _navMatrix() {
//}
//
//arNavWorldRotDrag::arNavWorldRotDrag( const arNavWorldRotDrag& nwrd ) :
//  arDragBehavior(),
//  _effMatrix( nwrd._effMatrix ),
//  _navMatrix( nwrd._navMatrix ) {
//}
//
//void arNavWorldRotDrag::init( const arEffector* const effector,
//                              const arInteractable* const navigator ) {
//  _effMatrix = ar_extractRotationMatrix( effector->getInputMatrix() );
//  _navMatrix = navigator->getMatrix();
//}
//
//void arNavWorldRotDrag::update( const arEffector* const effector,
//                     arInteractable* const navigator,
//                     const arGrabCondition* const /*grabCondition*/ ) {
//  const arMatrix4 effMatrix = effector->getInputMatrix();
//  navigator->setMatrix( ar_extractRotationMatrix( effMatrix ).inverse() * _effMatrix * _navMatrix  ); // first drag fine, let go of button & drag again messes up axes
//}
//   
//arDragBehavior* arNavWorldRotDrag::copy() const {
//  return (arDragBehavior*)new arNavWorldRotDrag( *this );
//}

