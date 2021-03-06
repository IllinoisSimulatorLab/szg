//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMath.h"

//***********************
// vector not-inlined
//***********************

ostream& operator<<(ostream& os, const arVector2& x) {
  os <<"( "<<x.v[0]<<", "<< x.v[1]<<" )";
  return os;
}

ostream& operator<<(ostream& os, const arVector3& x) {
  os <<"( "<<x.v[0]<<", "<< x.v[1]<<", "<<x.v[2]<<" )";
  return os;
}

ostream& operator<<(ostream& os, const arVector4& x) {
  os <<"( "<<x.v[0]<<", "<< x.v[1]<<", "<<x.v[2]<<", "<<x.v[3]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector2& x) {
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector3& x) {
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector4& x) {
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" "<<x.v[3]<<" )";
  return os;
}


arMatrix4 arVector4::outerProduct( const arVector4& rhs ) const {
  arMatrix4 lhs;
  float* plhs = lhs.v;
  for (unsigned i=0; i<4; ++i) {
    const float y = rhs.v[i];
    for (unsigned j=0; j<4; ++j) {
      *plhs++ = this->v[j] * y;
    }
  }
  return lhs;
}

//***********************
// matrix not-inlined
//***********************

arMatrix4::arMatrix4() {
  // Default to ar_identityMatrix().
  memset(v, 0, sizeof(v));
  v[0] = v[5] = v[10] = v[15] = 1.;
}

arMatrix4::arMatrix4(const float* const matrixRef) {
  memcpy(v, matrixRef, 16 * sizeof(float));
}

arMatrix4::arMatrix4(float v0, float v4, float v8, float v12,
                    float v1, float v5, float v9, float v13,
                    float v2, float v6, float v10, float v14,
                    float v3, float v7, float v11, float v15) {
  v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
  v[4] = v4; v[5] = v5; v[6] = v6; v[7] = v7;
  v[8] = v8; v[9] = v9; v[10] = v10; v[11] = v11;
  v[12] = v12; v[13] = v13; v[14] = v14; v[15] = v15;
}

// matrix inverse
arMatrix4 arMatrix4::inverse() const {
  int i=0, j=0;
  float buffer[4][8];

  // Prepare the Gaussian elimination.
  for (i=0; i<4; i++) {
    for (j=0; j<4; j++)
      buffer[i][j] = v[i+4*j];
    for (; j<8; j++)
      buffer[i][j] = (i+4 == j) ? 1. : 0.;
  }

  // Traverse the columns in order.
  for (i=0; i<4; i++) {
    if (fabs(buffer[i][i])==0) {
      // swap rows
      int which = i+1;
      while (which<4) {
        if (fabs(buffer[which][i]) != 0)
          break;
        ++which;
      }
      if (which==4) {
        // singular
        return arMatrix4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
      }

      for (j=0;j<8;j++) {
        const float temp = buffer[i][j];
        buffer[i][j] = buffer[which][j];
        buffer[which][j] = temp;
      }
    }
    // make buffer[i][i] == 1
    const float temp = buffer[i][i];
    for (j=0; j<8; j++) {
      buffer[i][j] /= temp;
    }
    // zero rest of column
    for (int k=0; k<4; k++) {
      if (k!=i) {
        const float scale = buffer[k][i];
        for (j=0; j<8; j++) {
          buffer[k][j] -= scale*buffer[i][j];
        }
      }
    }
  }
  arMatrix4 out;
  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      out.v[i+4*j] = buffer[i][4+j];
  return out;
}

arMatrix4 arMatrix4::transpose() const {
  return arMatrix4( v[0], v[1], v[2], v[3],
                    v[4], v[5], v[6], v[7],
                    v[8], v[9], v[10], v[11],
                    v[12], v[13], v[14], v[15] );
}

// matrix multiply
// todo: define operator*= as well!
arMatrix4 operator*(const arMatrix4& A, const arMatrix4& B) {
  arMatrix4 C;
  for (int i=0; i<4; i++)
    for (int j=0; j<4; j++) {
      C.v[4*j+i] = A.v[i]*B.v[4*j] + A.v[i+4]*B.v[4*j+1] +
                   A.v[i+8]*B.v[4*j+2] + A.v[i+12]*B.v[4*j+3];
      }
  return C;
}

arMatrix4 operator!(const arMatrix4& in) {
  return in.inverse();
}

