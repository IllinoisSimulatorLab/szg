// $Id: PyMath.i,v 1.1 2005/03/18 20:13:01 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// ************************** based on arMath.h *********************

// The class arVector3 contains detailed comments that illustrate the ideas
// and techniques that go into the generation of wrappers. The remaining
// classes are very similar and only contain detailed comments when they
// introduce new concepts.

// Note that there are several ways of wrapping overloaded operators.
// If the operator in question is a method of a class, then simply
// renaming it may be enough, as in the case of the two operators below.
%rename(__eq__) arVector3::operator==;
%rename(__ne__) arVector3::operator!=;
%rename(__getitem__) arVector3::operator[];

class arVector3{
 public:
  float v[3];

  arVector3();
  arVector3(const float* p);
  arVector3(float x, float y, float z);
  ~arVector3();

    bool operator==(const arVector3& rhs) const;
    bool operator!=(const arVector3& rhs) const;

    float operator[] (int i);
// Note that the above line looks like 'float& operator[] (int i);' in
// szg/src/math/arMath.h, but this does not work well in Python. SWIG
// automatically handles the new output type.

  void set(float x, float y, float z);
  float magnitude() const;
  arVector3 normalize() const;
  float dot( const arVector3& y ) const;

// The %extend directive below adds methods to the class arVector3. Note
// that this does note change the C++ class arVector3; rather, it creates
// new C++ methods that show up as methods of the Python class that shadows
// arVector3. The installation of szg is not affected by this.
%extend{

//  const arVector3& operator+=(const arVector3&);
    arVector3 __iadd__(const arVector3& rhs) {
        *self+=rhs;
        return *self;
    }
// Note that there is a simpler way to wrap the augmented operators like
// += and -= and such. To wit, one could add the line
//   %rename(__iadd__) arVector3::operator*=;
// to the other renaming directives, and then change the line
//   const arVector3& operator+=(const arVector3&);
// to
//   const arVector3 operator+=(const arVector3&);
// SWIG will automatically handle the necessary pointer conversions.
// The disadvantage of this approach is that it changes the semantics
// of the += operator, i.e., the meaning of a+=b in Python would be
// a=(a+b), in other words, each use of the += operator would cause
// the creation of a new object for the sum, whereas the current
// implementation of += changes an existing object in situ, which
// is a lot more efficient.

    float __setitem__(int i,float rhs) {
        self->v[i]=rhs;
        return rhs;
    }

//    const arVector3& operator-=(const arVector3&);
    arVector3 __isub__(const arVector3& rhs) {
        *self-=rhs;
        return *self;
    }

//    const arVector3& operator*=(float);
    arVector3 __imul__(float rhs) {
        *self*=rhs;
        return *self;
    }

//    const arVector3& operator/=(float);
    arVector3 __idiv__(float rhs) {
        *self/=rhs;
        return *self;
    }

// arVector3 operator*(const arVector3&, float); // scalar multiply
    arVector3 __mul__(float rhs) {
        return *self*rhs;
    }

//  arVector3 operator*(float ,const arVector3&); // scalar multiply
    arVector3 __rmul__(float lhs) {
        return lhs*(*self);
    }

// arVector3 operator*(const arVector3&, const arVector3&); // cross product
    arVector3 __mul__(const arVector3& rhs) {
        return *self*rhs;
    }

// arVector3 operator/(const arVector3&, float); // scalar division, handles x/0
    arVector3 __div__(float rhs) {
        return *self/rhs;
    }

// arVector3 operator+(const arVector3&, const arVector3&); // add
    arVector3 __add__(const arVector3& rhs) {
        return *self+rhs;
    }

// arVector3 operator-(const arVector3&); // negate
    arVector3 __neg__(void) {
        return -(*self);
    }

// arVector3 operator-(const arVector3&, const arVector3&); // subtract
    arVector3 __sub__(const arVector3& rhs) {
        return *self-rhs;
    }

// float operator%(const arVector3&, const arVector3&); // dot product
    float __mod__(const arVector3& rhs) {
        return *self%rhs;
    }

// float operator++(const arVector3&); // magnitude
    float __abs__(void) {
        return ++(*self);
    }

// __repr__ roughly serves the same purpose as the overloaded << operator.
// The wrapper below is not a 1-1 translation of operators but rather a
// translation from idiomatic C++ to idiomatic Python.
//
// ostream& operator<<(ostream&, const arVector3&);
    string __repr__(void) {
        ostringstream s(ostringstream::out);
        s << *self;
        return s.str();
    }
}

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self[0],self[1],self[2]]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,v):
        _swig_setattr(self, arVector3, 'this',
            _PySZG.new_arVector3(v[0],v[1],v[2]))
        _swig_setattr(self, arVector3, 'thisown', 1)
}
};

