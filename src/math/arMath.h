//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATH_H
#define AR_MATH_H

#include "arMathCalling.h"
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

// Sometimes we need to define a axis order.
enum arAxisOrder { AR_XYZ = 1, AR_XZY, AR_YXZ, AR_YZX, AR_ZXY, AR_ZYX };

class arQuaternion;

/// vector of 3 points.  Position or direction in 3-space.

class SZG_CALL arVector3{
 public:
  float v[3];

  arVector3() { v[0] = v[1] = v[2] = 0.; }
  arVector3(const float* p)
    { set(p[0], p[1], p[2]); }
  arVector3(float x, float y, float z)
    { set(x,y,z); }
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
  // DO NOT add an operator conversion to float*
  // DO NOT add an operator that returns const float& from []
  // these cause VC++ 6.0 to fail. Note how they make myVector3[1] ambiguous
  //operator float* () { return v; }
  //const float& operator[] (int i) const { return v[i]; }
  float& operator[] (int i)
    { return v[i]; }
  void set(float x, float y, float z)
    { v[0]=x; v[1]=y; v[2]=z; }
  float magnitude() const { return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); }
  arVector3 normalize() const {
    const float mag = magnitude();
    if (mag <= 0.) {
      cerr << "arVector3 error: cannot normalize zero vector.\n";
      return arVector3(0,0,0);
    }
    return arVector3(v[0]/mag,v[1]/mag,v[2]/mag);
  }
  float dot( const arVector3& y ) const {
    return v[0]*y.v[0]+v[1]*y.v[1]+v[2]*y.v[2];
  }
};

/// vector of 4 floats. A position in 4-space

class arMatrix4;

class SZG_CALL arVector4{
 public:
  float v[4];

  arVector4(){ v[0] = v[1] = v[2] = v[3] = 0; }
  arVector4(const float* p){ set(p[0], p[1], p[2], p[3]); }
  arVector4(float x, float y, float z, float w){ set(x,y,z,w); }
  arVector4(const arVector3& vec, float w) { set(vec.v[0],vec.v[1],vec.v[2],w); }
  // DO NOT add an operator conversion to float*
  // DO NOT add an operator that returns const float& from []
  // these cause VC++ 6.0 to fail. Note how they make myVector4[1] ambiguous
  //operator float* () { return v; }
  //const float& operator[] (int i) const { return v[i]; }
  float& operator[] (int i)
    { return v[i]; }
  void set(float x, float y, float z, float w)
    { v[0]=x; v[1]=y; v[2]=z; v[3] = w;}
  float magnitude() const 
    { return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3]); }
  float dot( const arVector4& y ) const {
    return v[0]*y.v[0]+v[1]*y.v[1]+v[2]*y.v[2]+v[3]*y.v[3];
  }
  arVector4 normalize() const {
    const float mag = magnitude();
    if (mag <= 0.) {
      cerr << "arVector4 error: attempt to normalize 0-length vector.\n";      
      return arVector4(0,0,0,0);
    }
    return arVector4(v[0]/mag,v[1]/mag,v[2]/mag,v[3]/mag);
  }
  arMatrix4 outerProduct( const arVector4& rhs ) const;
};

/// 4x4 matrix.  Typically an OpenGL transformation.

class SZG_CALL arMatrix4{
 public:
  arMatrix4();
  arMatrix4(const float* const);
  arMatrix4(float,float,float,float,float,float,float,float,
	          float,float,float,float,float,float,float,float);
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

/// For representing rotations.

class SZG_CALL arQuaternion{
 public:
  arQuaternion();
  arQuaternion(float real, float pure1, float pure2, float pure3);
  arQuaternion(float real, const arVector3& pure);
  arQuaternion( const float* numAddress );
  ~arQuaternion() {}

  operator arMatrix4() const;

  // THIS METHOD HAS NOT BEEN IMPLEMENTED (Not sure how just yet).
  // Meant to replace operator!, which has also not been implemented.
//  arQuaternion inverse() const;