arMatrix4::operator arQuaternion() const {
  const float trace = v[0]+v[5]+v[10]+1.;
  if (trace > 1e-8) {
    const float scale = 0.5 / sqrt(trace);
    return arQuaternion( 0.25/scale,
                         (v[6]-v[9])*scale,
                         (v[8]-v[2])*scale,
                         (v[1]-v[4])*scale );
  }

  if ((v[0]>v[5])&&(v[0]>v[10]))  {        // Column 0:
    if (1.+v[0]-v[5]-v[10] <= 0) {
      // Error.
      return arQuaternion(0, 0, 0, 1);
    }
    const float scale  = 0.5/sqrt( 1.+v[0]-v[5]-v[10] ) ;
    return arQuaternion( (v[6]-v[9])*scale,
                          0.25/scale,
                          (v[1]+v[4])*scale,
                          (v[8]+v[2])*scale );
  }

  if ( v[5] > v[10] ) {                        // Column 1:
    if (1.+v[5]-v[0]-v[10] <= 0) {
      // Error
      return arQuaternion(0, 0, 0, 1);
    }
    const float scale = 0.5/sqrt( 1.+v[5]-v[0]-v[10] );
    return arQuaternion( (v[8]-v[2] )*scale,
                         (v[1]+v[4] )*scale,
                         0.25/scale,
                         (v[6]+v[9] )*scale );
  }
  // Column 2
  if (1.+v[10]-v[0]-v[5] <= 0) {
    // Error
    return arQuaternion(0, 0, 0, 1);
  }
  const float scale = 0.5/sqrt( 1.+v[10]-v[0]-v[5] );
  return arQuaternion( (v[1]-v[4])*scale,
                       (v[8]+v[2])*scale,
                       (v[6]+v[9])*scale,
                       0.25/scale );
}

istream& operator>>(istream& is, arMatrix4& x) {
  is >>x.v[0]     >>x.v[4]     >>x.v[8 ]    >>x.v[12]
     >>x.v[1]     >>x.v[5]     >>x.v[9 ]    >>x.v[13]
     >>x.v[2]     >>x.v[6]     >>x.v[10]    >>x.v[14]
     >>x.v[3]     >>x.v[7]     >>x.v[11]    >>x.v[15];
  return is;
}

ostream& operator<<(ostream& os, const arMatrix4& x) {
  os <<x.v[0]<<" "<<x.v[4]<<" "<<x.v[8 ]<<" "<<x.v[12]<<"\n"
     <<x.v[1]<<" "<<x.v[5]<<" "<<x.v[9 ]<<" "<<x.v[13]<<"\n"
     <<x.v[2]<<" "<<x.v[6]<<" "<<x.v[10]<<" "<<x.v[14]<<"\n"
     <<x.v[3]<<" "<<x.v[7]<<" "<<x.v[11]<<" "<<x.v[15]<<"\n";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arMatrix4& x) {
  os <<x.v[0]<<" "<<x.v[4]<<" "<<x.v[8 ]<<" "<<x.v[12]<<"\n"
     <<x.v[1]<<" "<<x.v[5]<<" "<<x.v[9 ]<<" "<<x.v[13]<<"\n"
     <<x.v[2]<<" "<<x.v[6]<<" "<<x.v[10]<<" "<<x.v[14]<<"\n"
     <<x.v[3]<<" "<<x.v[7]<<" "<<x.v[11]<<" "<<x.v[15]<<"\n";
  return os;
}

//*************************
// quaternion not-inlined
//*************************

arQuaternion::arQuaternion() :
  real(1),
  pure(arVector3(0, 0, 0))
{
}

arQuaternion::arQuaternion(float x, float a, float b, float c) :
  real(x),
  pure(arVector3(a, b, c))
{
}

arQuaternion::arQuaternion(float x, const arVector3& y) :
  real(x),
  pure(y)
{
}

arQuaternion::arQuaternion( const float* numAddress ) :
  real(*numAddress),
  pure(numAddress+1)
{
}

arQuaternion arQuaternion::conjugate() const {
  return arQuaternion( real, -pure );
}

arQuaternion arQuaternion::normalize() const {
  return *this / this->magnitude();
}

arQuaternion arQuaternion::inverse() const {
  const float m = magnitude2();
  return (m < 1e-6) ?
    arQuaternion(0, 0, 0, 0) :
    conjugate() / m;
}