%rename(__getitem__) arVector4::operator[];

class arVector4{
 public:
  float v[4];

  arVector4();
  arVector4(const float* p);
  arVector4(float x, float y, float z, float w);
  float operator[] (int i);
        // the output type of [] used to be float&;
        // see corresponding comment in arVector3
  void set(float x, float y, float z, float w);
  float magnitude() const;
  arVector4 normalize() const;

%extend{

    float __setitem__(int i,float rhs) {
        self->v[i]=rhs;
        return rhs;
    }

    float __abs__(void) {
        return self->magnitude();
    }

// __repr__ roughly serves the same purpose as the overloaded << operator
// ostream& operator<<(ostream&, const arVector4&);
    string __repr__(void) {
        ostringstream s(ostringstream::out);
        s << *self;
        return s.str();
    }
}

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self[0],self[1],self[2],self[3]]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,v):
        _swig_setattr(self, arVector4, 'this',
            _PySZG.new_arVector4(v[0],v[1],v[2],v[3]))
        _swig_setattr(self, arVector4, 'thisown', 1)
}
};


%{
#include "arMath.h"
%}

%rename(__eq__) arMatrix4::operator==;
%rename(__ne__) arMatrix4::operator!=;
%rename(__getitem__) arMatrix4::operator[];
%rename(toQuaternion) arMatrix4::operator arQuaternion() const;

class arMatrix4{
 public:
  arMatrix4();
  arMatrix4(const float* const);
  arMatrix4(float,float,float,float,float,float,float,float,
              float,float,float,float,float,float,float,float);
  ~arMatrix4() {}
  
  arMatrix4 inverse() const;

