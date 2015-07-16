//
// Important note: argument labels in methods with %MethodCode blocks, e.g. in:
//
//   void fromSequence( SIP_PYOBJECT a0 );
//
// The 'a0' is there for clarity only and is ignored by the SIP parser.
// In particular, you can't change these labels because SIP automatically maps the first
// argument to 'a0', second to 'a1', and so on.
//
// The 'this' pointer is mapped by SIP onto the variable 'sipCpp', but there's a gotcha:
// Confusingly, if what is implemented as an object method is implemented in a
// global operator C++, as in the following case:
//
//    arVector3 __add__( arVector3 a1 );
//
// Which is implemented in C++ as:
//
//    arVector3 operator+( const arVector3&, const arVector3& );
//
// ...then the 'self-as-C++-object' pointer ends up in a0.

//%Module szgmath 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

// math-global extra code
%ModuleHeaderCode
#include "arMath.h"
#include <string>
using namespace std;

// Utility routines for all math classes

// Set an instance's data from a Python sequence type (e.g. list or tuple).
static int _mathSetFromSequence( const string& className, PyObject* seq, float* outputs, int numOutputs ) {
  ostringstream os(ostringstream::out);
  if (!PySequence_Check(seq)) {
    os << className << " constructor takes " << numOutputs << " Floats or a sequence thereof.";
    PyErr_SetString( PyExc_TypeError, os.str().c_str() );
    return 1;
  }
  if (PySequence_Size(seq) != numOutputs) {
    os << className << " constructor takes " << numOutputs << "-element sequence of floats.";
    PyErr_SetString( PyExc_ValueError, os.str().c_str() );
    return 1;
  }
  for (int i=0; i<numOutputs; ++i) {
    PyObject* tmp = PySequence_GetItem( seq, i );
    if (PyFloat_Check(tmp)) {
      outputs[i] = (float)PyFloat_AsDouble(tmp);
    } else if (PyInt_Check(tmp)) {
      outputs[i] = (float)PyInt_AsLong(tmp);
    } else {
      os << className << " constructor sequence items must all be Floats or Ints";
      PyErr_SetString( PyExc_ValueError, os.str().c_str() );
      Py_XDECREF( tmp );
      return 1;
    }
    Py_XDECREF( tmp );
  } 
  return 0;
}

// Convert an instance's data to a Python list.
static int _mathToList( const string& className, float* fptr, int numValues, PyObject** seq ) {
  *seq=PyList_New(numValues);
  if (!*seq) {
    ostringstream os(ostringstream::out);
    os << className << ".toList() error: PyList_New() failed";
    PyErr_SetString( PyExc_RuntimeError, os.str().c_str() );
    return 1;
  }
  for(int i=0;i<numValues;++i) {
    PyList_SetItem( *seq, i, PyFloat_FromDouble( fptr[i] ) );
  }
  return 0;
}

// Convert an instance's data to a Python tuple.
static int _mathToTuple( const string& className, float* fptr, int numValues, PyObject** seq ) {
  *seq=PyTuple_New(numValues);
  if (!*seq) {
    ostringstream os(ostringstream::out);
    os << className << ".toTuple() error: PyTuple_New() failed";
    PyErr_SetString( PyExc_RuntimeError, os.str().c_str() );
    return 1;
  }
  for(int i=0;i<numValues;++i) {
    PyTuple_SetItem( *seq, i, PyFloat_FromDouble( fptr[i] ) );
  }
  return 0;
}

// support for x = foo[i]
static int _mathGetItem( float* fptr, int i, int numValues, float* out ) {
  if (i < 0) {
    i = i+numValues;
  }
  if ((i<0)||(i>=numValues)) {
    PyErr_SetString( PyExc_IndexError, "szgmath index out of range" );
    return 1;
  }
  *out = fptr[i];
  return 0;
}

// support for foo[i] = x
static int _mathSetItem( float* fptr, int i, int numValues, float out ) {
  if (i < 0) {
    i = i+numValues;
  }
  if ((i<0)||(i>=numValues)) {
    PyErr_SetString( PyExc_IndexError, "szgmath index out of range" );
    return 1;
  }
  fptr[i] = out;
  return 0;
}

%End


// ************************** based on arMath.h *********************


class arVector2 {
 public:
  arVector2();
  arVector2(float x, float y);

  arVector2( const arVector2& a0 );
%MethodCode
    sipCpp = new arVector2( a0->v[0], a0->v[1] );
%End