arQuaternion::operator arMatrix4() const {
  const float aa = real*real;
  const float b1b1 = pure.v[0]*pure.v[0];
  const float b2b2 = pure.v[1]*pure.v[1];
  const float b3b3 = pure.v[2]*pure.v[2];
  const float ab12 = 2*real*pure.v[0];
  const float ab22 = 2*real*pure.v[1];
  const float ab32 = 2*real*pure.v[2];
  const float b1b2 = 2*pure.v[0]*pure.v[1];
  const float b2b3 = 2*pure.v[1]*pure.v[2];
  const float b1b3 = 2*pure.v[0]*pure.v[2];
  return arMatrix4(
    aa+b1b1-b2b2-b3b3, -ab32+b1b2,        ab22+b1b3, 0,
    ab32+b1b2,         aa+b2b2-b1b1-b3b3, -ab12+b2b3, 0,
    -ab22+b1b3,        ab12+b2b3,         aa+b3b3-b1b1-b2b2, 0,
    0, 0, 0, 1);
}

arEulerAngles::arEulerAngles( const arAxisOrder& ord, const arVector3& angs ) :
    _angles(angs) {
  setOrder( ord );
}

void arEulerAngles::setOrder( const arAxisOrder& ord ) {
  if (ord & 0x20) {
    _initialAxis = AR_Z_AXIS;
  } else if (ord & 0x10) {
    _initialAxis = AR_Y_AXIS;
  } else {
    _initialAxis = AR_X_AXIS;
  }
  _parityEven = !!(ord & 0x1);
}

arAxisOrder arEulerAngles::getOrder() const {
  int ord = _parityEven;
  switch(_initialAxis) {
    case AR_Y_AXIS:
      ord |= 0x10;
      break;
    case AR_Z_AXIS:
      ord |= 0x20;
      break;
    case AR_X_AXIS:
      break;
  }
  return static_cast<arAxisOrder>(ord);
}

void arEulerAngles::angleOrder( arAxisName& i, arAxisName& j, arAxisName& k ) const {
  const int ii = _initialAxis;
  const int jj = _parityEven ? (_initialAxis+1)%3 : (_initialAxis > 0 ? _initialAxis-1 : 2);
  const int kk = _parityEven ? (_initialAxis > 0 ? _initialAxis-1 : 2) : (_initialAxis+1)%3;
  i = static_cast<arAxisName>(ii);
  j = static_cast<arAxisName>(jj);
  k = static_cast<arAxisName>(kk);
}

arVector3 arEulerAngles::extract( const arMatrix4& mat ) {
  arAxisName i, j, k;
  angleOrder( i, j, k );

  arMatrix4 M(mat.inverse());
#define AR_MATRIX4_INDEX(i, j) (4*(i) + (j))

  // Extract the first angle, x.
  const float x = atan2( M[AR_MATRIX4_INDEX(j, k)], M[AR_MATRIX4_INDEX(k, k)] );

  // Remove x from M, so the remaining rotation N is only around two axes,
  // preventing gimbal lock.
  arVector3 r;
  r[i] = _parityEven ? -1. : 1.;
  arMatrix4 N(M * ar_rotationMatrix( r, x ));

  // Extract from N the other two angles, y and z.
  const float cy = sqrt(  N[AR_MATRIX4_INDEX(i, i)] * N[AR_MATRIX4_INDEX(i, i)] +
                          N[AR_MATRIX4_INDEX(i, j)] * N[AR_MATRIX4_INDEX(i, j)] );
  const float y = atan2( -N[AR_MATRIX4_INDEX(i, k)], cy );
  const float z = atan2( -N[AR_MATRIX4_INDEX(j, i)],  N[AR_MATRIX4_INDEX(j, j)] );

#undef AR_MATRIX4_INDEX
  _angles = arVector3(x, y, z);
  if (_parityEven)
    _angles *= -1;
  return _angles;
}

arMatrix4 arEulerAngles::toMatrix() const {
  arAxisName i, j, k;
  angleOrder( i, j, k );
  return
    ar_rotationMatrix( i, _angles.v[0] ) *
    ar_rotationMatrix( j, _angles.v[1] ) *
    ar_rotationMatrix( k, _angles.v[2] );
}


