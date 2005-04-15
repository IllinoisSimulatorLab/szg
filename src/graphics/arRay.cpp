//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arRay.h"

arRay::arRay(const arVector3& source, const arVector3& orientation) :
  _origin(source),
  _direction(orientation)
{
}

void arRay::transform(const arMatrix4& matrix){
  _origin = matrix*_origin;
  // Directions transform differently than positions.
  _direction = matrix*_direction - matrix*arVector3(0,0,0);
}

// does the ray intersect a particular bounding sphere?
float arRay::intersect(float radius, const arVector3& position){
  const float a = _direction%_direction;
  const float b = 2. * (_origin%_direction - _direction%position);
  const float c = _origin%_origin + position%position - 2*(_origin%position)
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
  return (t2>0 ? t2 : t1) * ++_direction;
}

arBoundingSphere::arBoundingSphere(const arVector3& pos, float rad) :
  position(pos),
  radius(rad)
{
}

/// Given a camera viewing matrix, does this bounding sphere intersect?
bool arBoundingSphere::intersectViewFrustum(arMatrix4& m){
  arVector3 n1(m[0], m[4], m[8]);
  arVector3 n2(m[1], m[5], m[9]);
  arVector3 n3(m[2], m[6], m[10]);
  arVector3 n4(m[3], m[7], m[11]);
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
