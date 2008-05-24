//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNavManager.h"
#include "arNavigationUtilities.h"
#include "arInteractionUtilities.h"

arNavInteractable::arNavInteractable() : arInteractable() {}

arNavInteractable::arNavInteractable( const arNavInteractable& ni ) :
  arInteractable( ni ) {
}

arNavInteractable& arNavInteractable::operator=( const arNavInteractable& ni ) {
  if (&ni == this)
    return *this;
  arInteractable::operator=( ni );
  return *this;
}

void arNavInteractable::setMatrix( const arMatrix4& matrix ) {
  arInteractable::setMatrix( matrix );
  ar_setNavMatrix( matrix );
}

arNavManager::arNavManager() :
  _effector(1,0,0,0,0,0,0),
  _navObject(),
  _transSpeeds(5,5,5),
  _rotSpeeds(0,0,0) {
  _effector.setInteractionSelector( arAlwaysInteractionSelector() );
  _navObject.useDefaultDrags( false );
}

arNavManager::arNavManager( const arNavManager& nm ) :
  _effector( nm._effector ),
  _navObject( nm._navObject ) {
  for (unsigned i=0; i<3; i++) {
    _transConditions[i] = nm._transConditions[i];
    _rotConditions[i] = nm._rotConditions[i];
  }
  _transSpeeds = nm._transSpeeds;
  _rotSpeeds = nm._rotSpeeds;
  _worldGrabCondition = nm._worldGrabCondition;
}

arNavManager& arNavManager::operator=( const arNavManager& nm ) {
  if (&nm == this)
    return *this;
  _effector = nm._effector;
  _navObject = nm._navObject;
  for (unsigned i=0; i<3; i++) {
    _transConditions[i] = nm._transConditions[i];
    _rotConditions[i] = nm._rotConditions[i];
  }
  _transSpeeds = nm._transSpeeds;
  _rotSpeeds = nm._rotSpeeds;
  _worldGrabCondition = nm._worldGrabCondition;
  return *this;
}

arNavManager::~arNavManager() {
}

bool arNavManager::setTransCondition( char axis,
                                      arInputEventType type,
                                      unsigned index,
                                      float threshold
				      ) {
  const int iAxis = axis - 'x';
  if (iAxis < 0 || iAxis > 2) {
    ar_log_error() << "arNavManager expected axis x y or z, not '" << axis << "'.\n";
    return false;
  }
  const arGrabCondition newCondition( type, index, threshold );
  _clearCondition( newCondition );
  if (_transConditions[iAxis].type() != AR_EVENT_GARBAGE)
    _navObject.deleteDrag( _transConditions[iAxis] );
  _navObject.setDrag( newCondition, arNavTransDrag( axis, _transSpeeds[iAxis] ) );
  _transConditions[iAxis] = newCondition;
  return true;
}

bool arNavManager::setRotCondition( char axis,
                                    arInputEventType type,
                                    unsigned index,
                                    float threshold ) {
  const int iAxis = axis - 'x';
  if (iAxis < 0 || iAxis > 2) {
    ar_log_error() << "arNavManager expected axis x y or z, not '" << axis << "'.\n";
    return false;
  }

  const arGrabCondition newCondition( type, index, threshold );
  _clearCondition( newCondition );
  if (_rotConditions[iAxis].type() != AR_EVENT_GARBAGE)
    _navObject.deleteDrag( _rotConditions[iAxis] );
  _navObject.setDrag( newCondition, arNavRotDrag( axis, _rotSpeeds[iAxis] ) );
  _rotConditions[iAxis] = newCondition;
  return true;
}

//bool arNavManager::setWorldRotGrabCondition( arInputEventType type,
//                                             unsigned index,
//                                             float threshold ) {
//  arGrabCondition newCondition( type, index, threshold );
//  _clearCondition( newCondition );
//  if (_worldGrabCondition.type() != AR_EVENT_GARBAGE)
//    _navObject.deleteDrag( _worldGrabCondition );
//  _navObject.setDrag( newCondition, arNavWorldRotDrag() );
//  _worldGrabCondition = newCondition;
//  return true;
//}

void arNavManager::setTransSpeed( float speed ) {
  for (unsigned i = 0; i<3; i++) {
    _transSpeeds[i] = speed;
    if (_transConditions[i].type() != AR_EVENT_GARBAGE) {
      _navObject.setDrag(
        _transConditions[i], arNavTransDrag( (char)(i+'x'), speed ) );
    }
  }   
}

void arNavManager::setRotSpeed( float degPerSec ) {
  for (unsigned i = 0; i<3; i++) {
    _rotSpeeds[i] = degPerSec;
    if (_rotConditions[i].type() != AR_EVENT_GARBAGE) {
      _navObject.setDrag(
	_rotConditions[i], arNavRotDrag( (char)(i+'x'), degPerSec ) );
    }
  }   
}

void arNavManager::setEffector( const arEffector& effector ) {
  _effector = effector;
}

void arNavManager::update( arInputState* inputState ) {
  _effector.updateState( inputState );
  _navObject._matrix = ar_getNavMatrix();
  ar_pollingInteraction( _effector, (arInteractable*)&_navObject );
}

void arNavManager::update( const arInputEvent& event ) {
  _effector.updateState( event );
  _navObject._matrix = ar_getNavMatrix();
  ar_pollingInteraction( _effector, (arInteractable*)&_navObject );
}

void arNavManager::_clearCondition( const arGrabCondition& condition ) {
  for (unsigned i = 0; i<3; i++) {
    if (_transConditions[i] == condition) {
      _navObject.deleteDrag( _transConditions[i] );
      _transConditions[i] = arGrabCondition();
    }
    if (_rotConditions[i] == condition) {
      _navObject.deleteDrag( _rotConditions[i] );
      _rotConditions[i] = arGrabCondition();
    }
  }
  if (_worldGrabCondition == condition) {
    _navObject.deleteDrag( _worldGrabCondition );
    _worldGrabCondition = arGrabCondition();
  }
}