  float real;
  arVector3 pure;
};

//***************************************************************
// here are all the function prototypes... most are inlined below
//***************************************************************

// *********** vector ******************
// cross product
SZG_CALL arVector3 operator*(const arVector3&, const arVector3&); 
SZG_CALL arVector3 operator*(float, const arVector3&); // scalar multiply
SZG_CALL arVector3 operator*(const arVector3&, float); // scalar multiply
// scalar division, handles x/0
SZG_CALL arVector3 operator/(const arVector3&, float); 
SZG_CALL arVector3 operator+(const arVector3&, const arVector3&); // add
SZG_CALL arVector3 operator-(const arVector3&); // negate
SZG_CALL arVector3 operator-(const arVector3&, const arVector3&); // subtract
SZG_CALL float operator%(const arVector3&, const arVector3&); // dot product
SZG_CALL float operator++(const arVector3&); // magnitude
SZG_CALL ostream& operator<<(ostream&, const arVector3&);
SZG_CALL ostream& operator<<(ostream&, const arVector4&);

//************ matrix *******************
// matrix multiply
SZG_CALL arMatrix4 operator*(const arMatrix4&,const arMatrix4&); 
SZG_CALL arMatrix4 operator+(const arMatrix4&, const arMatrix4&); //addition
SZG_CALL arMatrix4 operator-(const arMatrix4&); //negation
SZG_CALL arMatrix4 operator-(const arMatrix4&, const arMatrix4&); //subtraction
//inverse, return all zeros if singular
SZG_CALL arMatrix4 operator!(const arMatrix4&); 
SZG_CALL ostream& operator<<(ostream&, const arMatrix4&);

//************* quaternion ***************
SZG_CALL arQuaternion operator*(const arQuaternion&, const arQuaternion&);
SZG_CALL arQuaternion operator*(float, const arQuaternion&);
SZG_CALL arQuaternion operator*(const arQuaternion&, float);
// division by scalar,
// if scalar==0, return all zeros
SZG_CALL arQuaternion operator/(const arQuaternion&, float);                   
SZG_CALL arQuaternion operator+(const arQuaternion&, const arQuaternion&);
SZG_CALL arQuaternion operator-(const arQuaternion&);
SZG_CALL arQuaternion operator-(const arQuaternion&, const arQuaternion&);
// inverts unit quaternion (NOT IMPLEMENTED)
//SZG_CALL arQuaternion operator!(const arQuaternion&);       
SZG_CALL float operator++(const arQuaternion&);   // magnitude
SZG_CALL ostream& operator<<(ostream&, const arQuaternion&);

//************* misc **********************
// vector transform operators
SZG_CALL arVector3 operator*(const arMatrix4&, const arVector3&);
SZG_CALL arVector3 operator*(const arQuaternion&, const arVector3&);

//SZG_CALL arVector4 operator*(const arMatrix4&, const arVector4&);

//********* utility functions ******
SZG_CALL arMatrix4 ar_identityMatrix();
SZG_CALL arMatrix4 ar_translationMatrix(float,float,float);
SZG_CALL arMatrix4 ar_translationMatrix(const arVector3&);
SZG_CALL arMatrix4 ar_rotationMatrix(char,float);
SZG_CALL arMatrix4 ar_rotationMatrix(const arVector3&,float);
SZG_CALL arMatrix4 ar_rotateVectorToVector( const arVector3& vec1, 
                                            const arVector3& vec2 );
SZG_CALL arMatrix4 ar_transrotMatrix(const arVector3& position, 
                                     const arQuaternion& orientation);
SZG_CALL arMatrix4 ar_scaleMatrix(float);
SZG_CALL arMatrix4 ar_scaleMatrix(float,float,float);
SZG_CALL arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors);
SZG_CALL arMatrix4 ar_extractTranslationMatrix(const arMatrix4&);
SZG_CALL arVector3 ar_extractTranslation(const arMatrix4&);
SZG_CALL arMatrix4 ar_extractRotationMatrix(const arMatrix4&);
SZG_CALL arMatrix4 ar_extractScaleMatrix(const arMatrix4&);
SZG_CALL float     ar_angleBetween(const arVector3&, const arVector3&);
SZG_CALL arVector3 ar_extractEulerAngles(const arMatrix4& m, arAxisOrder o=AR_ZYX);
SZG_CALL arQuaternion ar_angleVectorToQuaternion(const arVector3&,float);
SZG_CALL float ar_convertToRad(float);
SZG_CALL float ar_convertToDeg(float);
// returns the relected vector of direction across normal.
SZG_CALL arVector3 ar_reflect(const arVector3& direction, 
                              const arVector3& normal);
SZG_CALL float ar_intersectRayTriangle(const arVector3& rayOrigin,
			               const arVector3& rayDirection,
			               const arVector3& vertex1,
			               const arVector3& vertex2,
			               const arVector3& veretx3);

