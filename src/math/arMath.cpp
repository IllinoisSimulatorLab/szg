//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMath.h"

//***********************
// vector not-inlined
//***********************

ostream& operator<<(ostream& os, const arVector2& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" )";
  return os;
}

ostream& operator<<(ostream& os, const arVector3& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" )";
  return os;
}

ostream& operator<<(ostream& os, const arVector4& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" "<<x.v[3]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector2& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector3& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arVector4& x){
  os <<"( "<<x.v[0]<<" "<< x.v[1]<<" "<<x.v[2]<<" "<<x.v[3]<<" )";
  return os;
}


arMatrix4 arVector4::outerProduct( const arVector4& rhs ) const {
  arMatrix4 result;
  float* ptr = result.v;
  for (unsigned int i=0; i<4; ++i) {
    const float y = rhs.v[i];
    for (unsigned int j=0; j<4; ++j) {
      *ptr++ = this->v[j]*y;
    }
  }
  return result;
}

//***********************
// matrix not-inlined
//***********************

arMatrix4::arMatrix4(){
  // default to the identity matrix
  memset(v, 0, sizeof(v));
  v[0] = 1.;
  v[5] = 1.;
  v[10] = 1.;
  v[15] = 1.;
}

arMatrix4::arMatrix4(const float* const matrixRef){
  memcpy(v, matrixRef, 16 * sizeof(float));
}

arMatrix4::arMatrix4(float v0, float v4, float v8, float v12,
		    float v1, float v5, float v9, float v13,
		    float v2, float v6, float v10, float v14,
		    float v3, float v7, float v11, float v15){
  v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
  v[4] = v4; v[5] = v5; v[6] = v6; v[7] = v7;
  v[8] = v8; v[9] = v9; v[10] = v10; v[11] = v11;
  v[12] = v12; v[13] = v13; v[14] = v14; v[15] = v15;
}

