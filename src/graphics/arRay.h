//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_RAY_H
#define AR_RAY_H

#include "arMath.h"

// the classes in this file are related to geometric processing

/// Ray, for intersection testing.

class SZG_CALL arRay{
 public:
  arRay(){}
  arRay(const arVector3&, const arVector3&);
  ~arRay(){}

  void transform(const arMatrix4&);
  float intersect(float, const arVector3&); // compute intersection with sphere
    // return -1 if no intersection, otherwise return distance
  const arVector3& getOrigin() const
    { return _origin; }
  const arVector3& getDirection() const
    { return _direction; }
 private:
  arVector3 _origin;
  arVector3 _direction;
};

/// Bounding sphere.

class SZG_CALL arBoundingSphere{
 public:
  arBoundingSphere(){ radius = 0; visibility = false; }
  arBoundingSphere(const arVector3&, float);
  ~arBoundingSphere(){}

  arVector3 position;
  float     radius;
  bool      visibility;
};

#endif