ostream& operator<<(ostream& os, const arQuaternion& x) {
  os<<"("<<x.real<<" "<<x.pure.v[0]<<" "<<x.pure.v[1]<<" "<<x.pure.v[2]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arQuaternion& x) {
  os<<"("<<x.real<<" "<<x.pure.v[0]<<" "<<x.pure.v[1]<<" "<<x.pure.v[2]<<" )";
  return os;
}

//*************************
// general
//*************************

bool ar_isPowerOfTwo( int i ) {
  return i > 0 && (i & (i - 1)) == 0;
}

// Deprecated: use arMatrix4(), since a constructor needs to be called anyways.
arMatrix4 ar_identityMatrix() {
  return arMatrix4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
}

arMatrix4 ar_translationMatrix(float x, float y, float z) {
  arMatrix4 lhs;
  lhs.v[12] = x;
  lhs.v[13] = y;
  lhs.v[14] = z;
  return lhs;
}

arMatrix4 ar_translationMatrix(const arVector3& v) {
  arMatrix4 lhs;
  memcpy(&lhs.v[12], &v.v[0], 3 * sizeof(float));
  return lhs;
}

// angles are all in radians

arMatrix4 ar_rotationMatrix(char axis, float r) {
  const float sr = sin(r);
  const float cr = cos(r);
  switch (axis) {
  case 'x':
    return arMatrix4(
      1,  0,   0, 0,
      0, cr, -sr, 0,
      0, sr,  cr, 0,
      0,  0,   0, 1);
  case 'y':
    return arMatrix4(
       cr, 0, sr, 0,
        0, 1,  0, 0,
      -sr, 0, cr, 0,
        0, 0,  0, 1);
  case 'z':
    return arMatrix4(
      cr, -sr, 0, 0,
      sr,  cr, 0, 0,
       0,   0, 1, 0,
       0,   0, 0, 1);
  }
  ar_log_error() << "ar_rotationMatrix: unknown axis '" << axis << "'.\n";
  return arMatrix4();
}

arMatrix4 ar_rotationMatrix( arAxisName a, float r) {
  return ar_rotationMatrix(a - AR_X_AXIS + 'x', r);
}

arMatrix4 ar_rotationMatrix(const arVector3& a, float radians) {
  radians *= .5;
  const arVector3 axis = a.zero() ? arVector3(1, 0, 0) : a;
  const arQuaternion q(cos(radians), (sin(radians)/axis.magnitude()) * axis);
  const arVector3 rotX(q*arVector3(1, 0, 0));
  const arVector3 rotY(q*arVector3(0, 1, 0));
  const arVector3 rotZ(q*arVector3(0, 0, 1));
  return arMatrix4(
                   rotX.v[0], rotY.v[0], rotZ.v[0], 0,
                   rotX.v[1], rotY.v[1], rotZ.v[1], 0,
                   rotX.v[2], rotY.v[2], rotZ.v[2], 0,
                   0, 0, 0, 1);
}

arMatrix4 ar_transrotMatrix(const arVector3& position, const arQuaternion& orientation) {
  return ar_translationMatrix( position ) * arMatrix4( orientation );
}

arMatrix4 ar_scaleMatrix(float s) {
  return ar_scaleMatrix(s, s, s);
}

arMatrix4 ar_scaleMatrix(float x, float y, float z) {
  arMatrix4 lhs;
  lhs[0] = x;
  lhs[5] = y;
  lhs[10] = z;
  return lhs;
}

arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors) {
  return ar_scaleMatrix(
    scaleFactors.v[0],
    scaleFactors.v[1],
    scaleFactors.v[2]);
}

arMatrix4 ar_extractTranslationMatrix(const arMatrix4& rhs) {
  arMatrix4 lhs;
  memcpy(&lhs.v[12], &rhs.v[12], 3 * sizeof(float));
  return lhs;
}

arVector3 ar_extractTranslation(const arMatrix4& rhs) {
  return arVector3(rhs.v + 12);
}

arMatrix4 ar_extractRotationMatrix(const arMatrix4& rhs) {
  arMatrix4 lhs;
  for (int i=0; i<3; ++i) {
    // i'th column of rhs's top-left 3x3 submatrix.
    const arVector3 column(&rhs.v[4*i]);
    const float magnitude = column.magnitude();
    if (magnitude > 0.) {
      // i'th column of lhs's top-left 3x3 submatrix.
      lhs.v[4*i  ] = rhs.v[4*i  ] / magnitude;
      lhs.v[4*i+1] = rhs.v[4*i+1] / magnitude;
      lhs.v[4*i+2] = rhs.v[4*i+2] / magnitude;
    }
    else{
      lhs.v[5*i] = 0.;
    }
  }
  return lhs;
}

