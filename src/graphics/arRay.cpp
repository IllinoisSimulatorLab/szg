//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arRay.h"
#include "arGraphicsHeader.h"


// arFrustumPlanes computations lifted from
// <http://www.lighthouse3d.com/opengl/viewfrustum/index.php>

arFrustumPlanes::arFrustumPlanes() {
  float buf[16];
  glGetFloatv( GL_PROJECTION_MATRIX, buf );
  const arMatrix4 projMatrix( buf );
  glGetFloatv( GL_MODELVIEW_MATRIX, buf );
  const arMatrix4 modMatrix( buf );
  const arMatrix4 matrix( projMatrix * modMatrix );
  _setFromMatrix( matrix );
}

arFrustumPlanes::arFrustumPlanes( const arMatrix4& matrix ) {
  _setFromMatrix( matrix );
}

arFrustumPlanes::arFrustumPlanes( const arFrustumPlanes& rhs ) {
  for (unsigned i=0; i<6; ++i) {
    normals[i] = rhs.normals[i];
  }
  memcpy( &D, &(rhs.D), 6*sizeof(float) );
}

void arFrustumPlanes::_setFromMatrix( const arMatrix4& matrix ) {
  const float* m = matrix.v;
  float m11 = m[0];
  float m21 = m[1];
  float m31 = m[2];
  float m41 = m[3];
  float m12 = m[4];
  float m22 = m[5];
  float m32 = m[6];
  float m42 = m[7];
  float m13 = m[8];
  float m23 = m[9];
  float m33 = m[10];
  float m43 = m[11];
  float m14 = m[12];
  float m24 = m[13];
  float m34 = m[14];
  float m44 = m[15];
  _setCoefficients( NEARP,  m31+m41,  m32+m42,  m33+m43,  m34+m44 );
  _setCoefficients( FARP,  -m31+m41, -m32+m42, -m33+m43, -m34+m44 );
  _setCoefficients( BOTTOM, m21+m41,  m22+m42,  m23+m43,  m24+m44 );
  _setCoefficients( TOP,   -m21+m41, -m22+m42, -m23+m43, -m24+m44 );
  _setCoefficients( LEFT,   m11+m41,  m12+m42,  m13+m43,  m14+m44 );
  _setCoefficients( RIGHT,  -m11+m41, -m12+m42, -m13+m43, -m14+m44 );
}

void arFrustumPlanes::_setCoefficients( PLANE_INDEX i, const float a, const float b, const float c, const float d ) {
  arVector3 N(a,b,c);
  float mag = N.magnitude();
  if (mag <= 0.) {
    cerr << "arFrustumPlanes::_setCoefficients() error: cannot normalize zero vector.\n";
    return;
  }
  mag = 1./mag;
  normals[i] = N*mag;
  D[i] = d*mag;
}


arBoundingSphere::arBoundingSphere(const arBoundingSphere& rhs) {
  radius = rhs.radius;
  visibility = rhs.visibility;
  position = rhs.position;
}

void arBoundingSphere::transform(const arMatrix4& m) {
  position = m * position;
  // Assume that scaling is uniform!
  const float factor = ++(m*arVector3(1, 0, 0) - m*arVector3(0, 0, 0));
  radius *= factor;
}

float arBoundingSphere::intersect(const arBoundingSphere& b) const {
  const float dist = ++(b.position - position);
  return (dist > b.radius + radius) ? -1 : dist;
}

// Given a set of frustum planes, does this bounding sphere intersect?
bool arBoundingSphere::intersectViewFrustum(const arFrustumPlanes& planes) const {
  // Check the frustum planes in turn.
  arVector3* N = const_cast<arVector3*>(planes.normals);
  float *D = const_cast<float*>(planes.D);
  for (unsigned i=0; i<6; ++i) {
    float distance = *D++ + N->dot( position );
    if (distance < -radius) {
      return false;
    }
    ++N;
  }
  return true;
}


// Given a camera viewing matrix, does this bounding sphere intersect?
bool arBoundingSphere::intersectViewFrustum(const arMatrix4& mArg) const {
  // hack: cast away constness, since arMatrix4.operator[] can't be const
  arMatrix4& m = (arMatrix4&) mArg;
  const arVector3 n1(m[0], m[4], m[8]);
  const arVector3 n2(m[1], m[5], m[9]);
  const arVector3 n3(m[2], m[6], m[10]);
  const arVector3 n4(m[3], m[7], m[11]);
  const float m14( m[12] );
  const float m24( m[13] );
  const float m34( m[14] );
  const float m44( m[15] );
  // Check the frustum planes in turn.
  arVector3 temp = n1 - n4;
  // Check the frustum planes in turn.
  if (temp.dot(position) + m14 - m44 > radius*temp.magnitude()) {
    return false;
  }
  temp = -n1 - n4;
  if (temp.dot(position) - m14 - m44 > radius*temp.magnitude()) {
    return false;
  }
  // Next 2.
  temp = n2 - n4;
  if (temp.dot(position) + m24 - m44 > radius*temp.magnitude()) {
    return false;
  }
  temp = -n2 - n4;
  if (temp.dot(position) - m24 - m44 > radius*temp.magnitude()) {
    return false;
  }
  // Next 2.
  temp = n3 - n4;
  if (temp.dot(position) + m34 - m44 > radius*temp.magnitude()) {
    return false;
  }
  temp = -n3 - n4;
  if (temp.dot(position) - m34 - m44 > radius*temp.magnitude()) {
    return false;
  }
  return true;
}

arRay::arRay(const arRay& rhs) {
  origin = rhs.origin;
  direction = rhs.direction;
}

void arRay::transform(const arMatrix4& matrix) {
  origin = matrix*origin;
  // Directions transform differently than positions.
  direction = matrix*direction - matrix*arVector3(0, 0, 0);
}

// Does the ray intersect a particular bounding sphere?
float arRay::intersect(float radius, const arVector3& position) {
  const float a = direction%direction;
  const float b = 2. * (origin%direction - direction%position);
  const float c = origin%origin + position%position - 2*(origin%position)
            - radius*radius;
  const float discriminant = b*b - 4.*a*c;
  if (discriminant <= 0)
    return -1.;

  const float t1 = (-b + sqrt(discriminant))/(2.*a);
  const float t2 = (-b - sqrt(discriminant))/(2.*a);
  if (t1<0. && t2<0.) {
    // The line intersects the sphere, but not in the positive direction.
    return -1;
  }

  // Return the closest intersection in the positive direction.  t2 <= t1.
  return (t2>0. ? t2 : t1) * ++direction;
}
