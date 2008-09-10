//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_RAY_H
#define AR_RAY_H

#include "arMath.h"
#include "arGraphicsCalling.h"

// The classes in this file are related to geometric processing.


class arBoundingSphere;

class SZG_CALL arFrustumPlanes {
 public:
  arFrustumPlanes();
  arFrustumPlanes( const arMatrix4& matrix );
  arFrustumPlanes( const arFrustumPlanes& rhs );
  ~arFrustumPlanes() {}
  arVector3 normals[6];
  float D[6];

 private:
        enum PLANE_INDEX {
                TOP = 0,
                BOTTOM,
                LEFT,
                RIGHT,
                NEARP,
                FARP
        };
  void _setFromMatrix( const arMatrix4& matrix );
  void _setCoefficients( PLANE_INDEX i, const float a, const float b, const float c, const float d );
};
  
  

// Bounding sphere.
class SZG_CALL arBoundingSphere {
 public:
  arBoundingSphere(): radius(0), visibility(false) {}
  arBoundingSphere(const arVector3& pos, float rad) : radius(rad), position(pos) {}
  arBoundingSphere(const arBoundingSphere&);
  ~arBoundingSphere() {}

  // Assumes that m maps spheres to spheres (scaling is uniform).
  void transform(const arMatrix4& m);

  // Return -1 if the two spheres don't meet.
  // Otherwise (they meet, or one contains the other),
  // return the distance between their centers.
  float intersect(const arBoundingSphere&) const;

  bool intersectViewFrustum(const arMatrix4&) const;
  bool intersectViewFrustum(const arFrustumPlanes&) const;

  float     radius;
  bool      visibility;
  arVector3 position;
};

// Ray, for intersection testing.

class SZG_CALL arRay {
 public:
  arRay() {}
  arRay(const arVector3& o, const arVector3& d): origin(o), direction(d) {}
  arRay(const arRay&);
  ~arRay() {}

  void transform(const arMatrix4&);

  // Compute intersection with sphere.
  // Return -1 if no intersection, otherwise return distance
  float intersect(float radius, const arVector3& position);
  inline float intersect(const arBoundingSphere& b)
    { return intersect(b.radius, b.position); }

  // Deprecated.
  const arVector3& getOrigin() const { return origin; }
  const arVector3& getDirection() const { return direction; }

  arVector3 origin;
  arVector3 direction;
};

inline float ar_intersectRayTriangle(
  const arRay& r, const arVector3& v1, const arVector3& v2, const arVector3& v3)
  { return ar_intersectRayTriangle(r.getOrigin(), r.getDirection(), v1, v2, v3); }

#endif