    operator arQuaternion() const;
    float operator[](int i); // used to be float&
    bool operator==(const arMatrix4& rhs) const;
    bool operator!=(const arMatrix4& rhs) const;

/*  float v[16];*/

%extend{

    float* getAddress() {
       return self->v;
    }

    float __setitem__(int i,float rhs) {
        self->v[i]=rhs;
        return rhs;
    }

// arMatrix4 operator*(const arMatrix4&,const arMatrix4&); // matrix multiply
    arMatrix4 __mul__(const arMatrix4& rhs) {
        return *self*rhs;
    }

// arVector3 operator*(const arMatrix4&,const arVector3&);
    arVector3 __mul__(const arVector3& rhs) {
        return *self*rhs;
    }

// arMatrix4 operator+(const arMatrix4&, const arMatrix4&); //addition
    arMatrix4 __add__(const arMatrix4& rhs) {
        return *self+rhs;
    }

// arMatrix4 operator-(const arMatrix4&); //negation
    arMatrix4 __neg__(void) {
        return -(*self);
    }

// arMatrix4 operator-(const arMatrix4&, const arMatrix4&); //subtraction
    arMatrix4 __sub__(const arMatrix4& rhs) {
        return *self-rhs;
    }

// arMatrix4 operator~(const arMatrix4&); //transpose
// Note that in Python, ~foo usually means the inverse. Unfortunately,
// ~foo means foo transposed in szg [snip]
// [JAC 1/7/05]: arMatrix4::operator~() was declared in arMath.h but never
// implemented. Ive removed the declaration, as I disapprove of it; there
// is now a transpose() method for this purpose.
    arMatrix4 __invert__(void) {
        return self->inverse();
    }


// arMatrix4 operator!(const arMatrix4&); inverse, return all zeros if singular
// [JAC 1/7/05]: Note that theres also an inverse() method for this.
    arMatrix4 invert(void) {
        return self->inverse();
    }

    arMatrix4 __pow__(int k) {
        arMatrix4 m;
        arMatrix4 res;
        if (k<0) {
            m=!(*self);
            k=-k;
        }
        else
            m=*self;
        for(int i=0;i<k;i++)
            res=res*m;
        return res;
    }


// __repr__ roughly serves the same purpose as the overloaded << operator
// ostream& operator<<(ostream&, const arMatrix4&);
    string __repr__(void) {
        ostringstream s(ostringstream::out);
        s << *self;
        return s.str();
    }

// This function has no correspondent in arMath.h, but it may be useful anyway.
    float __abs__(void) {
        float s;
        for(int i=0;i<16;i++)
            s+=self->v[i]*self->v[i];
        return sqrt(s);
    }
}

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self[0],self[4],self[8],self[12],
                self[1],self[5],self[9],self[13],
                self[2],self[6],self[10],self[14],
                self[3],self[7],self[11],self[15]]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,v):
        _swig_setattr(self, arMatrix4, 'this',
            _PySZG.new_arMatrix4(
                v[0],v[1],v[2],v[3],
                v[4],v[5],v[6],v[7],
                v[8],v[9],v[10],v[11],
                v[12],v[13],v[14],v[15]
            )
        )
        _swig_setattr(self, arMatrix4, 'thisown', 1)
}
};

%rename(toMatrix) arQuaternion::operator arMatrix4() const;

class arQuaternion{
 public:
  arQuaternion();
  arQuaternion(float real, float pure1, float pure2, float pure3);
  arQuaternion(float real, const arVector3& pure);
  arQuaternion( const float* numAddress );
  ~arQuaternion() {}

  operator arMatrix4() const;

  float real;
  arVector3 pure;

%extend{
// arQuaternion operator*(const arQuaternion&, const arQuaternion&);
    arQuaternion __mul__(const arQuaternion& rhs) {
        return *self*rhs;
    }

// arQuaternion operator*(const arQuaternion&, float);
    arQuaternion __mul__(float rhs) {
        return *self*rhs;
    }

// arVector3 operator*(const arQuaternion&,const arVector3&);
    arVector3 __mul__(const arVector3& rhs) {
        return *self*rhs;
    }

// arQuaternion operator*(float, const arQuaternion&);
    arQuaternion __rmul__(float lhs) {
        return lhs*(*self);
    }

// arQuaternion operator/(const arQuaternion&, float); // division by scalar,
                                            // if scalar==0, return all zeros
    arQuaternion __div__(float rhs) {
        return *self/rhs;
    }

// arQuaternion operator+(const arQuaternion&, const arQuaternion&);
    arQuaternion __add__(const arQuaternion& rhs) {
        return *self+rhs;
    }

// arQuaternion operator-(const arQuaternion&);
    arQuaternion __add__(void) {
        return -(*self);
    }

// arQuaternion operator-(const arQuaternion&, const arQuaternion&);
    arQuaternion __sub__(const arQuaternion& rhs) {
        return *self-rhs;
    }

// arQuaternion operator!(const arQuaternion&);       // inverts unit quaternion
// [JAC 1/7/05]: This operator was never implemented & has been deleted.
//    arQuaternion invert(void) {
//        return !(*self);
//    }

// float operator++(const arQuaternion&);   // magnitude
    float __abs__(void) {
        return ++(*self);
    }


// __repr__ roughly serves the same purpose as the overloaded << operator
// ostream& operator<<(ostream&, const arQuaternion&);
    string __repr__(void) {
        ostringstream s(ostringstream::out);
        s << *self;
        return s.str();
    }
}

%pythoncode {
    def __getstate__(self):
        raise "sorry, pickling not implemented for arQuaternion"

    def __setstate__(self,v):
        raise "sorry, pickling not implemented for arQuaternion"
}
};


