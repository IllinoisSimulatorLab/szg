//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGrabCondition.h"
#include "arEffector.h"

arGrabCondition::arGrabCondition() :
  _type( AR_EVENT_GARBAGE ),
  _index( 0 ),
  _threshold( 0. ),
  _currentValue( 0. ) {
}

arGrabCondition::arGrabCondition( arInputEventType eventType,
                                  unsigned int eventIndex,
                                  float thresholdValue ) :
  _type( eventType ),
  _index( eventIndex ),
  _currentValue( 0. ) {
  _threshold = thresholdValue;
  if ((_type != AR_EVENT_BUTTON)&&(_type != AR_EVENT_AXIS)) {
    cerr << "arGrabCondition warning: can only trigger off button or axis event;\n"
         << "   this arGrabCondition( " << _type << ", " << _index << ", "
         << _threshold << " ) will never be activated.\n";
  }
}

arGrabCondition::arGrabCondition( const arGrabCondition& g ) :
  _type( g._type ),
  _index( g._index ),
  _threshold( g._threshold ),
  _currentValue( g._currentValue ) {
}

arGrabCondition& arGrabCondition::operator=( const arGrabCondition& g ) {
  if (&g == this)
    return *this;
  _type = g._type;
  _index = g._index;
  _threshold = g._threshold;
  _currentValue = g._currentValue;
  return *this;
}

bool arGrabCondition::operator==( const arGrabCondition& g ) const {
  return ((_type == g._type)&&(_index == g._index));
}

bool arGrabCondition::check( arEffector* effector ) {
  if ((_type != AR_EVENT_BUTTON)&&(_type != AR_EVENT_AXIS)) {
    return false;
  }
  if (_type == AR_EVENT_BUTTON) {
    _currentValue = (float)effector->getButton( _index );
  } else { // axis
    _currentValue = effector->getAxis( _index );
  }
  return fabs( _currentValue ) >= _threshold;
}

arGrabCondition* arGrabCondition::copy() const {
  return new arGrabCondition( _type, _index, _threshold );
}

arDeltaGrabCondition::arDeltaGrabCondition() :
  arGrabCondition(),
  _currentState(false) {  
}

arDeltaGrabCondition::arDeltaGrabCondition( unsigned int eventIndex,
                                            bool on ) :
  arGrabCondition( AR_EVENT_BUTTON, eventIndex, 0.5 ),
  _isOnButtonEvent( on ),
  _currentState( !on ) {  
}

arDeltaGrabCondition::arDeltaGrabCondition( unsigned int eventIndex,
                                            bool on, bool current ) :
  arGrabCondition( AR_EVENT_BUTTON, eventIndex, 0.5 ),
  _isOnButtonEvent( on ),
  _currentState( current ) {  
}

arDeltaGrabCondition::arDeltaGrabCondition( const arDeltaGrabCondition& x ) :
  arGrabCondition(x),
  _isOnButtonEvent(x._isOnButtonEvent),
  _currentState(x._currentState) {  
}

arDeltaGrabCondition& arDeltaGrabCondition::operator=( const arDeltaGrabCondition& x ) {
  if (&x == this)
    return *this;
  arGrabCondition::operator=(x);
  _isOnButtonEvent = x._isOnButtonEvent;
  _currentState = x._currentState;
  return *this;
}

bool arDeltaGrabCondition::check( arEffector* effector ) {
  if (_type != AR_EVENT_BUTTON) {
    return false;
  }
  if (_isOnButtonEvent) {
    _currentValue = (float)effector->getOnButton( _index );
  } else {
    _currentValue = (float)effector->getOffButton( _index );
  }
  if (fabs( _currentValue ) >= _threshold) {
    _currentState = !_currentState;
    cerr << "arDeltaGrabCondition() remark: state = " << _currentState << endl;
  }
  return _currentState;
}

arGrabCondition* arDeltaGrabCondition::copy() const {
  return (arGrabCondition*)new arDeltaGrabCondition( _index, _isOnButtonEvent, _currentState );
}

