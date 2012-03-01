//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATH_H
#define AR_MATH_H

#include "arLogStream.h"
#include "arMathCalling.h"

#ifdef AR_USE_LINUX
#include <string.h> // for memcpy etc, newer g++.
#endif

#include <iostream>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#ifndef M_PI
#define M_PI (3.14159265359)
#endif

#if defined(AR_USE_WIN_32) || defined(AR_USE_SGI)
inline float roundf(const float x) { return floor(x + 0.5f); }
#endif

// For Euler angles.
enum arAxisOrder {
        AR_XYZ        = 0x01,
        AR_XZY        = 0x00,
        AR_YZX        = 0x11,
        AR_YXZ        = 0x10,
        AR_ZXY        = 0x21,
        AR_ZYX        = 0x20
  };

enum arAxisName {
  AR_X_AXIS = 0,
  AR_Y_AXIS = 1,
  AR_Z_AXIS = 2
};

class arQuaternion;

// Vector of 2 points, e.g. a 2D texture coordinate.

class SZG_CALL arVector2 {
 public:
  float v[2];
  arVector2() { v[0] = v[1] = v[2] = 0.; }
  arVector2(float x, float y) { v[0] = x; v[1] = y; }
  ~arVector2() {}
  float& operator[] (int i) { return v[i]; }
  void set(const float* p)
    { memcpy(v, p, sizeof(v)); }
  void set(float x, float y)
    { v[0]=x; v[1]=y; }
  void get(float* p) const
    { memcpy(p, v, sizeof(v)); }

  const arVector2& operator+=(const arVector2& rhs)
    { v[0]+=rhs.v[0]; v[1]+=rhs.v[1]; return *this; }
  const arVector2& operator-=(const arVector2& rhs)
    { v[0]-=rhs.v[0]; v[1]-=rhs.v[1]; return *this; }
  const arVector2& operator*=(float scalar)
    { v[0]*=scalar; v[1]*=scalar; return *this; }
  const arVector2& operator/=(float scalar)
    { v[0]/=scalar; v[1]/=scalar; return *this; }
    // No protection from division by zero, for speed.
  bool operator==(const arVector2& rhs) const
    { return memcmp(v, rhs.v, 2*sizeof(float)) == 0; }
  bool operator!=(const arVector2& rhs) const
    { return memcmp(v, rhs.v, 2*sizeof(float)) != 0; }

  float magnitude2() const { return v[0]*v[0]+v[1]*v[1]; }
  float magnitude() const { return sqrt(magnitude2()); }
  arVector2 normalize() const {
    const float mag = magnitude();
    if (mag <= 0.) {
      cerr << "arVector2 error: cannot normalize zero.\n";
      return arVector2(0, 0);
    }
    return arVector2(v[0]/mag, v[1]/mag);
  }
  float dot( const arVector2& rhs ) const {
    return v[0]*rhs.v[0]+v[1]*rhs.v[1];
  }
};

// Vector of 3 points, e.g. position or direction in 3-space.

class SZG_CALL arVector3{
 public:
  float v[3];

  arVector3() { memset(v, 0, 3*sizeof(float)); }
  arVector3(const float* p)
    { set(p[0], p[1], p[2]); }
  arVector3(float x, float y, float z)
    { set(x, y, z); }
  ~arVector3() {}

  const arVector3& operator+=(const arVector3& rhs)
    { v[0]+=rhs.v[0]; v[1]+=rhs.v[1]; v[2]+=rhs.v[2]; return *this; }
  const arVector3& operator-=(const arVector3& rhs)
    { v[0]-=rhs.v[0]; v[1]-=rhs.v[1]; v[2]-=rhs.v[2]; return *this; }
  const arVector3& operator*=(float scalar)
    { v[0]*=scalar; v[1]*=scalar; v[2]*=scalar; return *this; }
  const arVector3& operator/=(float scalar)
    { v[0]/=scalar; v[1]/=scalar; v[2]/=scalar; return *this; }
    // No protection from division by zero, for speed.
  bool operator==(const arVector3& rhs) const
    { return memcmp(v, rhs.v, 3*sizeof(float)) == 0; }
  bool operator!=(const arVector3& rhs) const
    { return memcmp(v, rhs.v, 3*sizeof(float)) != 0; }

  // Do not define an operator cast to float*.
  // Do not define an operator that returns const float& from [].
  // These create ambiguity.
  float& operator[] (int i)
    { return v[i]; }