// The remainder is straight from arMath.h.

arMatrix4 ar_identityMatrix();
arMatrix4 ar_translationMatrix(float,float,float);
arMatrix4 ar_translationMatrix(const arVector3&);
arMatrix4 ar_rotationMatrix(char,float);
arMatrix4 ar_rotationMatrix(const arVector3&,float);
arMatrix4 ar_rotateVectorToVector( const arVector3& vec1, const arVector3& vec2 );
arMatrix4 ar_transrotMatrix(const arVector3& position, const arQuaternion& orientation);
arMatrix4 ar_scaleMatrix(float);
arMatrix4 ar_scaleMatrix(float,float,float);
arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors);
arMatrix4 ar_mirrorMatrix( const arMatrix4& placementMatrix );
arMatrix4 ar_extractTranslationMatrix(const arMatrix4&);
arVector3 ar_extractTranslation(const arMatrix4&);
arMatrix4 ar_extractRotationMatrix(const arMatrix4&);
arMatrix4 ar_extractScaleMatrix(const arMatrix4&);
float     ar_angleBetween(const arVector3&, const arVector3&);
arVector3 ar_extractEulerAngles(const arMatrix4&);
arQuaternion ar_angleVectorToQuaternion(const arVector3&,float);
float ar_convertToRad(float);
float ar_convertToDeg(float);
// returns the relected vector of direction across normal.
arVector3 ar_reflect(const arVector3& direction, const arVector3& normal);
float ar_intersectRayTriangle(const arVector3& rayOrigin,
                  const arVector3& rayDirection,
                  const arVector3& vertex1,
                  const arVector3& vertex2,
                  const arVector3& veretx3);

bool ar_intersectLinePlane( const arVector3& linePoint,
                            const arVector3& lineDirection,
                            const arVector3& planePoint,
                            const arVector3& planeNormal,
                            float& range );

arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                 const arVector3& lineDirection,
                                 const arVector3& otherPoint,
                                 const float threshold = 1e-6 );

arMatrix4 ar_planeToRotation(float,float);

arMatrix4 ar_frustumMatrix( const arVector3& screenCenter,
                const arVector3& screenNormal,
                            const arVector3& screenUp,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& eyePosition );
arMatrix4 ar_frustumMatrix( const float screenDist,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& locEyePosition );
arMatrix4 ar_lookatMatrix( const arVector3& viewPosition,
                           const arVector3& lookatPosition,
                           const arVector3& up );

// ******************* based on arNavigationUtilities.h *******************

void ar_setNavMatrix( const arMatrix4& matrix );
arMatrix4 ar_getNavMatrix();
arMatrix4 ar_getNavInvMatrix();
void ar_navTranslate( const arVector3& vec );
void ar_navRotate( const arVector3& axis, float degrees );
/// replace master/slave virtualize...() calls
/// Specifically, this is taking a vector, etc. in tracker coordinates
/// and putting it into world coordinates.
arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix );
arVector3 ar_pointToNavCoords( const arVector3& vec );
arVector3 ar_vectorToNavCoords( const arVector3& vec );
/// replace master/slave reify...() calls
/// Taking a vector, etc. in world coordinates and putting it in tracker
/// coordinates.
arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix );
arVector3 ar_pointFromNavCoords( const arVector3& vec );
arVector3 ar_vectorFromNavCoords( const arVector3& vec );