  // construct from Python sequence.
  arVector2( SIP_PYOBJECT a0 );
%MethodCode
    float args[2];
    sipIsErr = _mathSetFromSequence( "arVector2", a0, args, 2 );
    if (!sipIsErr) {
      sipCpp = new arVector2( args[0], args[1] );
    }
%End

  ~arVector2();

  // Return string representation
  string __str__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << *sipCpp;
    sipRes = new string( s.str() );
%End

  // Return string that should be capable of recreating same object if eval-ed.
  string __repr__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << "arVector2(" << sipCpp->v[0] << "," << sipCpp->v[1] << ")";
    sipRes = new string( s.str() );
%End

  // Convert to Python list.
  SIP_PYOBJECT toList();
%MethodCode
    sipIsErr = _mathToList( "arVector2", sipCpp->v, 2, &sipRes );
%End

  // Convert to Python tuple.
  SIP_PYOBJECT toTuple();
%MethodCode
    sipIsErr = _mathToTuple( "arVector2", sipCpp->v, 2, &sipRes );
%End

  // Set data from Python sequence.
  void fromSequence( SIP_PYOBJECT a0 );
%MethodCode
    sipIsErr = _mathSetFromSequence( "arVector2", a0, sipCpp->v, 2 );
%End

  // NOTE: this automatically generates __getitem__, NOT __setitem__
  // (and there's no bounds-checking).
  //float operator[] (int);

  // x = foo[i]
  float __getitem__( int a0 );
%MethodCode
    sipIsErr = _mathGetItem( sipCpp->v, a0, 2, &sipRes );
%End

  // foo[i] = x
  void __setitem__( int a0, float a1 );
%MethodCode
    sipIsErr = _mathSetItem( sipCpp->v, a0, 2, a1 );
%End

  //v1==v2
  bool operator==(const arVector2& rhs) const;

  //v1!=v2
  bool operator!=(const arVector2& rhs) const;

  // sip 4.10 bug: generated code for operator?= has an extra close paren.  Fixed in sip 4.12.
  const arVector2& operator+=(const arVector2& rhs);
  const arVector2& operator-=(const arVector2& rhs);
  const arVector2& operator*=(float scalar);
  const arVector2& operator/=(float scalar);

  float magnitude() const;
  float magnitude2() const;

  // dot product.
  float dot( const arVector2& y ) const;

  // C++ normalize() doesn't raise an error if norm == 0.
  arVector2 normalize() const /TransferBack/;
%MethodCode
  const float mag = sipCpp->magnitude();
  if (mag <= 0.) {
    PyErr_SetString( PyExc_ValueError, "arVector2 error: can't normalize zero-length vector." );
    sipIsErr = 1;
  } else {
    float* v = const_cast<float *>(sipCpp->v);
    sipRes = new arVector2( v[0]/mag, v[1]/mag );
  }
%End

%PickleCode
    sipRes = Py_BuildValue( (char *)"ff", sipCpp->v[0], sipCpp->v[1] );
%End

};


// K*v1
arVector2 operator*( float, const arVector2 &) /TransferBack/;

// v1*K
arVector2 operator*( const arVector2 &, float) /TransferBack/;

// v1/K
arVector2 operator/(const arVector2&, float) /TransferBack/; // scalar division, handles /0

// v1+v2
arVector2 operator+(const arVector2&, const arVector2&) /TransferBack/;

// -v1
arVector2 operator-(const arVector2&) /TransferBack/; // negate

// v1-v2
arVector2 operator-(const arVector2&, const arVector2&) /TransferBack/;


class arVector3 {
 public:
  arVector3();
  arVector3(float x, float y, float z);

  arVector3( const arVector3& a0 );
%MethodCode
    sipCpp = new arVector3( a0->v );
%End

  // construct from Python sequence.
  arVector3( SIP_PYOBJECT a0 );
%MethodCode
    float args[3];
    sipIsErr = _mathSetFromSequence( "arVector3", a0, args, 3 );
    if (!sipIsErr) {
      sipCpp = new arVector3( const_cast<const float*>(args) );
    }
%End

  ~arVector3();

  //v1==v2
  bool operator==(const arVector3& rhs) const;

  //v1!=v2
  bool operator!=(const arVector3& rhs) const;

  // sip 4.10 bug: generated code for operator?= has an extra close paren.  Fixed in sip 4.12.
  const arVector3& operator+=(const arVector3& rhs);
  const arVector3& operator-=(const arVector3& rhs);
  const arVector3& operator*=(float scalar);
  const arVector3& operator/=(float scalar);