// note lineDirection & planeNormal don't have to be normalized
// (affects interpretation of range, though).
// returns false if no intersection.  Otherwise, intersection
// = linePoint + range*lineDirection.
SZG_CALL bool ar_intersectLinePlane( const arVector3& linePoint,
                                     const arVector3& lineDirection,
                                     const arVector3& planePoint,
                                     const arVector3& planeNormal,
                                     float& range );

// finds the point on a line that is nearest to some point
SZG_CALL arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                          const arVector3& lineDirection,
                                          const arVector3& otherPoint,
                                          const float threshold = 1e-6 );

// Matrix for doing reflections in a plane. This matrix should pre-multiply
// the object matrix on the stack (i.e. load this one on first, then multiply
// by the object placement matrix).
SZG_CALL arMatrix4 ar_mirrorMatrix( const arVector3& planePoint, const arVector3& planeNormal );

// Matrix to project a set of vertices onto a plane (for cast shadows)
// Note that this matrix should be post-multiplied on the top of the stack.
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


//********** useful user interface transformations ***********
//********** good for mouse->3D                    ***********
SZG_CALL arMatrix4 ar_planeToRotation(float,float);

//********** screen-related transformations, useful for VR *******
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
// vector inlined functions
//*************************************

// vector cross product
// Should also define operator*=
inline arVector3 operator*(const arVector3& x, const arVector3& y){
  return arVector3(x.v[1]*y.v[2] - x.v[2]*y.v[1],
                   x.v[2]*y.v[0] - x.v[0]*y.v[2],
                   x.v[0]*y.v[1] - x.v[1]*y.v[0]);
}

// scalar multiply
// Should also define operator*=
inline arVector3 operator*(float c, const arVector3& x){
  return arVector3(c*x.v[0], c*x.v[1], c*x.v[2]);
}
// Should also define operator*=
inline arVector3 operator*(const arVector3& x, float c){
  return c * x;
}

// scalar division, returns all zeros if scalar zero
// Should also define operator/=
inline arVector3 operator/(const arVector3& x, float c){
  return (c==0) ?
    arVector3(0,0,0) :
    arVector3(x.v[0]/c, x.v[1]/c, x.v[2]/c);
}

// addition
// Should also define operator+=
inline arVector3 operator+(const arVector3& x, const arVector3& y){
  return arVector3(x.v[0]+y.v[0], x.v[1]+y.v[1], x.v[2]+y.v[2]);
}

// opposite
// Should also define operator-=
inline arVector3 operator-(const arVector3& x){
  return arVector3(-x.v[0],-x.v[1],-x.v[2]);
}

// subtraction
// Should also define operator-=
inline arVector3 operator-(const arVector3& x, const arVector3& y){
  return arVector3(x.v[0]-y.v[0],x.v[1]-y.v[1],x.v[2]-y.v[2]);
}

// dot product
inline float operator%(const arVector3& x, const arVector3& y){
  return x.v[0]*y.v[0]+x.v[1]*y.v[1]+x.v[2]*y.v[2];
}

// magnitude
inline float operator++(const arVector3& x){
  return sqrt(x.v[0]*x.v[0]+x.v[1]*x.v[1]+x.v[2]*x.v[2]);
}
inline float magnitude(const arVector3& x){
  return sqrt(x.v[0]*x.v[0]+x.v[1]*x.v[1]+x.v[2]*x.v[2]);
}

//************************************
// matrix inlines
//************************************