  void set(float x, float y, float z)
    { v[0]=x; v[1]=y; v[2]=z; }
  void set(const float* p)
    { memcpy(v, p, sizeof(v)); }
  void get(float* p) const
    { memcpy(p, v, sizeof(v)); }
  float magnitude2() const { return v[0]*v[0]+v[1]*v[1]+v[2]*v[2]; }
  float magnitude() const { return sqrt(magnitude2()); }
  arVector3 normalize() const {
    const float mag = magnitude();
    if (mag <= 0.) {
      cerr << "arVector3 error: cannot normalize zero.\n";
      return arVector3(0, 0, 0);
    }
    return arVector3(v[0]/mag, v[1]/mag, v[2]/mag);
  }
  bool zero() const { return magnitude2() < 1e-6; }
  float dot( const arVector3& rhs ) const {
    return v[0]*rhs.v[0]+v[1]*rhs.v[1]+v[2]*rhs.v[2];
  }
  const arVector3& round() {
      v[0] = roundf(v[0]);
      v[1] = roundf(v[1]);
      v[2] = roundf(v[2]);
      return *this;
    }
};

// Vector of 4 floats. A position in 4-space.

class arMatrix4;

class SZG_CALL arVector4{
 public:
  float v[4];

  arVector4() { memset(v, 0, 4*sizeof(float)); }
  arVector4(const float* p) { set(p[0], p[1], p[2], p[3]); }
  arVector4(float x, float y, float z, float w) { set(x, y, z, w); }
  arVector4(const arVector3& vec, float w) { set(vec.v[0], vec.v[1], vec.v[2], w); }
  bool operator==(const arVector4& rhs) const
    { return memcmp(v, rhs.v, 4*sizeof(float)) == 0; }
  bool operator!=(const arVector4& rhs) const
    { return memcmp(v, rhs.v, 4*sizeof(float)) != 0; }

  // Do not define an operator cast to float*.
  // Do not define an operator that returns const float& from [].
  // These create ambiguity.
  float& operator[] (int i)
    { return v[i]; }

  void set(float x, float y, float z, float w)
    { v[0]=x; v[1]=y; v[2]=z; v[3] = w;}
  void set(const float* p)
    { memcpy(v, p, sizeof(v)); }
  void get(float* p) const
    { memcpy(p, v, sizeof(v)); }
  float magnitude2() const
    { return v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3]; }
  float magnitude() const { return sqrt(magnitude2()); }
  float dot( const arVector4& y ) const {
    return v[0]*y.v[0]+v[1]*y.v[1]+v[2]*y.v[2]+v[3]*y.v[3];
  }
  arVector4 normalize() const {
    const float mag = magnitude();
    if (mag <= 0.) {
      cerr << "arVector4 error: cannot normalize zero.\n";
      return arVector4(0, 0, 0, 0);
    }
    return arVector4(v[0]/mag, v[1]/mag, v[2]/mag, v[3]/mag);
  }
  arMatrix4 outerProduct( const arVector4& rhs ) const;
  const arVector4& round() {
      v[0] = roundf(v[0]);
      v[1] = roundf(v[1]);
      v[2] = roundf(v[2]);
      v[3] = roundf(v[3]);
      return *this;
    }
};

// 4x4 matrix.  Typically an OpenGL transformation.

class SZG_CALL arMatrix4{
 public:
  arMatrix4();
  arMatrix4(const float* const);
  arMatrix4(float, float, float, float, float, float, float, float,
                  float, float, float, float, float, float, float, float);
  ~arMatrix4() {}

  arMatrix4 inverse() const;
  arMatrix4 transpose() const;

  operator arQuaternion() const;
  // DO NOT add an operator conversion to float*
  // DO NOT add an operator that returns const float& from []
  // these cause VC++ 6.0 to fail. Note how they make myMatrix4[1] ambiguous
  float& operator[](int i)
    { return v[i]; }
  bool operator==(const arMatrix4& rhs) const
    { return memcmp(v, rhs.v, 16*sizeof(float)) == 0; }
  bool operator!=(const arMatrix4& rhs) const
    { return memcmp(v, rhs.v, 16*sizeof(float)) != 0; }

  float v[16];
};

// Rotations.

class SZG_CALL arQuaternion{
 public:
  arQuaternion();
  arQuaternion(float real, float pure1, float pure2, float pure3);
  arQuaternion(float real, const arVector3& pure);
  arQuaternion( const float* numAddress );
  ~arQuaternion() {}