arMatrix4 ar_extractScaleMatrix(const arMatrix4& rhs) {
  arMatrix4 lhs;
  for (int i=0; i<3; ++i) {
    lhs.v[5*i] = arVector3(&rhs.v[4*i]).magnitude();
  }
  return lhs;
}

// Nonnegative radians, from first to second (counterclockwise),
// around the vector first crossproduct second.
float ar_angleBetween(const arVector3& first, const arVector3& second) {
  if (first.magnitude() <=0. || second.magnitude() <= 0.)
    return 0.;

  // Rounding error might produce a dot product outside [-1, 1].
  // Clamp it, so acos() doesn't return nan.
  double dotProd = (first/first.magnitude()).dot(second/second.magnitude());
  if (dotProd > 1.) {
    dotProd = 1.;
  } else if (dotProd < -1.) {
    dotProd = -1.;
  }
  return (float)acos(dotProd);
}

// Euler angles in radians
arVector3 ar_extractEulerAngles(const arMatrix4& m, arAxisOrder o) {
  return arEulerAngles(o).extract(m);
}

arQuaternion ar_angleVectorToQuaternion(const arVector3& a, float radians) {
  const arVector3 axis = a.zero() ? arVector3(1, 0, 0) : a;
  radians *= .5;
  return arQuaternion(cos(radians), (sin(radians) / (++axis)) * axis);
}

// Reflect direction across a (normal) vector.
arVector3 ar_reflect(const arVector3& direction, const arVector3& normal) {
  const float mag2 = normal.magnitude2();
  return (mag2 < 1e-6) ? direction : // lame error handling
    direction - (2 * direction % normal / mag2) * normal;
}

float ar_intersectRayTriangle(const arVector3& rayOrigin,
                              const arVector3& rayDirection,
                              const arVector3& vertex1,
                              const arVector3& vertex2,
                              const arVector3& vertex3) {

  // Algorithm from geometrysurfer.com by Dan Sunday
  const arVector3 u(vertex2 - vertex1);
  const arVector3 v(vertex3 - vertex1);
  const arVector3 n(u*v);
  if (n.zero()) {
    // Degenerate triangle.
    return -1.;
  }
  const arVector3 rayDir( rayDirection.normalize() );
  const float b = n % rayDir;
  if (fabs(b) < 0.000001) {
    // Ray hits triangle edge-on.  No intersection.
    return -1.;
  }
  const arVector3 w0( rayOrigin - vertex1 );
  const float a = n % w0;
  const float r = -a / b;
  if (r < 0) {
    // Ray diverges from triangle.  No intersection.
    return -1.;
  }
  // Point where ray meets triangle's plane.
  const arVector3 intersect( rayOrigin + r*rayDir );

  // Is the intersection inside the triangle?
  const float uu = u % u;
  const float uv = u % v;
  const float vv = v % v;
  const arVector3 w(intersect - vertex1);
  const float wu = w % u;
  const float wv = w % v;
  const float D = uv * uv - uu * vv;
  if (fabs(D) < 0.000001) {
    // error
    return -1.;
  }
  const float s = (uv * wv - vv * wu) / D;
  if (s < 0. || s > 1.) {
    return -1.;
  }
  const float t = (uv * wu - uu * wv) / D;
  return (t < 0. || s+t > 1.) ? -1. : (r*rayDir).magnitude();
}

bool ar_intersectLinePlane( const arVector3& linePoint,
                            const arVector3& lineDirection,
                            const arVector3& planePoint,
                            const arVector3& planeNormal,
                            float& range ) {
  const float denominator = planeNormal.dot( lineDirection );
  if (fabs(denominator) < 1e-10) // NOTE: what's the best threshold?
    return false;
  range = planeNormal.dot( planePoint - linePoint )/denominator;
  return true;
}

