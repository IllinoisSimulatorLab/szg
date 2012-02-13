
#include "arPrecompiled.h"
#include "arGluDisk.h"

arGluDisk::arGluDisk( double innerRadius, double outerRadius, int slices, int rings ) :
  arGluQuadric(),
  _innerRadius(innerRadius),
  _outerRadius(outerRadius),
  _slices(slices),
  _rings(rings) {
}

arGluDisk::arGluDisk( const arGluDisk& x ) :
  arGluQuadric(x),
  _innerRadius(x._innerRadius),
  _outerRadius(x._outerRadius),
  _slices(x._slices),
  _rings(x._rings) {
}

arGluDisk& arGluDisk::operator=( const arGluDisk& x ) {
  if (&x == this)
    return *this;
  arGluQuadric::operator=(x);
  _innerRadius = x._innerRadius;
  _outerRadius = x._outerRadius;
  _slices = x._slices;
  _rings = x._rings;
  return *this;
}

void arGluDisk::draw() {
  if (!_prepareQuadric())
    return;
  gluDisk( _quadric, _innerRadius, _outerRadius, _slices, _rings );
}

void arGluDisk::setRadii( double innerRadius, double outerRadius ) {
  _innerRadius = innerRadius;
  _outerRadius = outerRadius;
}

void arGluDisk::setSlicesRings( int slices, int rings ) {
  _slices = slices;
  _rings = rings;
}


