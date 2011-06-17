
#include "arPrecompiled.h"
#include "arGluCylinder.h"

arGluCylinder::arGluCylinder( double startRadius, double endRadius, double length, int slices, int stacks ) :
  arGluQuadric(),
  _startRadius(startRadius),
  _endRadius(endRadius),
  _length(length),
  _slices(slices),
  _stacks(stacks) {
}

arGluCylinder::arGluCylinder( const arGluCylinder& x ) :
  arGluQuadric(x),
  _startRadius(x._startRadius),
  _endRadius(x._endRadius),
  _length(x._length),
  _slices(x._slices),
  _stacks(x._stacks) {
}

arGluCylinder& arGluCylinder::operator=( const arGluCylinder& x ) {
  if (&x == this)
    return *this;
  arGluQuadric::operator=(x);
  _startRadius = x._startRadius;
  _endRadius = x._endRadius;
  _length = x._length;
  _slices = x._slices;
  _stacks = x._stacks;
  return *this;
}

void arGluCylinder::draw() {
  if (!_prepareQuadric())
    return;
  gluCylinder( _quadric, _startRadius, _endRadius, _length, _slices, _stacks );
}

void arGluCylinder::setRadii( double startRadius, double endRadius ) {
  _startRadius = startRadius;
  _endRadius = endRadius;
}

void arGluCylinder::setLength( double length ) {
  _length = length;
}

void arGluCylinder::setSlicesStacks( int slices, int stacks ) {
  _slices = slices;
  _stacks = stacks;
}