// Find the point on a line that is nearest to another point.
arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                 const arVector3& lineDirection,
                                 const arVector3& otherPoint,
                                 const float threshold ) {
  const arVector3 V(otherPoint - linePoint);
  const arVector3 tmp(V * lineDirection);
  // avoid expensive sqrt()
  if (tmp.magnitude2() < threshold*threshold)
    // Point is near line, so force it onto line lest normalize() become unstable.
    return otherPoint;

  const arVector3 N(tmp.normalize());
  const arVector3 M((lineDirection * N).normalize());
  return otherPoint - (V % M)*M;
}

//arMatrix4 ar_mirrorMatrix( const arMatrix4& placementMatrix ) {
//  arMatrix4 reflect( arMatrix4() );
//  reflect.v[10] = -1.;
//  return placementMatrix.inverse() * reflect * placementMatrix;
//}

arMatrix4 ar_mirrorMatrix( const arVector3& planePoint, const arVector3& planeNormal ) {
  arVector4 v1( planeNormal.v[0], planeNormal.v[1], planeNormal.v[2], 0. );
  const float d = -planeNormal.dot( planePoint );
  arVector4 v2( 2.*planeNormal.v[0], 2.*planeNormal.v[1], 2.*planeNormal.v[2], 2.*d );
  return arMatrix4() - v1.outerProduct( v2 );
}

arMatrix4 ar_castShadowMatrix( const arMatrix4& objectMatrix,
                               const arVector4& lightPosition,
                               const arVector3& planePoint,
                               const arVector3& planeNormal ) {
  const arMatrix4 m(objectMatrix.inverse());
  const arVector3 lightPos( m * arVector3( lightPosition.v ) );
  const arVector4 lightVec( lightPos, lightPosition.v[3] );
  const arVector3 point( m * planePoint );
  const arVector3 normal( ar_ERM(m) * planeNormal);
  const arVector4 planeParams( normal, -normal.dot( point ) );
  const arMatrix4 outer( lightVec.outerProduct( planeParams ) );
  const float dot = lightVec.dot( planeParams );
  arMatrix4 dotMatrix( ar_scaleMatrix( dot ) );
  dotMatrix.v[15] = dot;
  return dotMatrix - outer;
}


arMatrix4 ar_rotateVectorToVector( const arVector3& vec1, const arVector3& vec2 ) {
  const float mag1 = vec1.magnitude();
  const float mag2 = vec2.magnitude();
  if (mag1==0. || mag2==0.) {
    ar_log_error() << "ar_rotateVectorToVector: zero input.\n";
    return arMatrix4();
  }

  const arVector3 rotAxis(vec1 * vec2);
  const float mag = rotAxis.magnitude();
  if (mag<1.e-6) {
    return (vec1.dot(vec2) >= 0.) ? arMatrix4() :
      ar_scaleMatrix(-1.); // cheat
  }

  return ar_rotationMatrix(rotAxis/mag, acos((vec1/mag1) % (vec2/mag2)));
}

#ifdef UNUSED
arMatrix4 ar_planeToRotation(float posX, float posY) {
  // Special case.
  if (posX == 0. && posY == 0.)
    return arMatrix4();

  // Map this patch of the plane to the Riemann sphere.
  const float t = -8. / (posX*posX + posY*posY);
  const arVector3 spherePos(arVector3(-t*posX/2., -t*posY/2., t+1.).normalize());

  // Rotate (0, 0, -1) to spherePos.
  return ar_rotateVectorToVector( arVector3(0, 0, -1), spherePos );
}
#endif

// In the screen-related math functions, screen normal points
// from the observer to the screen (i.e. along the ray of vision)

// Calculate the offset vector from the overall screen center to
// the center of an individual tile.  Tiles are in a rectangular,
// planar grid covering the screen, tile (0, 0) at lower left.
arVector3 ar_tileScreenOffset(const arVector3& screenNormal,
                              const arVector3& screenUp,
                              float width, float height,
                              float xTile, int nxTiles,
                              float yTile, int nyTiles) {
  if (nxTiles == 0 || nyTiles == 0 || screenNormal.zero() || screenUp.zero()) {
    // Invalid arguments.
    return arVector3(0, 0, 0);
  }

  const arVector3 zHat(screenNormal.normalize());
  const arVector3 xHat(zHat * screenUp.normalize());
  const arVector3 yHat(xHat * zHat);

  const float tileWidth = width/nxTiles;
  const float tileHeight = height/nyTiles;
  return (-0.5*width  + 0.5*tileWidth  + xTile*tileWidth ) * xHat +
         (-0.5*height + 0.5*tileHeight + yTile*tileHeight) * yHat;
}