// matrix inverse
arMatrix4 arMatrix4::inverse() const {
  int i=0,j=0;
  float buffer[4][8];

  // Prepare the Gaussian elimination.
  for (i=0; i<4; i++) {
    for (j=0; j<4; j++)
      buffer[i][j] = v[i+4*j];
    for (; j<8; j++)
      buffer[i][j] = (i+4 == j) ? 1. : 0.;
  }

  // Traverse the columns in order.
  for (i=0; i<4; i++){
    if (fabs(buffer[i][i])==0){
      // swap rows
      int which = i+1;
      while (which<4) {
        if (fabs(buffer[which][i]) != 0)
	  break;
	++which;
      }
      if (which==4){
        // not invertible
        return arMatrix4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
      }

      for (j=0;j<8;j++){
        const float temp = buffer[i][j];
        buffer[i][j] = buffer[which][j];
        buffer[which][j] = temp;
      }
    }
    // make buffer[i][i] == 1
    const float temp = buffer[i][i];
    for (j=0; j<8; j++){
      buffer[i][j] /= temp;
    }
    // zero other entries in column
    for (int k=0; k<4; k++){
      if (k!=i){
        const float scale = buffer[k][i];
        for (j=0; j<8; j++){
          if (buffer[k][i]>0){
            //cerr << "i=" << i << " k= "<< k << "\n";
	  }
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
/// \todo define operator*= as well!
arMatrix4 operator*(const arMatrix4& A, const arMatrix4& B){
  arMatrix4 C;
  for (int i=0; i<4; i++)
    for (int j=0; j<4; j++){
      C.v[4*j+i] = A.v[i]*B.v[4*j] + A.v[i+4]*B.v[4*j+1] +
                   A.v[i+8]*B.v[4*j+2] + A.v[i+12]*B.v[4*j+3];
      }
  return C;
}

// matrix inverse again

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

  if ((v[0]>v[5])&&(v[0]>v[10]))  {	// Column 0:
    if (1.+v[0]-v[5]-v[10] <= 0){
      // Error.
      return arQuaternion(0,0,0,1);
    }
    const float scale  = 0.5/sqrt( 1.+v[0]-v[5]-v[10] ) ;
    return arQuaternion( (v[6]-v[9])*scale, 
                          0.25/scale, 
                          (v[1]+v[4])*scale, 
                          (v[8]+v[2])*scale );
  }

  if ( v[5] > v[10] ) {			// Column 1:
    if (1.+v[5]-v[0]-v[10] <= 0){
      // Error
      return arQuaternion(0,0,0,1);
    }
    const float scale = 0.5/sqrt( 1.+v[5]-v[0]-v[10] );
    return arQuaternion( (v[8]-v[2] )*scale, 
                         (v[1]+v[4] )*scale, 
                         0.25/scale, 
                         (v[6]+v[9] )*scale );
  } 				      
  // Column 2
  if (1.+v[10]-v[0]-v[5] <= 0){
    // Error
    return arQuaternion(0,0,0,1);
  }
  const float scale = 0.5/sqrt( 1.+v[10]-v[0]-v[5] );
  return arQuaternion( (v[1]-v[4])*scale, 
                       (v[8]+v[2])*scale, 
                       (v[6]+v[9])*scale, 
                       0.25/scale );

}

ostream& operator<<(ostream& os, const arMatrix4& x){
  os <<x.v[0]<<" "<<x.v[4]<<" "<<x.v[8]<<" "<<x.v[12]<<"\n"
     <<x.v[1]<<" "<<x.v[5]<<" "<<x.v[9]<<" "<<x.v[13]<<"\n"
     <<x.v[2]<<" "<<x.v[6]<<" "<<x.v[10]<<" "<<x.v[14]<<"\n"
     <<x.v[3]<<" "<<x.v[7]<<" "<<x.v[11]<<" "<<x.v[15]<<"\n";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arMatrix4& x){
  os <<x.v[0]<<" "<<x.v[4]<<" "<<x.v[8]<<" "<<x.v[12]<<"\n"
  <<x.v[1]<<" "<<x.v[5]<<" "<<x.v[9]<<" "<<x.v[13]<<"\n"
  <<x.v[2]<<" "<<x.v[6]<<" "<<x.v[10]<<" "<<x.v[14]<<"\n"
  <<x.v[3]<<" "<<x.v[7]<<" "<<x.v[11]<<" "<<x.v[15]<<"\n";
  return os;
}

//*************************
// quaternion not-inlined
//*************************

arQuaternion::arQuaternion() :
  real(1),
  pure(arVector3(0,0,0))
{
}

arQuaternion::arQuaternion(float x, float a, float b, float c) :
  real(x),
  pure(arVector3(a,b,c))
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
    ab32+b1b2,        aa+b2b2-b1b1-b3b3, -ab12+b2b3, 0,
    -ab22+b1b3,         ab12+b2b3,        aa+b3b3-b1b1-b2b2, 0,
    0, 0, 0, 1);
}

ostream& operator<<(ostream& os, const arQuaternion& x){
  os<<"("<<x.real<<" "<<x.pure.v[0]<<" "<<x.pure.v[1]<<" "<<x.pure.v[2]<<" )";
  return os;
}

arLogStream& operator<<(arLogStream& os, const arQuaternion& x){
  os<<"("<<x.real<<" "<<x.pure.v[0]<<" "<<x.pure.v[1]<<" "<<x.pure.v[2]<<" )";
  return os;
}

//*************************
// general use
//*************************

arMatrix4 ar_identityMatrix(){
  return arMatrix4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
}

arMatrix4 ar_translationMatrix(float x, float y, float z){
  arMatrix4 result;
  result.v[12] = x; result.v[13] = y; result.v[14] = z;
  return result;
}

arMatrix4 ar_translationMatrix(const arVector3& v){
  arMatrix4 result;
  memcpy(&result.v[12], &v.v[0], 3 * sizeof(float));
  return result;
}

// angles are all in radians

arMatrix4 ar_rotationMatrix(char axis, float r){
  switch (axis){
  case 'x':
    return arMatrix4(
      1, 0, 0, 0,
      0, cos(r), -sin(r), 0,
      0, sin(r), cos(r), 0,
      0, 0, 0, 1);
  case 'y':
    return arMatrix4(
      cos(r), 0, sin(r), 0,
      0, 1, 0, 0,
      -sin(r), 0, cos(r), 0,
      0, 0, 0, 1);
  case 'z':
    return arMatrix4(
      cos(r), -sin(r), 0, 0,
      sin(r), cos(r), 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1);
  }
  cerr << "syzygy ar_rotationMatrix error: unknown axis " << axis << endl;
  return ar_identityMatrix();
}

arMatrix4 ar_rotationMatrix(const arVector3& a, float radians){
  const arVector3 axis = (++a) == 0 ? arVector3(1,0,0) : a;
  const arQuaternion
    quaternion(cos(radians/2.), (sin(radians/2.)/(axis.magnitude()))*axis);
  const arVector3 rotX(quaternion*arVector3(1,0,0));
  const arVector3 rotY(quaternion*arVector3(0,1,0));
  const arVector3 rotZ(quaternion*arVector3(0,0,1));
  return arMatrix4(
		   rotX.v[0], rotY.v[0], rotZ.v[0], 0,
		   rotX.v[1], rotY.v[1], rotZ.v[1], 0,
		   rotX.v[2], rotY.v[2], rotZ.v[2], 0,
		   0, 0, 0, 1);
}

arMatrix4 ar_transrotMatrix(const arVector3& position, const arQuaternion& orientation) {
  return ar_translationMatrix( position )*arMatrix4( orientation );
}

arMatrix4 ar_scaleMatrix(float s){
  return ar_scaleMatrix(s, s, s);
}

arMatrix4 ar_scaleMatrix(float x, float y, float z){
  arMatrix4 result;
  result[0] = x;
  result[5] = y;
  result[10] = z;
  return result;
}

arMatrix4 ar_scaleMatrix(const arVector3& scaleFactors){
  arMatrix4 result;
  result[0] = scaleFactors.v[0];
  result[5] = scaleFactors.v[1];
  result[10] = scaleFactors.v[2];
  return result;
}

arMatrix4 ar_extractTranslationMatrix(const arMatrix4& original){
  arMatrix4 result;
  memcpy(&result.v[12], &original.v[12], 3 * sizeof(float));
  return result;
}

arVector3 ar_extractTranslation(const arMatrix4& original){
  return arVector3(original.v[12], original.v[13], original.v[14]);
}

arMatrix4 ar_extractRotationMatrix(const arMatrix4& original){
  arMatrix4 result;
  for (int i=0; i<3; i++){
    const arVector3 column(&original.v[4*i]);
    const float magnitude = ++column;
    if (magnitude > 0.){
      result.v[4*i  ] = original.v[4*i  ] / magnitude;
      result.v[4*i+1] = original.v[4*i+1] / magnitude;
      result.v[4*i+2] = original.v[4*i+2] / magnitude;
    }
    else{
      result.v[5*i] = 0.;
    }
  }
  return result;
}

arMatrix4 ar_extractScaleMatrix(const arMatrix4& original){
  arMatrix4 result;
  for (int i=0; i<3; i++){
    const arVector3 column(&original.v[4*i]);
    result.v[5*i] = ++column;
  }
  return result;
}

/// Returns the nonnegative angle, in radians, from first to second
/// (counterclockwise) around the vector first*second (cross product).
float ar_angleBetween(const arVector3& first, const arVector3& second){
  if (first.magnitude() <=0. || second.magnitude() <= 0.)
    return 0.;

  // In some cases the result of the dot
  // product (presumably because of rounding error) was going
  // outside [-1,1] (at least on Windows, I didn't check
  // Linux), and acos() was of course returning nan.
  double dotProd = (first/first.magnitude()).dot(second/second.magnitude());
  if (dotProd > 1.) {
    dotProd = 1.;
  } else if (dotProd < -1.) {
    dotProd = -1.;
  }
  return (float)acos(dotProd);
}

/// Returns euler angles calculated from fixed rotation axes. By default,
/// the axis order is ZYX (as denoted AR_ZYX.. but other possibilities are
/// AR_XYZ, AR_XZY, etc.) which means axis1 = (0,0,1), axis2 = (0,1,0),
/// and axis3 = (1,0,0). We assume that the given matrix is, in fact, a
/// pure rotation (and attempt to extract the rotation in order to ensure
/// this). Under this assumption, the original matrix will be:
/// ar_rotationMatrix(axis1, v[2])
/// * ar_rotationMatrix(axis2, v[1])
/// * ar_rotationMatrix(axis3, v[0])
arVector3 ar_extractEulerAngles(const arMatrix4& m, arAxisOrder o){
  arVector3 axis1, axis2, axis3;
  switch(o){
  case AR_XYZ:
    axis1 = arVector3(1,0,0);
    axis2 = arVector3(0,1,0);
    axis3 = arVector3(0,0,1);
    break;
  case AR_XZY:
    axis1 = arVector3(1,0,0);
    axis2 = arVector3(0,0,1);
    axis3 = arVector3(0,1,0);
    break;
  case AR_YXZ:
    axis1 = arVector3(0,1,0);
    axis2 = arVector3(1,0,0);
    axis3 = arVector3(0,0,1);
    break;
  case AR_YZX:
    axis1 = arVector3(0,1,0);
    axis2 = arVector3(0,0,1);
    axis3 = arVector3(1,0,0);
    break;
  case AR_ZXY:
    axis1 = arVector3(0,0,1);
    axis2 = arVector3(1,0,0);
    axis3 = arVector3(0,1,0);
    break;
  case AR_ZYX:
    axis1 = arVector3(0,0,1);
    axis2 = arVector3(0,1,0);
    axis3 = arVector3(1,0,0);
    break;
  }
  const arMatrix4 theMatrix(!ar_extractRotationMatrix(m));
  // Calculate the axis3 euler angle.
  arVector3 v(theMatrix * axis1);
  float magnitude = v.magnitude();
  if (magnitude <= 0.) {
    cerr << "ar_extractEulerAngles warning: bogus matrix.\n";
    return arVector3(0,0,0);
  }
  v /= magnitude;

  // Project to the axis1-axis2 plane.
  const arVector3 vyz = (v%axis1)*axis1 + (v%axis2)*axis2;
  // Euler angle is determined by rotating vyz to the axis1.
  float rot3 = ar_angleBetween(vyz,axis1);
  if (axis3.dot(vyz*axis1) < 0)
    rot3 = -rot3;

  // Calculate the axis2 euler angle.
  // Project the rotated vector to the axis1-axis3 plane, determine 
  // the angle with axis1 and this gives the axis2 euler angle
  v = ar_rotationMatrix(axis3,rot3)*v;
  const arVector3 vxz = (v%axis1)*axis1 + (v%axis3)*axis3;
  float rot2 = ar_angleBetween(vxz,axis1);
  if (axis2.dot(vxz*axis1) < 0)
    rot2 = -rot2;

  // Calculcate the axis1 euler angle.
  // Find the vector v2 mapped to axis3. The vector
  // ar_rotationMatrix(axis2, rot2)*ar_rotationMatrix3,rot3)*v2 
  // will be in the axis2-axis3 plane. The angle with
  // axis3 determines the axis1 euler angle.
  arVector3 v2(theMatrix * axis3);
  magnitude = ++v2;
  if (magnitude <= 0.) {
    cerr << "ar_extractEulerAngles warning: bogus matrix.\n";
    return arVector3(0,0,0);
  }
  v2 /= magnitude;
  v2 = ar_rotationMatrix(axis2,rot2)*ar_rotationMatrix(axis3,rot3)*v2;
  float rot1 = ar_angleBetween(v2,axis3);
  if (axis1.dot(v2*axis3) < 0)
    rot1 = -rot1;
 
  return arVector3(rot3,rot2,rot1);
}

arQuaternion ar_angleVectorToQuaternion(const arVector3& a, float radians) {
  const arVector3 axis = (++a) == 0 ? arVector3(1,0,0) : a;
  return arQuaternion(cos(radians/2), (sin(radians/2)/(++axis))*axis);
}

/// Returns the reflection of direction across the given normal vector
arVector3 ar_reflect(const arVector3& direction, const arVector3& normal){
  float mag = ++normal;
  return direction - (2 * direction % normal / (mag*mag)) * normal;
  //return(sub(incoming,
  //           scale(2*dot(incoming, normDIR)/(mag*mag), normDIR)));
  
}

// QUESTION: it looks like rayDirection is assumed to have been normalized.
// If so, we should normalize it before using it.
float ar_intersectRayTriangle(const arVector3& rayOrigin,
			      const arVector3& rayDirection,
			      const arVector3& vertex1,
			      const arVector3& vertex2,
			      const arVector3& vertex3){

  // this algorithm is from geometrysurfer.com by Dan Sunday
  const arVector3 rayDir( rayDirection.normalize() );
  const arVector3 u = vertex2 - vertex1;
  const arVector3 v = vertex3 - vertex1;
  const arVector3 n = u*v; // cross product
  if (++n == 0){
    // degenerate triangle
    return -1;
  }
  const arVector3 w0 = rayOrigin - vertex1;
  const float a = n % w0;
  const float b = n % rayDir;
  if (fabs(b) < 0.000001){
    // ray is hitting the triangle edge on...  call it "no intersection".
    return -1;
  }
  const float r = -a / b;
  if (r < 0){
    // ray goes away from triangle... no intersection
    return -1;
  }
  // intersection point of ray and triangle's plane
  const arVector3 intersect = rayOrigin + r*rayDir;
  // see if the intersection is inside the triangle
  const float uu = u % u;
  const float uv = u % v;
  const float vv = v % v;
  const arVector3 w = intersect - vertex1;
  const float wu = w % u;
  const float wv = w % v;
  const float D = uv * uv - uu * vv;
  if (fabs(D) < 0.000001){
    // error
    return -1;
  }
  const float s = (uv * wv - vv * wu) / D;
  if (s < 0. || s > 1.0){
    return -1;
  }
  const float t = (uv * wu - uu * wv) / D;
  if (t < 0. || (s+t) > 1.0){
    return -1;
  }
//  return r;
  return magnitude(r*rayDir);
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

// finds the point on a line that is nearest to some point
arVector3 ar_projectPointToLine( const arVector3& linePoint,
                                 const arVector3& lineDirection,
                                 const arVector3& otherPoint,
                                 const float threshold ) {
  const arVector3 V = otherPoint - linePoint;
  const arVector3 tmp = V * lineDirection;
  // If point is too close to line,
  // pretend point is on line so normalize() doesn't become unstable.
  if (tmp.magnitude() < threshold)
    return otherPoint;
  const arVector3 N = tmp.normalize();
  const arVector3 M = (lineDirection * N).normalize();
  return otherPoint - (V % M)*M;
}

//arMatrix4 ar_mirrorMatrix( const arMatrix4& placementMatrix ) {
//  arMatrix4 reflect( ar_identityMatrix() );
//  reflect.v[10] = -1.;
//  return placementMatrix.inverse() * reflect * placementMatrix;
//}

arMatrix4 ar_mirrorMatrix( const arVector3& planePoint, const arVector3& planeNormal ) {
  arVector4 tmp1( planeNormal.v[0], planeNormal.v[1], planeNormal.v[2], 0. );
  float d = -planeNormal.dot( planePoint );
  arVector4 tmp2( 2.*planeNormal.v[0], 2.*planeNormal.v[1], 2.*planeNormal.v[2], 2.*d );
  return ar_identityMatrix() - tmp1.outerProduct( tmp2 );
}

arMatrix4 ar_castShadowMatrix( const arMatrix4& objectMatrix,
                               const arVector4& lightPosition,
                               const arVector3& planePoint,
                               const arVector3& planeNormal ) {
  const arMatrix4 theMatrix = objectMatrix.inverse();
  arVector3 lightPos( lightPosition.v[0], lightPosition.v[1], lightPosition.v[2] );
  lightPos = theMatrix * lightPos;
  arVector4 lightVec( lightPos, lightPosition.v[3] );
  arVector3 point = theMatrix * planePoint;
  arVector3 normal = ar_extractRotationMatrix(theMatrix) * planeNormal;
  arVector4 planeParams( normal, -normal.dot( point ) );
  arMatrix4 outer( lightVec.outerProduct( planeParams ) );
  float dot( lightVec.dot( planeParams ) );
  arMatrix4 dotMatrix( ar_scaleMatrix( dot ) );
  dotMatrix.v[15] = dot;
  arMatrix4 result( dotMatrix - outer );
  return result;
}


arMatrix4 ar_rotateVectorToVector( const arVector3& vec1, const arVector3& vec2 ) {
  const float mag1 = vec1.magnitude();
  const float mag2 = vec2.magnitude();
  if (mag1==0. || mag2==0.) {
    cerr << "ar_rotateVectorToVector error: 0-length input vector.\n";
    return ar_identityMatrix();
  }
  const arVector3 rotAxis = vec1 * vec2;
  const float mag = rotAxis.magnitude();
  if (mag<1.e-6) {
    if (vec1.dot(vec2) < 0.) {
      // cheat
      return ar_scaleMatrix(-1.);
    } else {
      return ar_identityMatrix();
    }
  }
  return ar_rotationMatrix(rotAxis/mag, acos((vec1/mag1) % (vec2/mag2)));
}

arMatrix4 ar_planeToRotation(float posX, float posY){
  // Special case.
  if (posX == 0. && posY == 0.)
    return ar_identityMatrix();

  // Determine the mapping to the Riemann sphere from this piece of the plane.
  const float t = -8./(posX*posX + posY*posY);
  arVector3 spherePos(-t*posX/2., -t*posY/2., t+1.);
  // a cheat
  spherePos /= ++spherePos;

  // Rotate (0,0,-1) to spherePos.
  return ar_rotateVectorToVector( arVector3(0,0,-1), spherePos );
}

// In the screen-related math functions, screen normal points
// from the observer to the screen (i.e. along the ray of vision)

// Calculate the offset vector from the overall screen center to
// the center of an individual tile.  Tiles are in a rectangular,
// planar grid covering the screen, tile (0,0) at lower left.
arVector3 ar_tileScreenOffset(const arVector3& screenNormal,
			      const arVector3& screenUp,
			      float width, float height,
			      float xTile, int nxTiles,
			      float yTile, int nyTiles) {
  if (nxTiles == 0 || nyTiles == 0){
    // Bogus arguments.
    return arVector3(0,0,0);
  }

  /// copypaste start
  float mag = ++screenNormal;
  if (mag <= 0.)
    return arVector3(0,0,0);
  const arVector3 zHat = screenNormal/mag;
  mag = ++screenUp;
  if (mag <= 0.)
    return arVector3(0,0,0);
  const arVector3 xHat = zHat * screenUp/mag;  // '*' = cross product
  const arVector3 yHat = xHat * zHat;
  /// copypaste end
  float tileWidth = width/nxTiles;
  float tileHeight = height/nyTiles;
  return (-0.5*width + 0.5*tileWidth + xTile*tileWidth)*xHat 
    + (-0.5*height + 0.5*tileHeight + yTile*tileHeight)*yHat;
}

arMatrix4 ar_frustumMatrix( const arVector3& screenCenter,
			    const arVector3& screenNormal,
                            const arVector3& screenUp,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& eyePosition ) {
  /// copypaste start
  float mag = screenNormal.magnitude();
  if (mag <= 0.)
    return ar_identityMatrix(); // error
  const arVector3 zHat = screenNormal/mag;
  const arVector3 xHat = zHat * screenUp/mag;  // '*' = cross product
  const arVector3 yHat = xHat * zHat;
  /// copypaste end

  const arVector3 rightEdge = screenCenter + halfWidth * xHat;
  const arVector3 leftEdge = screenCenter - halfWidth * xHat;
  const arVector3 topEdge = screenCenter + halfHeight * yHat;
  const arVector3 botEdge = screenCenter - halfHeight * yHat;

  // float zEye = (eyePosition - headPosition) % zHat; // '%' = dot product
  float screenDistance = ( screenCenter - eyePosition ) % zHat;
  if (screenDistance == 0.)
    return ar_identityMatrix(); // error

  const float nearFrust = nearClip;
  const float distScale = nearFrust / screenDistance;
  const float rightFrust = distScale*(( rightEdge - eyePosition ) % xHat);
  const float leftFrust = distScale*(( leftEdge - eyePosition ) % xHat);
  const float topFrust = distScale*(( topEdge - eyePosition ) % yHat);
  const float botFrust = distScale*(( botEdge - eyePosition ) % yHat);
  const float farFrust = screenDistance + farClip;

  if (rightFrust == leftFrust || topFrust == botFrust || nearFrust == farFrust)
    return ar_identityMatrix(); // error

   // this is necessary because g++ 2.96 is messed up.
  float funnyElement = (nearFrust+farFrust)/(nearFrust-farFrust);
  arMatrix4 result = arMatrix4((2*nearFrust)/(rightFrust-leftFrust), 0., (rightFrust+leftFrust)/(rightFrust-leftFrust), 0.,
		   0., (2*nearFrust)/(topFrust-botFrust), (topFrust+botFrust)/(topFrust-botFrust), 0.,
		   0., 0., funnyElement, 2*nearFrust*farFrust/(nearFrust-farFrust),
		   0., 0., -1., 0. );
  return result;
}

// in this version, eyePosition has been multiplied by
// ar_lookatMatrix( screenCenter, screenNormal, screenUp )
// screenDistance is the distance from the local coordinate system
// to the image plane
arMatrix4 ar_frustumMatrix( const float screenDist,
                            const float halfWidth, const float halfHeight,
                            const float nearClip, const float farClip,
                            const arVector3& locEyePosition ) {
  float screenDistance = screenDist + locEyePosition.v[2];
  if (screenDistance == 0.)
    return ar_identityMatrix(); // error

  const float nearFrust = nearClip;
  const float distScale = nearFrust / screenDistance;
  const float rightFrust = distScale*( halfWidth - locEyePosition.v[0] );
  const float leftFrust = distScale*( -halfWidth - locEyePosition.v[0] );
  const float topFrust = distScale*( halfHeight - locEyePosition.v[1] );
  const float botFrust = distScale*( -halfHeight - locEyePosition.v[1] );
  const float farFrust = screenDistance + farClip;

  if (rightFrust == leftFrust || topFrust == botFrust || nearFrust == farFrust)
    return ar_identityMatrix(); // error

   // this is necessary because g++ 2.96 is messed up.
  float funnyElement = (nearFrust+farFrust)/(nearFrust-farFrust);
  arMatrix4 result = arMatrix4((2*nearFrust)/(rightFrust-leftFrust), 0., (rightFrust+leftFrust)/(rightFrust-leftFrust), 0.,
		   0., (2*nearFrust)/(topFrust-botFrust), (topFrust+botFrust)/(topFrust-botFrust), 0.,
		   0., 0., funnyElement, 2*nearFrust*farFrust/(nearFrust-farFrust),
		   0., 0., -1., 0. );
  return result;
}

/// ar_lookatMatrix is equivalent to gluLookAt(...).
/// It transforms the frame (a,b,c):
///   c = unit vector pointing from lookatPosition to viewPosition
///   b = unit vector along the portion of up orthogonal to c
///   a = b cross c
/// to the frame (x,y,z).
/// NOTE: In OpenGL, the eye is looking in the negative z direction.
arMatrix4 ar_lookatMatrix( const arVector3& viewPosition,
                           const arVector3& lookatPosition,
                           const arVector3& up ) {
  // The unit vector from lookatPosition to viewPosition.
  // This will be positive z (after the transform).
  const arVector3 Lhat( (viewPosition - lookatPosition).normalize() );
  // Uhat will be positive y (after the transform).
  const arVector3 Uhat( (up - (up % Lhat)*Lhat).normalize() );
  // Xhat will be positive x (after the transform).
  const arVector3 Xhat( Uhat * Lhat );
  const arMatrix4 look( Xhat.v[0], Xhat.v[1], Xhat.v[2], 0.,
                        Uhat.v[0], Uhat.v[1], Uhat.v[2], 0.,
                        Lhat.v[0], Lhat.v[1], Lhat.v[2], 0.,
                        0., 0., 0., 1. );
  return look * ar_translationMatrix( -viewPosition );
}

// uniform (0.0,1.0) pseudorandom number generater ran1 from Numerical Recipes in C
// ugly, but we're not ready to trust rand() yet (not to mention RAND_MAX is pathetically
// small under MS VC++). Not recommended for sequences longer than about 10^8
// initialize by setting *idum to a negative value.
// Note never returns 0 or 1

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
  // NOTE: ar_randUniformFloat() never returns 0 or 1
  float r = ar_randUniformFloat(idum);
  return int(floor( r*(hi-lo+1) ) + lo);
}