  float magnitude() const;
  float magnitude2() const;

  // dot product.
  float dot( const arVector3& y ) const;

  // C++ normalize() doesn't raise an error if norm == 0.
  arVector3 normalize() const /TransferBack/;
%MethodCode
  const float mag = sipCpp->magnitude();
  if (mag <= 0.) {
    PyErr_SetString( PyExc_ValueError, "arVector3 error: can't normalize zero-length vector." );
    sipIsErr = 1;
  } else {
    float* v = const_cast<float *>(sipCpp->v);
    sipRes = new arVector3( v[0]/mag, v[1]/mag, v[2]/mag );
  }
%End

  // Return string that should be capable of recreating same object if eval-ed.
  string __repr__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << "arVector3" << *sipCpp;
    sipRes = new string( s.str() );
%End

  // Convert to Python list.
  SIP_PYOBJECT toList();
%MethodCode
    sipIsErr = _mathToList( "arVector3", sipCpp->v, 3, &sipRes );
%End

  // Convert to Python tuple.
  SIP_PYOBJECT toTuple();
%MethodCode
    sipIsErr = _mathToTuple( "arVector3", sipCpp->v, 3, &sipRes );
%End

  // Set data from Python sequence.
  void fromSequence( SIP_PYOBJECT a0 );
%MethodCode
    sipIsErr = _mathSetFromSequence( "arVector3", a0, sipCpp->v, 3 );
%End

  // NOTE: this automatically generates __getitem__, NOT __setitem__
  // (and there's no bounds-checking).
  //float operator[] (int);

  // x = foo[i]
  float __getitem__( int a0 );
%MethodCode
    sipIsErr = _mathGetItem( sipCpp->v, a0, 3, &sipRes );
%End

  // foo[i] = x
  void __setitem__( int a0, float a1 );
%MethodCode
    sipIsErr = _mathSetItem( sipCpp->v, a0, 3, a1 );
%End

%PickleCode
    sipRes = Py_BuildValue( (char *)"fff", sipCpp->v[0], sipCpp->v[1], sipCpp->v[2] );
%End

};

// v1*v2
arVector3 operator*(const arVector3&, const arVector3&) /TransferBack/; 

// K*v1
arVector3 operator*( float, const arVector3 &) /TransferBack/;

// v1*K
arVector3 operator*( const arVector3 &, float) /TransferBack/;

// v1/K
arVector3 operator/(const arVector3&, float) /TransferBack/; // scalar division, handles /0

// v1+v2
arVector3 operator+(const arVector3&, const arVector3&) /TransferBack/;

// -v1
arVector3 operator-(const arVector3&) /TransferBack/; // negate

// v1-v2
arVector3 operator-(const arVector3&, const arVector3&) /TransferBack/;


class arVector4 {
 public:
  arVector4();
  arVector4(float x, float y, float z, float w);
  arVector4(const arVector3& vec, float w);

  arVector4( const arVector4& a0 );
%MethodCode
    sipCpp = new arVector4( a0->v );
%End

  // construct from Python sequence.
  arVector4( SIP_PYOBJECT a0 );
%MethodCode
    float args[4];
    sipIsErr = _mathSetFromSequence( "arVector4", a0, args, 4 );
    if (!sipIsErr) {
      sipCpp = new arVector4( const_cast<const float*>(args) );
    }
%End

  ~arVector4();

  //v1==v2
  bool operator==(const arVector4& rhs) const;

  //v1!=v2
  bool operator!=(const arVector4& rhs) const;

  float magnitude() const;
  float magnitude2() const;

  // dot product.
  float dot( const arVector4& y ) const;

  arMatrix4 outerProduct( const arVector4& rhs ) const /TransferBack/;

  // C++ normalize() doesn't raise an error if norm == 0.
  arVector4 normalize() const /TransferBack/;
%MethodCode
  const float mag = sipCpp->magnitude();
  if (mag <= 0.) {
    PyErr_SetString( PyExc_ValueError, "arVector4 error: can't normalize zero-length vector." );
    sipIsErr = 1;
  } else {
    float* v = const_cast<float *>(sipCpp->v);
    sipRes = new arVector4( v[0]/mag, v[1]/mag, v[2]/mag, v[3]/mag );
  }
%End

  // Return simple string representation
  string __str__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << *sipCpp;
    sipRes = new string( s.str() );
%End