arMatrix4 ar_frustumMatrix( const arVector3& screenCenter,
                            const arVector3& screenNormal,
                            const arVector3& screenUp,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& eyePosition ) {
  if (screenNormal.zero() || screenUp.zero()) {
LAbort:
    return arMatrix4();
  }

  const arVector3 zHat(screenNormal.normalize());
  const arVector3 xHat(zHat * screenUp.normalize());
  const arVector3 yHat(xHat * zHat);

  const arVector3 rightEdge(screenCenter + halfWidth * xHat);
  const arVector3 leftEdge(screenCenter - halfWidth * xHat);
  const arVector3 topEdge(screenCenter + halfHeight * yHat);
  const arVector3 botEdge(screenCenter - halfHeight * yHat);

  // float zEye = (eyePosition - headPosition) % zHat;
  const float screenDistance = ( screenCenter - eyePosition ) % zHat;
  if (screenDistance == 0.)
    goto LAbort;

  const float nearFrust = nearClip;
  const float distScale = nearFrust / screenDistance;
  const float rightFrust = distScale*(( rightEdge - eyePosition ) % xHat);
  const float leftFrust = distScale*(( leftEdge - eyePosition ) % xHat);
  const float topFrust = distScale*(( topEdge - eyePosition ) % yHat);
  const float botFrust = distScale*(( botEdge - eyePosition ) % yHat);
  const float farFrust = screenDistance + farClip;

  if (rightFrust == leftFrust || topFrust == botFrust || nearFrust == farFrust)
    goto LAbort;

  // workaround for g++ 2.96 bug
  const float funnyElement = (nearFrust+farFrust) / (nearFrust-farFrust);

  return arMatrix4((2*nearFrust)/(rightFrust-leftFrust), 0., (rightFrust+leftFrust)/(rightFrust-leftFrust), 0.,
                   0., (2*nearFrust)/(topFrust-botFrust), (topFrust+botFrust)/(topFrust-botFrust), 0.,
                   0., 0., funnyElement, 2*nearFrust*farFrust/(nearFrust-farFrust),
                   0., 0., -1., 0. );
}

// in this version, eyePosition has been multiplied by
// ar_lookatMatrix( screenCenter, screenNormal, screenUp )
// screenDistance is the distance from the local coordinate system to the image plane
arMatrix4 ar_frustumMatrix( const float screenDist,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& locEyePosition ) {
  const float screenDistance = screenDist + locEyePosition.v[2];
  if (screenDistance == 0.) {
LAbort:
    return arMatrix4();
  }

  const float nearFrust = nearClip;
  const float distScale = nearFrust / screenDistance;
  const float rightFrust = distScale*( halfWidth - locEyePosition.v[0] );
  const float leftFrust = distScale*( -halfWidth - locEyePosition.v[0] );
  const float topFrust = distScale*( halfHeight - locEyePosition.v[1] );
  const float botFrust = distScale*( -halfHeight - locEyePosition.v[1] );
  const float farFrust = screenDistance + farClip;

  if (rightFrust == leftFrust || topFrust == botFrust || nearFrust == farFrust)
    goto LAbort;

  // workaround for g++ 2.96 bug
  float funnyElement = (nearFrust+farFrust) / (nearFrust-farFrust);

  return arMatrix4((2*nearFrust)/(rightFrust-leftFrust), 0., (rightFrust+leftFrust)/(rightFrust-leftFrust), 0.,
                   0., (2*nearFrust)/(topFrust-botFrust), (topFrust+botFrust)/(topFrust-botFrust), 0.,
                   0., 0., funnyElement, 2*nearFrust*farFrust/(nearFrust-farFrust),
                   0., 0., -1., 0. );
}