  float magnitude2() const {
    return real*real + pure.magnitude2();
  }
  float magnitude() const {
    return sqrt(magnitude2());
  }
  float dot(const arQuaternion& rhs) const {
    return real*rhs.real + pure.dot(rhs.pure);
  }
  bool operator==(const arQuaternion& rhs) const
    { return real==rhs.real && pure==rhs.pure; }
  bool operator!=(const arQuaternion& rhs) const
    { return real!=rhs.real || pure!=rhs.pure; }

  // Not inline, only because operators they use aren't yet declared.
  arQuaternion normalize() const;
  arQuaternion conjugate() const;
  arQuaternion inverse() const;

  operator arMatrix4() const;

  float real;
  arVector3 pure;
};

// Adapted from
// www.krugle.org/kse/files/svn/svn.sourceforge.net/neoengineng/neoengine/neoicexr/Imath/ImathEuler.h
class SZG_CALL arEulerAngles {
 public:
  // No implicit (default) angle order.  That's too subtle and dangerous.
  arEulerAngles( const arAxisOrder& ord, const arVector3& angs=arVector3() );
  ~arEulerAngles() {}
  void setOrder( const arAxisOrder& ord );
  void setAngles( const arVector3& ang ) { _angles  = ang; }
  void addAngles( const arVector3& ang ) { _angles += ang; }
  void setAngles( const float x, const float y, const float z ) { _angles  = arVector3(x, y, z); }
  void addAngles( const float x, const float y, const float z ) { _angles += arVector3(x, y, z); }
  arAxisOrder getOrder() const;
  void angleOrder( arAxisName& i, arAxisName& j, arAxisName& k ) const;
  arVector3 extract( const arMatrix4& mat );
  arMatrix4 toMatrix() const;
  operator arVector3() const { return _angles; }
 private:
  arVector3 _angles;
  arAxisName _initialAxis;
  bool _parityEven;
};

//***************************************************************
// function prototypes... most are inlined below
//***************************************************************

// *********** vector ******************
SZG_CALL arVector2 operator*(float, const arVector2&);
SZG_CALL arVector2 operator*(const arVector2&, float);
SZG_CALL arVector2 operator/(const arVector2&, float); // scalar division, handles /0
SZG_CALL arVector2 operator+(const arVector2&, const arVector2&);
SZG_CALL arVector2 operator-(const arVector2&); // negation
SZG_CALL arVector2 operator-(const arVector2&, const arVector2&);
// cross product
SZG_CALL arVector3 operator*(const arVector3&, const arVector3&);
SZG_CALL arVector3 operator*(float, const arVector3&);
SZG_CALL arVector3 operator*(const arVector3&, float);
SZG_CALL arVector3 operator/(const arVector3&, float); // scalar division, handles /0
SZG_CALL arVector3 operator+(const arVector3&, const arVector3&);
SZG_CALL arVector3 operator-(const arVector3&); // negation
SZG_CALL arVector3 operator-(const arVector3&, const arVector3&);
SZG_CALL float operator%(const arVector3&, const arVector3&); // dot product
SZG_CALL float operator++(const arVector3&); // magnitude
SZG_CALL ostream& operator<<(ostream&, const arVector2&);
SZG_CALL ostream& operator<<(ostream&, const arVector3&);
SZG_CALL ostream& operator<<(ostream&, const arVector4&);
SZG_CALL arLogStream& operator<<(arLogStream&, const arVector2&);
SZG_CALL arLogStream& operator<<(arLogStream&, const arVector3&);
SZG_CALL arLogStream& operator<<(arLogStream&, const arVector4&);

//************ matrix *******************
SZG_CALL arMatrix4 operator*(const arMatrix4&, const arMatrix4&);
SZG_CALL arMatrix4 operator+(const arMatrix4&, const arMatrix4&);
SZG_CALL arMatrix4 operator-(const arMatrix4&); //negate
SZG_CALL arMatrix4 operator-(const arMatrix4&, const arMatrix4&);
SZG_CALL arMatrix4 operator!(const arMatrix4&); //inverse; all zeros if singular
SZG_CALL istream& operator>>(istream&, arMatrix4&);
SZG_CALL ostream& operator<<(ostream&, const arMatrix4&);
SZG_CALL arLogStream& operator<<(arLogStream&, const arMatrix4&);