  // Return string that should be capable of recreating same object if eval-ed.
  string __repr__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << "arVector4(" << sipCpp->v[0] << "," << sipCpp->v[1] << "," << sipCpp->v[2]
      << "," << sipCpp->v[1] << ")";
    sipRes = new string( s.str() );
%End

  // Convert to Python list.
  SIP_PYOBJECT toList();
%MethodCode
    sipIsErr = _mathToList( "arVector4", sipCpp->v, 4, &sipRes );
%End

  // Convert to Python tuple.
  SIP_PYOBJECT toTuple();
%MethodCode
    sipIsErr = _mathToTuple( "arVector4", sipCpp->v, 4, &sipRes );
%End

  // Set data from Python sequence.
  void fromSequence( SIP_PYOBJECT a0 );
%MethodCode
    sipIsErr = _mathSetFromSequence( "arVector4", a0, sipCpp->v, 4 );
%End

  // NOTE: this automatically generates __getitem__, NOT __setitem__
  // (and there's no bounds-checking).
  //float operator[] (int);

  // x = foo[i]
  float __getitem__( int a0 );
%MethodCode
    sipIsErr = _mathGetItem( sipCpp->v, a0, 4, &sipRes );
%End

  // foo[i] = x
  void __setitem__( int a0, float a1 );
%MethodCode
    sipIsErr = _mathSetItem( sipCpp->v, a0, 4, a1 );
%End

%PickleCode
    sipRes = Py_BuildValue( (char *)"ffff", sipCpp->v[0], sipCpp->v[1], sipCpp->v[2], sipCpp->v[3] );
%End

};


class arMatrix4 {
 public:
  arMatrix4();
  arMatrix4(float,float,float,float,float,float,float,float,
              float,float,float,float,float,float,float,float);
  arMatrix4( const arMatrix4& a0 );
%MethodCode
    sipCpp = new arMatrix4( a0->v );
%End

  ~arMatrix4();
  
  // construct from Python sequence.
  arMatrix4( SIP_PYOBJECT a0 );
%MethodCode
    float args[16];
    sipIsErr = _mathSetFromSequence( "arMatrix4", a0, args, 16 );
    if (!sipIsErr) {
      sipCpp = new arMatrix4( const_cast<const float* const>(args) );
    }
%End

  // To get at arQuaternion::operator arMatrix4();
  arMatrix4( arQuaternion a0 );
%MethodCode
    arMatrix4 m = *a0;
    sipCpp = new arMatrix4( m );
%End

  arMatrix4 inverse() const /TransferBack/;

  // allows Python style ~m (== m.inverse())
  arMatrix4 __invert__() /TransferBack/;
%MethodCode
    sipRes = new arMatrix4( sipCpp->inverse().v );
%End


  arMatrix4 transpose() const /TransferBack/;

  bool operator==(const arMatrix4& rhs) const;
  bool operator!=(const arMatrix4& rhs) const;

  // m**k
  arMatrix4 __pow__( int a0 ) /TransferBack/;
%MethodCode
    arMatrix4 m;
    sipRes = new arMatrix4();
    if (a0<0) {
      m = sipCpp->inverse();
      a0 = -a0;
    } else {
      m = *sipCpp;
    }
    for(int i=0; i<a0; ++i) {
      *sipCpp=(*sipCpp)*m;
    }
%End

  float __abs__();
%MethodCode
    float s(0.);
    float* v = sipCpp->v;
    for(int i=0; i<16; ++i) {
      float x = v[i];
      s += x*x;
    }
    sipRes = sqrt(s);
%End


  string __str__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << *sipCpp;
    sipRes = new string( s.str() );
%End

  // Return string that should be capable of recreating same object if eval-ed.
  string __repr__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << "arMatrix4(" << endl;
    float* v(sipCpp->v);
    for (int i=0; i<16; ++i) {
      s << v[i];
      if (i != 15)
        s << ",";
    }
    s << ")";
    sipRes = new string( s.str() );
%End

  // Convert to Python list.
  SIP_PYOBJECT toList();
%MethodCode
    sipIsErr = _mathToList( "arMatrix4", sipCpp->v, 16, &sipRes );
%End

  // Convert to Python tuple.
  SIP_PYOBJECT toTuple();
%MethodCode
    sipIsErr = _mathToTuple( "arMatrix4", sipCpp->v, 16, &sipRes );
%End

  // Set data from Python sequence.
  void fromSequence( SIP_PYOBJECT a0 );
%MethodCode
    sipIsErr = _mathSetFromSequence( "arMatrix4", a0, sipCpp->v, 16 );
%End

