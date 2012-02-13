#include "arPrecompiled.h"
#include "arGluQuadric.h"
#include "arLogStream.h"

GLUquadricObj* arGluQuadric::_quadric = 0;
unsigned int arGluQuadric::_refCount = 0;

arGluQuadric::arGluQuadric() :
  _drawStyle(GLU_FILL),
  _normalDirection(GLU_OUTSIDE),
  _normalStyle(GLU_NONE) {
  ++_refCount;
  if (!_quadric) {
    _quadric = gluNewQuadric();
    if (!_quadric) {
      ar_log_error() << "arGluQuadric error: gluNewQuadric() failed.\n";
      return;
    }
//    gluQuadricCallback( _quadric, GLU_ERROR, (void (*)())arGluErrorCallback );
  }
}

arGluQuadric::arGluQuadric( const arGluQuadric& x ) :
  _drawStyle(x._drawStyle),
  _normalDirection(x._normalDirection),
  _normalStyle(x._normalStyle) {
  ++_refCount;
}

arGluQuadric& arGluQuadric::operator=( const arGluQuadric& x ) {
  if (&x == this)
    return *this;
  _drawStyle = x._drawStyle;
  _normalDirection = x._normalDirection;
  _normalStyle = x._normalStyle;
  return *this;
}

arGluQuadric::~arGluQuadric() {
  --_refCount;
  if (_refCount == 0) {
    if (_quadric) {
      gluDeleteQuadric( _quadric );
      _quadric = 0;
    }
  }
}

bool arGluQuadric::_prepareQuadric() {
  if (!_quadric)
    return false;
  gluQuadricDrawStyle( _quadric, _drawStyle );
  gluQuadricOrientation( _quadric, _normalDirection );
  gluQuadricNormals( _quadric, _normalStyle );
  gluQuadricTexture( _quadric, GL_TRUE );
  return true;
}


