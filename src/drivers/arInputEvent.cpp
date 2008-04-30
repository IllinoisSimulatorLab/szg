//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputEvent.h"

arInputEvent::arInputEvent() :
  _type( AR_EVENT_GARBAGE ),
  _index( 0 ),
  _matrix( NULL ) {
}

arInputEvent::arInputEvent( const arInputEventType type, const unsigned int index ) :
  _type( type ),
  _index( index ),
  _button( 0 ),
  _axis( 0. ),
  _matrix( NULL ) {
    if (_type == AR_EVENT_MATRIX)
      _matrix = new arMatrix4( ar_identityMatrix() );
}

arInputEvent::~arInputEvent() {
  if (_matrix)
    delete _matrix;
}

arInputEvent::arInputEvent( const arInputEvent& e ) :
  _type( e._type ),
  _index( e._index ),
  _button( e._button ),
  _axis( e._axis ),
  _matrix( NULL ) {
    if ( e._matrix )
      _matrix = new arMatrix4( e._matrix->v );
}

arInputEvent& arInputEvent::operator=( const arInputEvent& e ) {
  if (&e == this) {
    return *this;
  }
  _type = e._type;
  _index = e._index;
  _button = e._button;
  _axis = e._axis;
  if (_matrix) {
    delete _matrix;
  }
  _matrix = e._matrix ? (new arMatrix4( e._matrix->v )) : NULL;
  return *this;
}

int arInputEvent::getButton() const {
  if (_type != AR_EVENT_BUTTON)
    ar_log_error() << "arInputEvent getting button value from non-button event.\n";
  return _button;
}
float arInputEvent::getAxis() const {
  if (_type != AR_EVENT_AXIS)
    ar_log_error() << "arInputEvent getting axis value from non-axis event.\n";
  return _axis;
}
arMatrix4 arInputEvent::getMatrix() const {
  if (_type != AR_EVENT_MATRIX)
    ar_log_error() << "arInputEvent getting matrix value from non-matrix event.\n";
  return *_matrix;
}

bool arInputEvent::setButton( const unsigned int b ) {
  if (_type != AR_EVENT_BUTTON)
    return false;
  _button = b;
  return true;
}

bool arInputEvent::setAxis( const float a ) {
  if (_type != AR_EVENT_AXIS)
    return false;
  _axis = a;
  return true;
}

bool arInputEvent::setMatrix( const float* v ) {
  if (_type != AR_EVENT_MATRIX)
    return false;
  if (_matrix)
    memcpy( _matrix->v, v, 16*sizeof(float) );
  else
    _matrix = new arMatrix4( v );
  return true;
}

bool arInputEvent::setMatrix( const arMatrix4& m ) {
  if (_type != AR_EVENT_MATRIX)
    return false;
  if (_matrix)
    memcpy( _matrix->v, m.v, 16*sizeof(float) );
  else
    _matrix = new arMatrix4( m.v );
  return true;
}

void arInputEvent::trash() {
  _type = AR_EVENT_GARBAGE;
  if (_matrix) {
    delete _matrix;
    _matrix = NULL;
  }
}

void arInputEvent::zero() {
  switch (_type) {
    case AR_EVENT_BUTTON:
      setButton(0);
      break;
    case AR_EVENT_AXIS:
      setAxis(0.);
      break;
    case AR_EVENT_MATRIX:
      setMatrix(ar_identityMatrix());
      break;
    default:
      ar_log_error() << "arInputEvent can't zero while trashed.\n";
      break;
  }
}

// Protected constructors
arInputEvent::arInputEvent( const arInputEventType type,
                            const unsigned int index,
                            const int value ) :
  _type( type ),
  _index( index ),
  _button( value ),
  _matrix(0) {
}

arInputEvent::arInputEvent( const arInputEventType type,
                            const unsigned int index,
                            const float value ) :
  _type( type ),
  _index( index ),
  _axis( value ),
  _matrix(0) {
}

arInputEvent::arInputEvent( const arInputEventType type,
                            const unsigned int index,
                            const float* v ) :
  _type( type ),
  _index( index ),
  _matrix(new arMatrix4(v)) {
}

ostream& operator<<(ostream& s, const arInputEvent& event) {
  switch (event.getType()) {
    case AR_EVENT_BUTTON:
      s << "BUTTON[" << event.getIndex() << "]: " << event.getButton();
      break;
    case AR_EVENT_AXIS:
      s << "  AXIS[" << event.getIndex() << "]: " << event.getAxis();
        break;
    case AR_EVENT_MATRIX:
      s << "MATRIX[" << event.getIndex() << "]:\n" << event.getMatrix();
      break;
    case AR_EVENT_GARBAGE:
      s << "GARBAG[" << event.getIndex() << "]";
      break;
    default:
      s << "EV_ERR[" << event.getIndex() << "]";
  }
  return s;
}