//************* quaternion ***************
SZG_CALL arQuaternion operator*(const arQuaternion&, const arQuaternion&);
SZG_CALL arQuaternion operator*(float, const arQuaternion&);
SZG_CALL arQuaternion operator*(const arQuaternion&, float);
SZG_CALL arQuaternion operator/(const arQuaternion&, float); // scalar division; all zeros if /0
SZG_CALL arQuaternion operator+(const arQuaternion&, const arQuaternion&);
SZG_CALL arQuaternion operator-(const arQuaternion&);
SZG_CALL arQuaternion operator-(const arQuaternion&, const arQuaternion&);
SZG_CALL float operator++(const arQuaternion&);   // magnitude
SZG_CALL ostream& operator<<(ostream&, const arQuaternion&);
SZG_CALL arLogStream& operator<<(arLogStream&, const arQuaternion&);

//************* misc **********************
// transform vector
SZG_CALL arVector3 operator*(const arMatrix4&, const arVector3&);
SZG_CALL arVector3 operator*(const arQuaternion&, const arVector3&);

//SZG_CALL arVector4 operator*(const arMatrix4&, const arVector4&);

//********* utility functions ******

SZG_CALL bool ar_isPowerOfTwo( int x );

// Matrix creation.
SZG_CALL arMatrix4 ar_translationMatrix(float, float, float);
SZG_CALL arMatrix4 ar_translationMatrix(const arVector3&);
SZG_CALL arMatrix4 ar_rotationMatrix(char, float);
SZG_CALL arMatrix4 ar_rotationMatrix(arAxisName, float);
SZG_CALL arMatrix4 ar_rotationMatrix(const arVector3&, float);
SZG_CALL arMatrix4 ar_rotateVectorToVector( const arVector3& vec1,
                                            const arVector3& vec2 );
SZG_CALL arMatrix4 ar_transrotMatrix(const arVector3& position,
                                     const arQuaternion& orientation);
SZG_CALL arMatrix4 ar_scaleMatrix(float);
SZG_CALL arMatrix4 ar_scaleMatrix(float, float, float);
SZG_CALL arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors);
SZG_CALL arMatrix4 ar_identityMatrix();

// Abbreviations.
inline arMatrix4 ar_TM(float x, float y, float z) {
  return ar_translationMatrix(x, y, z);
}
inline arMatrix4 ar_TM(const arVector3& v) {
  return ar_translationMatrix(v);
}
inline arMatrix4 ar_RM(char axis, float angle) {
  return ar_rotationMatrix(axis, angle);
}
inline arMatrix4 ar_RM(const arVector3& axis, float angle) {
  return ar_rotationMatrix(axis, angle);
}
inline arMatrix4 ar_SM(float scale) {
  return ar_scaleMatrix(scale);
}
inline arMatrix4 ar_SM(float a, float b, float c) {
  return ar_scaleMatrix(a, b, c);
}
inline arMatrix4 ar_SM(const arVector3& scaleFactors) {
  return ar_scaleMatrix(scaleFactors);
}

// Factor products of rotations, translations and uniform scalings.
SZG_CALL arMatrix4 ar_extractTranslationMatrix(const arMatrix4&);
SZG_CALL arVector3 ar_extractTranslation(const arMatrix4&);
SZG_CALL arMatrix4 ar_extractRotationMatrix(const arMatrix4&);
SZG_CALL arMatrix4 ar_extractScaleMatrix(const arMatrix4&);

// Abbreviations.
inline arMatrix4 ar_ETM(const arMatrix4& m) {
  return ar_extractTranslationMatrix(m);
}
inline arVector3 ar_ET(const arMatrix4& m) {
  return ar_extractTranslation(m);
}
inline arMatrix4 ar_ERM(const arMatrix4& m) {
  return ar_extractRotationMatrix(m);
}
inline arMatrix4 ar_ESM(const arMatrix4& m) {
  return ar_extractScaleMatrix(m);
}

// Utility functions.
SZG_CALL float     ar_angleBetween(const arVector3&, const arVector3&);
SZG_CALL arVector3 ar_extractEulerAngles(const arMatrix4& m, arAxisOrder o);
SZG_CALL arQuaternion ar_angleVectorToQuaternion(const arVector3&, float);
// returns the relected vector of direction across normal.
SZG_CALL arVector3 ar_reflect(const arVector3& direction, const arVector3& normal);
SZG_CALL float ar_intersectRayTriangle(const arVector3& rayOrigin,
                                       const arVector3& rayDirection,
                                       const arVector3& v1,
                                       const arVector3& v2,
                                       const arVector3& v3);

