//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_RAY_H
#define AR_RAY_H

#include "arMath.h"
#include "arGraphicsCalling.h"

// The classes in this file are related to geometric processing.

// Bounding sphere.

class SZG_CALL arBoundingSphere{
 public:
  arBoundingSphere(){ radius = 0; visibility = false; }
  arBoundingSphere(const arVector3&, float);
  arBoundingSphere(const arBoundingSphere&);
  ~arBoundingSphere(){}

  // Somewhat inaccurate. Only works if the matrix maps spheres to spheres.
  void transform(const arMatrix4& m);
  // There are three possibilities when trying to intersect two spheres.
  // 1. They do not intersect.
  // 2. They intersect but do not contain one another.
  // 3. One sphere contains the other.
  // The return value are correspondingly:
  // -1: do not intersect, do not contain one another.
  // >= 0: intersect or one sphere contains the other. Distance between centers.
  float intersect(const arBoundingSphere&) const;
  bool intersectViewFrustum(const arMatrix4&) const;

  arVector3 position;
  float     radius;
  bool      visibility;
};

// Ray, for intersection testing.

class SZG_CALL arRay{
 public:
  arRay(){}
  arRay(const arVector3& o, const arVector3& d);
  arRay(const arRay&);
  ~arRay(){}

  void transform(const arMatrix4&);
  // Compute intersection with sphere.
  // Return -1 if no intersection, otherwise return distance
  float intersect(float radius, const arVector3& position);
  inline float intersect(const arBoundingSphere& b)
    { return intersect(b.radius, b.position); }
  // Not needed, but provided for backwards compatibility (the origin and
  // direction used to be private).
  const arVector3& getOrigin() const { return origin; }
  // Not needed, but provided for backwards compatibility (the origin and
  // direction used to be private).
  const arVector3& getDirection() const { return direction; }

  arVector3 origin;
  arVector3 direction;
};

#endif
