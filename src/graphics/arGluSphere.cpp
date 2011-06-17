
#include "arPrecompiled.h"
#include "arGluSphere.h"

arGluSphere::arGluSphere( double radius, int slices, int stacks ) :
  arGluQuadric(),
  _radius(radius),
  _slices(slices),
  _stacks(stacks) {
}

arGluSphere::arGluSphere( const arGluSphere& x ) :
  arGluQuadric(x),
  _radius(x._radius),
  _slices(x._slices),
  _stacks(x._stacks) {
}

arGluSphere& arGluSphere::operator=( const arGluSphere& x ) {
  if (&x == this)
    return *this;
  arGluQuadric::operator=(x);
  _radius = x._radius;
  _slices = x._slices;
  _stacks = x._stacks;
  return *this;
}

void arGluSphere::draw() {
  if (!_prepareQuadric())
    return;
  gluSphere( _quadric, _radius, _slices, _stacks );
}

void arGluSphere::setRadius( double radius ) {
  _radius = radius;
}

void arGluSphere::setSlicesStacks( int slices, int stacks ) {
  _slices = slices;
  _stacks = stacks;
}