// One more abbreviation, analogous to ar_ET but after ar_extractEulerAngles
// in this .h file.
inline arVector3 ar_ER(const arMatrix4& m, arAxisOrder o) {
  return ar_extractEulerAngles(m, o);
}

// lineDirection & planeNormal need not be normalized (but affects interpretation of range).
// Returns false if no intersection.
// Otherwise, intersection = linePoint + range*lineDirection.
SZG_CALL bool ar_intersectLinePlane( const arVector3& linePoint,
                                     const arVector3& lineDirection,
                                     const arVector3& planePoint,
                                     const arVector3& planeNormal,
                                     float& range );

// Point on a line that is nearest to otherPoint.
SZG_CALL arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                          const arVector3& lineDirection,
                                          const arVector3& otherPoint,
                                          const float threshold = 1e-6 );

// Matrix for reflecting in a plane. This matrix should pre-multiply
// the object matrix on the stack (i.e. load this one on first, then multiply
// by the object placement matrix).
SZG_CALL arMatrix4 ar_mirrorMatrix( const arVector3& planePoint, const arVector3& planeNormal );

// Matrix to project a set of vertices onto a plane (for cast shadows)
// This matrix should be post-multiplied on the top of the stack.
// How to use: specify all parameters in top-level coordinates (i.e. the
// coordinates that the object placement matrix objectMatrix are specified in).
// Render your object normally.
// Then glMultMatrixf() by the ar_castShadowMatrix() matrix. Either disable the depth
// test or set the plane position just slightly in front of the actual plane.
// Set color to black, disable lighting and texture. For the most realism
// enable blending with the shadow's alpha set to .3 or so, but this will
// backfire if you've got more than one shadow and they overlap (I think
// you can fix that with the stencil buffer). Then redraw the object.
SZG_CALL arMatrix4 ar_castShadowMatrix( const arMatrix4& objectMatrix,
                                        const arVector4& lightPosition,
                                        const arVector3& planePoint,
                                        const arVector3& planeNormal );


//********** user interface, for mouse->3D

//UNUSED SZG_CALL arMatrix4 ar_planeToRotation(float, float);

//********** screen

SZG_CALL arVector3 ar_tileScreenOffset(const arVector3&,
                                       const arVector3&,
                                       float, float, float, int, float, int );
SZG_CALL arMatrix4 ar_frustumMatrix( const arVector3& screenCenter,
                                     const arVector3& screenNormal,
                                     const arVector3& screenUp,
                                     const float halfWidth,
                                     const float halfHeight,
                                     const float nearClip, const float farClip,
                                     const arVector3& eyePosition );
SZG_CALL arMatrix4 ar_frustumMatrix( const float screenDist,
                                     const float halfWidth,
                                     const float halfHeight,
                                     const float nearClip, const float farClip,
                                     const arVector3& locEyePosition );
SZG_CALL arMatrix4 ar_lookatMatrix( const arVector3& viewPosition,
                                    const arVector3& lookatPosition,
                                    const arVector3& up );

//*************************************
// vector inline
//*************************************

// scalar multiply
// Should also define operator*=
inline arVector2 operator*(float c, const arVector2& x) {
  return arVector2(c*x.v[0], c*x.v[1]);
}
// Should also define operator*=
inline arVector2 operator*(const arVector2& x, float c) {
  return c * x;
}

// scalar division, returns all zeros if scalar zero
// Should also define operator/=
inline arVector2 operator/(const arVector2& x, float c) {
  return (c==0) ?
    arVector2(0, 0) :
    arVector2(x.v[0]/c, x.v[1]/c);
}

// addition
// Should also define operator+=
inline arVector2 operator+(const arVector2& x, const arVector2& y) {
  return arVector2(x.v[0]+y.v[0], x.v[1]+y.v[1]);
}

// negation
// Should also define operator-=
inline arVector2 operator-(const arVector2& x) {
  return arVector2(-x.v[0], -x.v[1]);
}

// subtraction
// Should also define operator-=
inline arVector2 operator-(const arVector2& x, const arVector2& y) {
  return arVector2(x.v[0]-y.v[0], x.v[1]-y.v[1]);
}