  // x = foo[i]
  float __getitem__( int a0 );
%MethodCode
    sipIsErr = _mathGetItem( sipCpp->v, a0, 16, &sipRes );
%End

  // foo[i] = x
  void __setitem__( int a0, float a1 );
%MethodCode
    sipIsErr = _mathSetItem( sipCpp->v, a0, 16, a1 );
%End

%PickleCode
    sipRes = Py_BuildValue( (char *)"ffffffffffffffff",
               sipCpp->v[0], sipCpp->v[1], sipCpp->v[2], sipCpp->v[3],
               sipCpp->v[4], sipCpp->v[5], sipCpp->v[6], sipCpp->v[7],
               sipCpp->v[8], sipCpp->v[9], sipCpp->v[10], sipCpp->v[11],
               sipCpp->v[12], sipCpp->v[13], sipCpp->v[14], sipCpp->v[15] );
%End

};


arMatrix4 operator*(const arMatrix4&,const arMatrix4&) /TransferBack/; 
arMatrix4 operator+(const arMatrix4&, const arMatrix4&) /TransferBack/;
arMatrix4 operator-(const arMatrix4&) /TransferBack/; //negate
arMatrix4 operator-(const arMatrix4&, const arMatrix4&) /TransferBack/;
arVector3 operator*(const arMatrix4&, const arVector3&) /TransferBack/;


class arQuaternion {
 public:
  arQuaternion();
  arQuaternion(float real, float pure1, float pure2, float pure3);
  arQuaternion(float real, const arVector3& pure);
  arQuaternion( const arQuaternion& a0 );
%MethodCode
    sipCpp = new arQuaternion( a0->real, a0->pure );
%End


  // To get at arMatrix4::operator arQuaternion();
  arQuaternion( arMatrix4 a0 );
%MethodCode
    arQuaternion q = *a0;
    sipCpp = new arQuaternion( q.real, q.pure );
%End

  // construct from Python sequence.
  arQuaternion( SIP_PYOBJECT a0 );
%MethodCode
    float args[4];
    sipIsErr = _mathSetFromSequence( "arQuaternion", a0, args, 4 );
    if (!sipIsErr) {
      sipCpp = new arQuaternion( const_cast<const float* const>(args) );
    }
%End

  ~arQuaternion();

  float magnitude2() const;
  float magnitude() const;
  arQuaternion conjugate() const /TransferBack/;
  arQuaternion inverse() const /TransferBack/;

  float real;
  arVector3 pure;

  string __str__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << *sipCpp;
    sipRes = new string( s.str() );
%End

  // Return string that should be capable of recreating same object if eval-ed.
  string __repr__();
%MethodCode
    ostringstream s(ostringstream::out);
    s << "arQuaternion(" << sipCpp->real << "," << sipCpp->pure << ")";
    sipRes = new string( s.str() );
%End

%PickleCode
    sipRes = Py_BuildValue( (char *)"ffff", sipCpp->real, sipCpp->pure[0], sipCpp->pure[1], sipCpp->pure[2] );
%End

};


arQuaternion operator*(const arQuaternion&, const arQuaternion&) /TransferBack/;
arQuaternion operator*(float, const arQuaternion&) /TransferBack/;
arQuaternion operator*(const arQuaternion&, float) /TransferBack/;
arQuaternion operator/(const arQuaternion&, float) /TransferBack/; // scalar division; all zeros if /0
arQuaternion operator+(const arQuaternion&, const arQuaternion&) /TransferBack/;
arQuaternion operator-(const arQuaternion&) /TransferBack/;
arQuaternion operator-(const arQuaternion&, const arQuaternion&) /TransferBack/;

arVector3 operator*(const arQuaternion&, const arVector3&);

// For Euler angles.
enum arAxisOrder {
	AR_XYZ	= 0x01,
	AR_XZY	= 0x00,
	AR_YZX	= 0x11,
	AR_YXZ	= 0x10,
	AR_ZXY	= 0x21,
	AR_ZYX	= 0x20
  };


bool ar_isPowerOfTwo( int x );

// Matrix creation.
arMatrix4 ar_identityMatrix() /TransferBack/;
arMatrix4 ar_translationMatrix(float,float,float) /TransferBack/;
arMatrix4 ar_translationMatrix(const arVector3&) /TransferBack/;
arMatrix4 ar_rotationMatrix(char,float) /TransferBack/;
arMatrix4 ar_rotationMatrix(const arVector3&,float) /TransferBack/;
arMatrix4 ar_rotateVectorToVector( const arVector3& vec1, 
                                  const arVector3& vec2 ) /TransferBack/;