// Like gluLookAt(), transform the frame (a, b, c) to (x, y, z), where
//   c = unit vector pointing from lookatPosition to viewPosition
//   b = unit vector along the portion of up orthogonal to c
//   a = b cross c
// In OpenGL, the eye looks in the -z direction.
arMatrix4 ar_lookatMatrix( const arVector3& viewPosition,
                           const arVector3& lookatPosition,
                           const arVector3& up ) {
  if ((viewPosition - lookatPosition).zero()) {
    ar_log_error() << "ar_lookatMatrix myopically degenerate.\n";
    return arMatrix4();
  }
  // Unit vector from lookatPosition to viewPosition.
  // Positive z after the transform.
  const arVector3 Lhat( (viewPosition - lookatPosition).normalize() );

  // Positive y after the transform.
  const arVector3 Uhat( (up - (up % Lhat)*Lhat).normalize() );

  // Positive x after the transform.
  const arVector3 Xhat( Uhat * Lhat );

  const arMatrix4 look( Xhat.v[0], Xhat.v[1], Xhat.v[2], 0.,
                        Uhat.v[0], Uhat.v[1], Uhat.v[2], 0.,
                        Lhat.v[0], Lhat.v[1], Lhat.v[2], 0.,
                        0., 0., 0., 1. );
  return look * ar_translationMatrix( -viewPosition );
}

// Uniform (0.0, 1.0) pseudorandom number generater "ran1" from Numerical Recipes in C.
// Ugly but more trustworthy than rand() (whose RAND_MAX is too small in VC++).
// small under MS VC++). Not recommended for sequences longer than 10^8.
// Initialize by setting *idum to a negative value.

float ar_randUniformFloat(long* idum)
{
  const long IA = 16807;
  const long IM = 2147483647;
  const float AM = 1.0 / IM;
  const long IQ = 127773;
  const long IR = 2836;
  const int NTAB = 32;
  const long NDIV = 1 + (IM-1)/NTAB;
  const float EPS = 1.2e-7;
  const float RNMX = 1.0 - EPS;
  int j;
  long k;
  static long iy=0;
  static long iv[NTAB];

  if (*idum <= 0 || !iy) {
    if (-*idum < 1)
      *idum=1;
    else
      *idum = -*idum;
    for (j=NTAB+7;j>=0;j--) {
      k=(*idum)/IQ;
      *idum=IA*(*idum-k*IQ)-IR*k;
      if (*idum < 0)
        *idum += IM;
      if (j < NTAB)
        iv[j] = *idum;
    }
    iy=iv[0];
  }
  k=(*idum)/IQ;
  *idum=IA*(*idum-k*IQ)-IR*k;
  if (*idum < 0) *idum += IM;

  j=iy/NDIV;
  iy=iv[j];
  iv[j] = *idum;
  const float temp = AM * iy;
  return (temp > RNMX) ? RNMX : temp;
}

int ar_randUniformInt(long* idum, int lo, int hi) {
  return int(floor( ar_randUniformFloat(idum) * (hi-lo+1) ) + lo);
}


bool ar_unpackIntVector( vector<int>& vec, int** p ) {
  *p = new int[vec.size()];
  if (!p) {
    ar_log_error() << "ar_unpackIntVector failed to allocate buffer.\n";
    return false;
  }
  std::copy( vec.begin(), vec.end(), *p );
  return true;
}

bool ar_unpackVector3Vector( vector<arVector3>& vec, float** p ) {
  *p = new float[3*vec.size()];
  if (!p) {
    ar_log_error() << "ar_unpackVector3Vector failed to allocate buffer.\n";
    return false;
  }
  vector<arVector3>::const_iterator iter;
  float *pp = *p;
  for (iter = vec.begin(); iter != vec.end(); ++iter) {
    memcpy( pp, iter->v, 3*sizeof(float) );
    pp += 3;
  }
  return true;
}

bool ar_unpackVector2Vector( vector<arVector2>& vec, float** p ) {
  *p = new float[2*vec.size()];
  if (!p) {
    ar_log_error() << "ar_unpackVector2Vector failed to allocate buffer.\n";
    return false;
  }
  vector<arVector2>::const_iterator iter;
  float *pp = *p;
  for (iter = vec.begin(); iter != vec.end(); ++iter) {
    memcpy( pp, iter->v, 2*sizeof(float) );
    pp += 2;
  }
  return true;
}

bool ar_unpackVector4Vector( vector<arVector4>& vec, float** p ) {
  *p = new float[4*vec.size()];
  if (!p) {
    ar_log_error() << "ar_unpackVector4Vector failed to allocate buffer.\n";
    return false;
  }
  vector<arVector4>::const_iterator iter;
  float *pp = *p;
  for (iter = vec.begin(); iter != vec.end(); ++iter) {
    memcpy( pp, iter->v, 4*sizeof(float) );
    pp += 4;
  }
  return true;
}