// cross product
// Should also define operator*=
inline arVector3 operator*(const arVector3& x, const arVector3& y) {
  return arVector3(x.v[1]*y.v[2] - x.v[2]*y.v[1],
                   x.v[2]*y.v[0] - x.v[0]*y.v[2],
                   x.v[0]*y.v[1] - x.v[1]*y.v[0]);
}

// scalar multiply
// Should also define operator*=
inline arVector3 operator*(float c, const arVector3& x) {
  return arVector3(c*x.v[0], c*x.v[1], c*x.v[2]);
}
// Should also define operator*=
inline arVector3 operator*(const arVector3& x, float c) {
  return c * x;
}

// scalar division, returns all zeros if scalar zero
// Should also define operator/=
inline arVector3 operator/(const arVector3& x, float c) {
  return (c==0) ?
    arVector3(0, 0, 0) :
    arVector3(x.v[0]/c, x.v[1]/c, x.v[2]/c);
}

// addition
// Should also define operator+=
inline arVector3 operator+(const arVector3& x, const arVector3& y) {
  return arVector3(x.v[0]+y.v[0], x.v[1]+y.v[1], x.v[2]+y.v[2]);
}

// negation
// Should also define operator-=
inline arVector3 operator-(const arVector3& x) {
  return arVector3(-x.v[0], -x.v[1], -x.v[2]);
}

// subtraction
// Should also define operator-=
inline arVector3 operator-(const arVector3& x, const arVector3& y) {
  return arVector3(x.v[0]-y.v[0], x.v[1]-y.v[1], x.v[2]-y.v[2]);
}

// dot product
inline float operator%(const arVector3& x, const arVector3& y) {
  return x.v[0]*y.v[0]+x.v[1]*y.v[1]+x.v[2]*y.v[2];
}

// magnitude
inline float operator++(const arVector3& x) {
  return sqrt(x.v[0]*x.v[0]+x.v[1]*x.v[1]+x.v[2]*x.v[2]);
}

//************************************
// matrix inline
//************************************

// add matrices
// Should also define operator+=
inline arMatrix4 operator+(const arMatrix4& x, const arMatrix4& y) {
  return arMatrix4(x.v[0]+y.v[0], x.v[4]+y.v[4],
                   x.v[8]+y.v[8], x.v[12]+y.v[12],
                   x.v[1]+y.v[1], x.v[5]+y.v[5],
                   x.v[9]+y.v[9], x.v[13]+y.v[13],
                   x.v[2]+y.v[2], x.v[6]+y.v[6],
                   x.v[10]+y.v[10], x.v[14]+y.v[14],
                   x.v[3]+y.v[3], x.v[7]+y.v[7],
                   x.v[11]+y.v[11], x.v[15]+y.v[15]);
}

// Should also define operator-=
inline arMatrix4 operator-(const arMatrix4& x) {
  return arMatrix4(-x.v[0], -x.v[4], -x.v[8], -x.v[12],
                   -x.v[1], -x.v[5], -x.v[9], -x.v[13],
                   -x.v[2], -x.v[6], -x.v[10], -x.v[14],
                   -x.v[3], -x.v[7], -x.v[11], -x.v[15]);
}

// Should also define operator-=
inline arMatrix4 operator-(const arMatrix4& x, const arMatrix4& y) {
  return arMatrix4(x.v[0]-y.v[0], x.v[4]-y.v[4],
                   x.v[8]-y.v[8], x.v[12]-y.v[12],
                   x.v[1]-y.v[1], x.v[5]-y.v[5],
                   x.v[9]-y.v[9], x.v[13]-y.v[13],
                   x.v[2]-y.v[2], x.v[6]-y.v[6],
                   x.v[10]-y.v[10], x.v[14]-y.v[14],
                   x.v[3]-y.v[3], x.v[7]-y.v[7],
                   x.v[11]-y.v[11], x.v[15]-y.v[15]);
}

inline arMatrix4 operator~(const arMatrix4& x) {
  return arMatrix4(x.v[0], x.v[1], x.v[2], x.v[3],
                   x.v[4], x.v[5], x.v[6], x.v[7],
                   x.v[8], x.v[9], x.v[10], x.v[11],
                   x.v[12], x.v[13], x.v[14], x.v[15]);
}

//******************************
// quaternion inline
//******************************

