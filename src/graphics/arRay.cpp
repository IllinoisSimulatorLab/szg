//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arRay.h"

arBoundingSphere::arBoundingSphere(const arVector3& pos, float rad) :
  position(pos),
  radius(rad)
{
}

arBoundingSphere::arBoundingSphere(const arBoundingSphere& rhs){
  position = rhs.position;
  radius = rhs.radius;
  visibility = rhs.visibility;
}

// Somewhat inaccurate. Only works if the matrix maps spheres to spheres
// (for instance, all scaling must be uniform).
void arBoundingSphere::transform(const arMatrix4& m){
  position = m*position;
  // Assume that scaling is uniform!
  float factor = ++(m*arVector3(1,0,0) - m*arVector3(0,0,0));
  radius = radius*factor;
}

// There are three possibilities when trying to intersect two spheres.
// 1. They do not intersect.
// 2. They intersect but do not contain one another.
// 3. One sphere contains the other.
// The return value are correspondingly:
// -1: do not intersect, do not contain one another.
// >= 0: intersect or one sphere contains the other. Distance between centers.
float arBoundingSphere::intersect(const arBoundingSphere& b) const {
  const float dist = ++(b.position - position);
  return (dist > b.radius + radius) ? -1 : dist;
}

/// Given a camera viewing matrix, does this bounding sphere intersect?
bool arBoundingSphere::intersectViewFrustum(const arMatrix4& mArg) const {
  // hack: cast away constness, since arMatrix4.operator[] can't be const
  arMatrix4& m = (arMatrix4&) mArg;
  const arVector3 n1(m[0], m[4], m[8]);
  const arVector3 n2(m[1], m[5], m[9]);
  const arVector3 n3(m[2], m[6], m[10]);
  const arVector3 n4(m[3], m[7], m[11]);
  arVector3 temp = n1 - n4;
  // Check the frustum planes in turn. 
  if (temp%position + m[12] - m[15] > radius*temp.magnitude()){
    return false;
  }
  temp = -n1 - n4;
  if (temp%position - m[12] - m[15] > radius*temp.magnitude()){
    return false;
  }
  // Next 2.
  temp = n2 - n4;
  if (temp%position + m[13] - m[15] > radius*temp.magnitude()){
    return false;
  }
  temp = -n2 - n4;
  if (temp%position - m[13] - m[15] > radius*temp.magnitude()){
    return false;
  }
  // Next 2.
  temp = n3 - n4;
  if (temp%position + m[14] - m[15] > radius*temp.magnitude()){
    return false;
  }
  temp = -n3 - n4;
  if (temp%position - m[14] - m[15] > radius*temp.magnitude()){
    return false;
  }
  return true;
}

arRay::arRay(const arVector3& o, const arVector3& d) :
  origin(o),
  direction(d)
{
}

arRay::arRay(const arRay& rhs){
  origin = rhs.origin;
  direction= rhs.direction;
}

void arRay::transform(const arMatrix4& matrix){
  origin = matrix*origin;
  // Directions transform differently than positions.
  direction = matrix*direction - matrix*arVector3(0,0,0);
}

// Does the ray intersect a particular bounding sphere?
float arRay::intersect(float radius, const arVector3& position){
  const float a = direction%direction;
  const float b = 2. * (origin%direction - direction%position);
  const float c = origin%origin + position%position - 2*(origin%position)
            - radius*radius;

  const float discriminant = b*b - 4.*a*c;
  if (discriminant <= 0)
    return -1.;

  // possible intersection
  const float t1 = (-b + sqrt(discriminant))/(2.*a);
  const float t2 = (-b - sqrt(discriminant))/(2.*a);
  if (t1<0 && t2<0) {
    // the line intersects the sphere, but not in the positive direction
    return -1;
  }

  // Return the closest intersection in the positive direction.
  // Note that t2 <= t1.
  return (t2>0 ? t2 : t1) * ++direction;
}