arMatrix4 ar_transrotMatrix(const arVector3& position, 
                                     const arQuaternion& orientation);
arMatrix4 ar_scaleMatrix(float) /TransferBack/;
arMatrix4 ar_scaleMatrix(float,float,float) /TransferBack/;
arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors) /TransferBack/;
// Abbreviations.
arMatrix4 ar_TM(float x, float y, float z) /TransferBack/;
arMatrix4 ar_TM(const arVector3& v) /TransferBack/;
arMatrix4 ar_RM(char axis, float angle) /TransferBack/;
arMatrix4 ar_RM(const arVector3& axis, float angle) /TransferBack/;
arMatrix4 ar_SM(float scale) /TransferBack/;
arMatrix4 ar_SM(float a, float b, float c) /TransferBack/;
arMatrix4 ar_SM(const arVector3& scaleFactors) /TransferBack/;

// Factor products of rotations, translations and uniform scalings.
arMatrix4 ar_extractTranslationMatrix(const arMatrix4&) /TransferBack/;
arVector3 ar_extractTranslation(const arMatrix4&) /TransferBack/;
arMatrix4 ar_extractRotationMatrix(const arMatrix4&) /TransferBack/;
arMatrix4 ar_extractScaleMatrix(const arMatrix4&) /TransferBack/;

// Abbreviations.
arMatrix4 ar_ETM(const arMatrix4& m) /TransferBack/;
arVector3 ar_ET(const arMatrix4& m) /TransferBack/;
arMatrix4 ar_ERM(const arMatrix4& m) /TransferBack/;
arMatrix4 ar_ESM(const arMatrix4& m) /TransferBack/;

// Utility functions.
float     ar_angleBetween(const arVector3&, const arVector3&);
arVector3 ar_extractEulerAngles(const arMatrix4& m, arAxisOrder o) /TransferBack/;
//arQuaternion ar_angleVectorToQuaternion(const arVector3&,float);
// returns the relected vector of direction across normal.
arVector3 ar_reflect(const arVector3& direction, const arVector3& normal) /TransferBack/;
float ar_intersectRayTriangle(const arVector3& rayOrigin,
                               const arVector3& rayDirection,
                               const arVector3& v1,
                               const arVector3& v2,
                               const arVector3& v3);

// lineDirection & planeNormal need not be normalized (but affects interpretation of range).
// Returns false if no intersection.
// Otherwise, intersection = linePoint + range*lineDirection.
bool ar_intersectLinePlane( const arVector3& linePoint,
                             const arVector3& lineDirection,
                             const arVector3& planePoint,
                             const arVector3& planeNormal,
                             float& range /Out/);

// Point on a line that is nearest to otherPoint.
arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                  const arVector3& lineDirection,
                                  const arVector3& otherPoint,
                                  const float threshold = 1.e-6 ) /TransferBack/;

// Matrix for reflecting in a plane. This matrix should pre-multiply
// the object matrix on the stack (i.e. load this one on first, then multiply
// by the object placement matrix).
arMatrix4 ar_mirrorMatrix( const arVector3& planePoint, const arVector3& planeNormal ) /TransferBack/;

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
arMatrix4 ar_castShadowMatrix( const arMatrix4& objectMatrix,
                                const arVector4& lightPosition,
                                const arVector3& planePoint,
                                const arVector3& planeNormal ) /TransferBack/;


//********** user interface, for mouse->3D

//UNUSED arMatrix4 ar_planeToRotation(float,float) /TransferBack/;

//********** screen

arVector3 ar_tileScreenOffset(const arVector3&,
                              const arVector3&,
          			              float, float, float, int, float, int ) /TransferBack/;
arMatrix4 ar_frustumMatrix( const arVector3& screenCenter,
			                      const arVector3& screenNormal,
                             const arVector3& screenUp,
                             const float halfWidth, 
                             const float halfHeight,
                             const float nearClip, const float farClip,
                             const arVector3& eyePosition ) /TransferBack/;
arMatrix4 ar_frustumMatrix( const float screenDist,
                             const float halfWidth, 
                             const float halfHeight,
                             const float nearClip, const float farClip,
                             const arVector3& locEyePosition ) /TransferBack/;
arMatrix4 ar_lookatMatrix( const arVector3& viewPosition,
                           const arVector3& lookatPosition,
                           const arVector3& up ) /TransferBack/;