// multiplication
// Should also define operator*=
inline arQuaternion operator*(const arQuaternion& x, const arQuaternion& y) {
  return arQuaternion(
    x.real*y.real-x.pure.v[0]*y.pure.v[0]-x.pure.v[1]*y.pure.v[1]-
    x.pure.v[2]*y.pure.v[2],
    x.real*y.pure.v[0]+y.real*x.pure.v[0]+x.pure.v[1]*y.pure.v[2]-
    x.pure.v[2]*y.pure.v[1],
    x.real*y.pure.v[1]+y.real*x.pure.v[1]+x.pure.v[2]*y.pure.v[0]-
    x.pure.v[0]*y.pure.v[2],
    x.real*y.pure.v[2]+y.real*x.pure.v[2]+x.pure.v[0]*y.pure.v[1]-
    x.pure.v[1]*y.pure.v[0]);
}

// scalar multiplication
// Should also define operator*=
inline arQuaternion operator*(const float c, const arQuaternion& x) {
  return arQuaternion(c*x.real, c*x.pure);
}
inline arQuaternion operator*(const arQuaternion& x, const float c) {
  return c * x;
}

// scalar division, return all zeros if scalar==0
// Should also define operator/=
inline arQuaternion operator/(const arQuaternion& x, const float c) {
  return c==0 ? arQuaternion(0, 0, 0, 0) : x * (1./c);
}

// addition
// Should also define operator+=
inline arQuaternion operator+(const arQuaternion& x, const arQuaternion& y) {
  return arQuaternion(x.real + y.real, x.pure + y.pure);
}

// negation
// Should also define operator-=
inline arQuaternion operator-(const arQuaternion& x) {
  return arQuaternion(-x.real, -x.pure);
}

// subtraction
// Should also define operator-=
inline arQuaternion operator-(const arQuaternion& x, const arQuaternion& y) {
  return arQuaternion(x.real - y.real, x.pure - y.pure);
}

// inverse of quaternion
inline arQuaternion operator!(const arQuaternion& x) {
  return x.inverse();
}

//******************************************
// misc inlined
//******************************************

inline float ar_cmToFeet( float cm ) {
  return cm * 0.032808399;
}

// todo: rename like DegFromRad, RadFromDeg.

inline float ar_convertToDeg(const float angle) {
  return angle * 180 / M_PI;
}

inline float ar_convertToRad(const float angle) {
  return angle / 180 * M_PI;
}

inline arVector3 ar_convertToDeg(const arVector3 v) {
  return 180 / M_PI * v;
}

inline arVector3 ar_convertToRad(const arVector3 v) {
  return M_PI / 180 * v;
}

// Division by 4th homogeneous coordinate handles projective transformations.
inline arVector3 operator*(const arMatrix4& m, const arVector3& x) {
  arVector3 result;
  for (int i=0; i<3; i++) {
    result.v[i] = m.v[i]*x.v[0] + m.v[i+4]*x.v[1] + m.v[i+8]*x.v[2] + m.v[i+12];
  }
  return 1 / (m.v[3]*x.v[0] + m.v[7]*x.v[1] + m.v[11]*x.v[2] + m.v[15]) * result;
}

//inline arVector4 operator*(const arMatrix4& m, const arVector4& x) {
// arVector4 result;
// float scaleFactor = 1./(m.v[3]*x.v[0]+m.v[7]*x.v[1]+m.v[11]*x.v[2]+m.v[15]);
//  for (int i=0; i<4; i++) {
//    result.v[i] = scaleFactor*(m.v[i]*x.v[0]+m.v[i+4]*x.v[1]+m.v[i+8]*x.v[2]+x.v[3]*m.v[i+12]);
//  }
//  return result;
//}

inline arVector3 operator*(const arQuaternion& q , const arVector3& x) {
  return (q * arQuaternion(0, x.v[0], x.v[1], x.v[2]) * !q).pure;
}

float SZG_CALL ar_randUniformFloat(long* idum);
int SZG_CALL ar_randUniformInt(long* idum, int lo, int hi);


bool SZG_CALL ar_unpackIntVector( vector<int>& vec, int** p );
bool SZG_CALL ar_unpackVector3Vector( vector<arVector3>& vec, float** p );
bool SZG_CALL ar_unpackVector2Vector( vector<arVector2>& vec, float** p );
bool SZG_CALL ar_unpackVector4Vector( vector<arVector4>& vec, float** p );



#endif