// add matrices
// Should also define operator+=
inline arMatrix4 operator+(const arMatrix4& x, const arMatrix4& y){
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
inline arMatrix4 operator-(const arMatrix4& x){
  return arMatrix4(-x.v[0],-x.v[4],-x.v[8],-x.v[12],
		   -x.v[1],-x.v[5],-x.v[9],-x.v[13],
		   -x.v[2],-x.v[6],-x.v[10],-x.v[14],
		   -x.v[3],-x.v[7],-x.v[11],-x.v[15]);
}

// Should also define operator-=
inline arMatrix4 operator-(const arMatrix4& x, const arMatrix4& y){
  return arMatrix4(x.v[0]-y.v[0], x.v[4]-y.v[4],
		   x.v[8]-y.v[8], x.v[12]-y.v[12],
		   x.v[1]-y.v[1], x.v[5]-y.v[5],
		   x.v[9]-y.v[9], x.v[13]-y.v[13],
		   x.v[2]-y.v[2], x.v[6]-y.v[6],
		   x.v[10]-y.v[10], x.v[14]-y.v[14],
                   x.v[3]-y.v[3], x.v[7]-y.v[7],
		   x.v[11]-y.v[11], x.v[15]-y.v[15]);
}

inline arMatrix4 operator~(const arMatrix4& x){
  return arMatrix4(x.v[0],x.v[1],x.v[2],x.v[3],
		   x.v[4],x.v[5],x.v[6],x.v[7],
		   x.v[8],x.v[9],x.v[10],x.v[11],
		   x.v[12],x.v[13],x.v[14],x.v[15]);
}


//******************************
// quaternion inlined
//******************************

// multiplication
// Should also define operator*=
inline arQuaternion operator*(const arQuaternion& x, const arQuaternion& y){
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
inline arQuaternion operator*(float c, const arQuaternion& x){
  return arQuaternion(c*x.real,c*x.pure.v[0],c*x.pure.v[1],c*x.pure.v[2]);
}
inline arQuaternion operator*(const arQuaternion& x, float c){
  return c * x;
}

// scalar division, return all zeros if scalar==0
// Should also define operator/=
inline arQuaternion operator/(const arQuaternion& x, float c){
  if (c==0){
    return arQuaternion(0,0,0,0);
  }
  return arQuaternion(x.real/c, x.pure.v[0]/c, x.pure.v[1]/c, x.pure.v[2]/c);
}

// addition
// Should also define operator+=
inline arQuaternion operator+(const arQuaternion& x, const arQuaternion& y){
  return arQuaternion(x.real+y.real, x.pure.v[0]+y.pure.v[0],
	              x.pure.v[1]+y.pure.v[1], x.pure.v[2]+y.pure.v[2]);
}

// negation
// Should also define operator-=
inline arQuaternion operator-(const arQuaternion& x){
  return arQuaternion(-x.real,-x.pure.v[0],-x.pure.v[1],-x.pure.v[2]);
}

// subtraction
// Should also define operator-=
inline arQuaternion operator-(const arQuaternion& x, const arQuaternion& y){
  return arQuaternion(x.real-y.real, x.pure.v[0]-y.pure.v[0],
		      x.pure.v[1]-y.pure.v[1], x.pure.v[2]-y.pure.v[2]);
}

// inverse of unit quaternion
inline arQuaternion operator!(const arQuaternion& x){
  return arQuaternion(x.real,-x.pure.v[0],-x.pure.v[1],-x.pure.v[2]);
}

// magnitude
inline float operator++(const arQuaternion& x){
  return sqrt(x.real*x.real+x.pure.v[0]*x.pure.v[0]+
	      x.pure.v[1]*x.pure.v[1]+x.pure.v[2]*x.pure.v[2]);
}

//******************************************
// misc inlined
//******************************************

inline float ar_convertToDeg(float angle){
  return 57.29578*angle;
}

inline float ar_convertToRad(float angle){
  return 0.017453293*angle;
}

inline float ar_cmToFeet( float cm ) {
  return cm * 0.032808399;
}

// NOTE: Added division by 4th homogeneous coordinate to correctly handle
// projective transformations.  Original function body commented out below
inline arVector3 operator*(const arMatrix4& m,const arVector3& x){
 arVector3 result;
 float scaleFactor = 1./(m.v[3]*x.v[0]+m.v[7]*x.v[1]+m.v[11]*x.v[2]+m.v[15]);
  for (int i=0; i<3; i++){
    result.v[i] = scaleFactor*(m.v[i]*x.v[0]+m.v[i+4]*x.v[1]+m.v[i+8]*x.v[2]+m.v[i+12]);
  }
  return result;
//  for (int i=0; i<3; i++){
//    result.v[i] = m.v[i]*x.v[0]+m.v[i+4]*x.v[1]+m.v[i+8]*x.v[2]+m.v[i+12];
//  }
//  return result;
}

//inline arVector4 operator*(const arMatrix4& m,const arVector4& x){
// arVector4 result;
// float scaleFactor = 1./(m.v[3]*x.v[0]+m.v[7]*x.v[1]+m.v[11]*x.v[2]+m.v[15]);
//  for (int i=0; i<4; i++){
//    result.v[i] = scaleFactor*(m.v[i]*x.v[0]+m.v[i+4]*x.v[1]+m.v[i+8]*x.v[2]+x.v[3]*m.v[i+12]);
//  }
//  return result;
//}

inline arVector3 operator*(const arQuaternion& q ,const arVector3& x){
  arQuaternion temp = q*arQuaternion(0,x.v[0],x.v[1],x.v[2])*!q;
  return temp.pure;
}

float SZG_CALL ar_randUniformFloat(long* idum);
int SZG_CALL ar_randUniformInt(long* idum, int lo, int hi);

#endif
