//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arMath.h"
#include "arDataUtilities.h"

const float epsilon = 0.0004;

bool identityTest(arMatrix4 m){
  bool result = true;
  for (int i=0; i<16; i++){
    if (i%4 == i/4){
      if (fabs(m[i]-1) > epsilon){
	result = false;
      }
    }
    else{
      if (fabs(m[i]-0) > epsilon){
	result = false;
      }
    }
  }
  return result;
}

bool zeroTest(arMatrix4 m){
  bool result = true;
  for (int i=0; i<16; i++){
    if (fabs(m[i]) > epsilon){
      result = false;
    }
  }
  return result;
}

bool equalVectorTest(arVector3 v1, arVector3 v2) {
  return (v1-v2).magnitude() <= epsilon;
}

int main(){
  int i, j;

  cout << "Math test: if no FAILED messages appear, then all tests succeeded.\n";

  cout << "Testing vector algebra.\n";
  if (++( arVector3(1,0,0)*arVector3(0,1,0) - arVector3(0,0,1)) > epsilon){
    cout << "FAILED: vector cross product (1).\n";
  }
  if (++( arVector3(0,1,0)*arVector3(0,0,1) - arVector3(1,0,0)) > epsilon){
    cout << "FAILED: vector cross product (2).\n";
  }
  if (++( arVector3(0,0,1)*arVector3(1,0,0) - arVector3(0,1,0)) > epsilon){
    cout << "FAILED: vector cross product (3).\n";
  }

  if (++( 4*arVector3(1,1,1)*3 - arVector3(12,12,12)) > epsilon){
    cout << "FAILED: multiplication of scalar by vector.\n";
  }

  if (fabs(++arVector3(1,-1,1) - 1.7320508) > epsilon){
    cout << "FAILED: vector magnitude test.\n";
  }

  if (fabs(arVector3(1,-1,1).normalize().magnitude() - 1.) > epsilon){
    cout << "FAILED: vector magnitude test 2.\n";
  }

  if (++(arVector3(3,4,5)+(-arVector3(-1,-1,-1))-arVector3(4,5,6)) > epsilon){
    cout << "FAILED: vector addition test.\n";
  }

  if (fabs(arVector3(-1,2,3)%arVector3(1,1,1) - 4) > epsilon){
    cout << "FAILED: vector cross product.\n";
  }

  cout << "Testing matrix algebra.\n";

  if (!zeroTest(arMatrix4() - ar_identityMatrix())) {
    cout << "FAILED: matrix default constructor.\n";
  }

  arMatrix4 matrix1 = arMatrix4(5,5,5,5,
				5,5,5,5,
				5,5,5,5,
				5,5,5,5);
  arMatrix4 matrix2            (1,1,1,1,
				0,0,0,0,
				1,1,1,1,
				0,0,0,0);
  arMatrix4 testMatrix = arMatrix4() + matrix1 - matrix2;
  if (!zeroTest(testMatrix - arMatrix4(5,4,4,4,
				       5,6,5,5,
				       4,4,5,4,
				       5,5,5,6))){
    cout << "FAILED: matrix addition test.\n";
  }
  
  // ar_identityMatrix() function plus matrix multiplication (i.e. 
  // multiply something by the identity matrix and you get that matrix back)
  if (!zeroTest(ar_translationMatrix(1,2,3) - 
		arMatrix4() * ar_translationMatrix(1,2,3) *
		arMatrix4())){
    cout << "FAILED: identity matrix test.\n";
  }

  // matrix inverse
  if (!identityTest(ar_translationMatrix(1,2,3)*!ar_translationMatrix(1,2,3))){
    cout << "FAILED: matrix inverse (1).\n";
  }

  if (!identityTest(ar_rotationMatrix('x',ar_convertToRad(90))
                    *!ar_rotationMatrix('x',ar_convertToRad(90)))){
    cout << "FAILED: matrix inverse (2).\n";
  }

  if (!identityTest(ar_scaleMatrix(1,2,3)*!ar_scaleMatrix(1,2,3))){
    cout << "FAILED: matrix inverse (3).\n";
  }

  // transpose. the transpose of a 3D rotation matrix is
  // the inverse of the original matrix.
  if (!identityTest(ar_rotationMatrix('x',ar_convertToRad(-45))
	            *~ar_rotationMatrix('x',ar_convertToRad(-45)))){
    cout << "FAILED: matrix transpose.\n";
  }

  cout << "Testing rotation matrices.\n";
  arVector3 test1 = ar_rotationMatrix('x', ar_convertToRad(90))
    *arVector3(0,1,0);
  if (++(test1-arVector3(0,0,1)) > epsilon){
    cout << "FAILED: rotation matrix about x-axis.\n";
  }
  test1 = ar_rotationMatrix('y', ar_convertToRad(90))
    *arVector3(1,0,0);
  if (++(test1-arVector3(0,0,-1)) > epsilon){
    cout << "FAILED: rotation matrix about y-axis.\n";
  }
  test1 = ar_rotationMatrix('z', ar_convertToRad(90))
    *arVector3(1,0,0);
  if (++(test1-arVector3(0,1,0)) > epsilon){
    cout << "FAILED: rotation matrix about z-axis.\n";
  }

  // let's also test the quaternion-based matrix rotation code
  test1 = ar_rotationMatrix(arVector3(1,0,0),ar_convertToRad(90))
    *arVector3(0,1,0);
  if (++(test1-arVector3(0,0,1)) > epsilon){
    cout << "FAILED: rotation matrix about x-axis (quaternion).\n";
  }
  test1 = ar_rotationMatrix(arVector3(0,1,0), ar_convertToRad(90))
    *arVector3(1,0,0);
  if (++(test1-arVector3(0,0,-1)) > epsilon){
    cout << "FAILED: rotation matrix about y-axis (quaternion).\n";
  }
  test1 = ar_rotationMatrix(arVector3(0,0,1), ar_convertToRad(90))
    *arVector3(1,0,0);
  if (++(test1-arVector3(0,1,0)) > epsilon){
    cout << "FAILED: rotation matrix about z-axis (quaternion).\n";
  }
  test1 = ar_rotationMatrix(arVector3(0.5,0.5,0), ar_convertToRad(90))
    *arVector3(.707107,-.707107,0);
  if (++(test1-arVector3(0,0,-1)) > epsilon){
    cout << "FAILED: rotation matrix about axis (quaternion).\n";
    cout << "    result = " << test1 << endl;
  }

  // test translation matrix code
  cout << "Testing translation matrices.\n";
  test1 = ar_translationMatrix(1,2,3)*arVector3(0,0,0);
  if (++(test1-arVector3(1,2,3)) > epsilon){
    cout << "FAILED: translation matrix creation.\n";
  }

  // scale matrix test code
  cout << "Testing scale matrices.\n";
  test1 = ar_scaleMatrix(2,3,4)*arVector3(1,1,1);
  if (++(test1-arVector3(2,3,4)) > epsilon){
    cout << "FAILED: scale matrix creation.\n";
  }

  // do the ar_extractRotationMatrix(), etc function calls actually work?
  cout << "Testing extraction of matrix components.\n";
  arMatrix4 testTrans = ar_translationMatrix(1,2,3);
  arMatrix4 testRot = ar_rotationMatrix('x',ar_convertToRad(180));
  arMatrix4 testScale = ar_scaleMatrix(2,3,4);
  testMatrix = testTrans*testRot*testScale;
  if (!zeroTest(testTrans - ar_extractTranslationMatrix(testMatrix))){
    cout << "FAILED: translation matrix extraction.\n";
  }
  if (!zeroTest(testRot - ar_extractRotationMatrix(testMatrix))){
    cout << "FAILED: rotation matrix extraction.\n";
  }
  if (!zeroTest(testScale - ar_extractScaleMatrix(testMatrix))){
    cout << "FAILED: extract scale matrix.\n";
  }

  // test calculating the angle between two vectors
  cout << "Testing angle between vectors.\n";
  if (fabs(ar_angleBetween(arVector3(0,0,1),arVector3(0,1,0))
	    -ar_convertToRad(90)) > epsilon){
    cout << "FAILED: angle between vectors (1).\n";
  }
  if (fabs(ar_angleBetween(arVector3(0,0,1),arVector3(1,0,0))
	    -ar_convertToRad(90)) > epsilon){
    cout << "FAILED: angle between vectors (2).\n";
  }
  if (fabs(ar_angleBetween(arVector3(1,0,0),arVector3(1,1,0))
	    -ar_convertToRad(45)) > epsilon){
    cout << "FAILED: angle between vectors (3).\n";
  }
  if (fabs(ar_angleBetween(arVector3(1,1,1),arVector3(-1,-1,-1))
	    -ar_convertToRad(180)) > epsilon){
    cout << "FAILED: angle between vectors (4).\n";
    cout << ar_angleBetween(arVector3(1,1,1),arVector3(-1,-1,-1)) 
         << " <> " << ar_convertToRad(180) << endl;
  }
  if (fabs(ar_angleBetween(arVector3(0,0,1),arVector3(-1,0,-1))
	    -ar_convertToRad(135)) > epsilon){
    cout << "FAILED: angle between vectors (5).\n";
  }

  cout << "Testing screen tiling.\n";
  if (fabs(++(ar_tileScreenOffset(arVector3(0,0,-1),
				  arVector3(0,1,0),1,1,0,2,0,2) -
	      arVector3(-0.25,-0.25,0))) > epsilon) {
    cout << "FAILED: screen tile test (1).\n";
  }
  if (fabs(++(ar_tileScreenOffset(arVector3(1,0,0),
				  arVector3(0,1,0),2,2,0,4,0,2) -
	      arVector3(0,-0.5,-0.75))) > epsilon) {
    cout << "FAILED: screen tile test (2).\n";
  }
  if (fabs(++(ar_tileScreenOffset(arVector3(1,0,0),
				  arVector3(0,1,0),2,2,3,4,1,2) -
	      arVector3(0,0.5,0.75))) > epsilon) {
    cout << "FAILED: screen tile test (3).\n";
  }
  if (fabs(++(ar_tileScreenOffset(arVector3(1,0,0),
				  arVector3(0,1,0),1,1,0,1,0,1) -
	      arVector3(0,0,0))) > epsilon) {
    cout << "FAILED: screen tile test (4).\n";
  }

  cout << "Testing frustum calculations.\n";
  arVector3 resultVec;
  arMatrix4 frustum = ar_frustumMatrix( arVector3( 0, 0, -1 ),
			    arVector3( 0, 0, -1 ), arVector3( 0, 1, 0 ),
			    0.5, 0.5, 1, 10, arVector3( 0, 0, 0 ) );
  resultVec = frustum*arVector3( 0.5, 0.5, -1 );
  if (!equalVectorTest( resultVec, arVector3( 1, 1, -1 ) ) ) {
    cout << "FAILED: ar_frustumMatrix test (1).\n"
         << resultVec << " <> " << arVector3( 1, 1, -1 ) << endl
         << "matrix:\n" << frustum << endl;
  }

  frustum = ar_frustumMatrix( arVector3( 0, 0, -1 ),
			    arVector3( 0, 0, -1 ), arVector3( 0, 1, 0 ),
			    0.5, 0.5, 1, 9, arVector3( 0, 0, 0 ) );
  resultVec = frustum*arVector3( -5, -5, -10 );
  if (!equalVectorTest( resultVec, arVector3( -1, -1, 1 )  ) ) {
    cout << "FAILED: ar_frustumMatrix test (2).\n"
         << resultVec << " <> " << arVector3( -1, -1, 1 ) << endl
         << "matrix:\n" << frustum << endl;
  }

  frustum = ar_frustumMatrix( arVector3( 0, 0, -1 ),
			    arVector3( 0, 0, -1 ), arVector3( 0, 1, 0 ),
			    0.5, 0.5, 1, 9, arVector3( 0.5, 0, 0 ) ); 
  resultVec = frustum*arVector3( -10, 0, -10 );
  if (!equalVectorTest( resultVec, arVector3( -1, 0, 1 ) ) ) {
    cout << "FAILED: ar_frustumMatrix test (3).\n"
         << resultVec << " <> " << arVector3( -1, 0, 1 ) << endl
         << "matrix:\n" << frustum << endl;
  }

  cout << "Testing quaternions.\n";
  arVector3 rotAxis( .75, -.2, .17 );
  rotAxis/=++rotAxis;    // arbitrary unit vector
  float rotAngle = 0.8213;
  if (!equalVectorTest( ar_rotationMatrix( rotAxis, rotAngle )
                        *arVector3( 1, 4, -2 ),
                        ar_angleVectorToQuaternion( rotAxis, rotAngle )
                        *arVector3( 1, 4, -2 ) )) {
    cout << "FAILED: arQuaternion test (1).\n";
  }
  if (!equalVectorTest( ar_rotationMatrix( rotAxis, rotAngle )
                        *arVector3( 3, 0, 2 ),
                        arMatrix4(ar_angleVectorToQuaternion(rotAxis,rotAngle))
                        *arVector3( 3, 0, 2 ) )) {
    cout << "FAILED: arQuaternion test (2).\n";
  }
  if (!equalVectorTest( arQuaternion( ar_rotationMatrix( rotAxis, rotAngle ) )
                        *arVector3( 3, 0, 2 ),
                        ar_angleVectorToQuaternion( rotAxis, rotAngle )
                        *arVector3( 3, 0, 2 ) )) {
    cout << "FAILED: arQuaternion test (3).\n";
  }

  cout << "Testing geometry utility functions.\n";
  arVector3 rayOrigin(1,1,1);
  arVector3 rayDirection(-1,-1,-1);
  arVector3 v1(1,-0.1,0);
  arVector3 v2(-1,-0.1,0);
  arVector3 v3(-1,1,0);
  if (fabs(ar_intersectRayTriangle(rayOrigin, rayDirection, v1, v2, v3)-sqrt(3.0)) >
      epsilon){
    cout << "FAILED: intersect ray triangle test.\n";
  }
  if (!equalVectorTest( ar_projectPointToLine( arVector3(0,0,1), arVector3(1,0,0), 
                                                    arVector3(12,20,1)),
                        arVector3( 12,0,1 ) )) {
    cout << "FAILED: point-to-line projection test.\n";
  }

  cout << "Testing euler angle extraction.\n";
  arVector3 testAngles(.3,.5,.6);
  arEulerAngles eulerAngles( AR_XYZ, testAngles );
  arMatrix4 eulerTestMatrix = eulerAngles.toMatrix();
  arVector3 eulerResult = eulerAngles.extract( eulerTestMatrix );
  float errMag = (eulerResult-testAngles).magnitude();
  if (errMag > 1.e-5) {
    cout << "FAILED: euler angle conversion #1 (error = " << errMag << ").\n";
  }

  testAngles = arVector3(1.3,-.7,-.05);
  eulerAngles.setOrder( AR_ZYX );
  eulerAngles.setAngles( testAngles );
  eulerTestMatrix = eulerAngles.toMatrix();
  eulerResult = eulerAngles.extract( eulerTestMatrix );
  errMag = (eulerResult-testAngles).magnitude();
  if (errMag > 1.e-5) {
    cout << "FAILED: euler angle conversion #2 (error = " << errMag << ").\n";
  }

  testAngles = arVector3(-.3,1.5,.1);
  eulerAngles.setOrder( AR_YZX );
  eulerAngles.setAngles( testAngles );
  eulerTestMatrix = eulerAngles.toMatrix();
  eulerResult = eulerAngles.extract( eulerTestMatrix );
  errMag = (eulerResult-testAngles).magnitude();
  if (errMag > 1.e-5) {
    cout << "FAILED: euler angle conversion #3 (error = " << errMag << ").\n";
  }

  cout << "Testing quaternion/matrix conversions.\n";
  arQuaternion testQ = eulerTestMatrix;
  arMatrix4 newTest = testQ;
  for (i=0; i<16; i++){
    if (fabs(eulerTestMatrix[i]-newTest[i]) > epsilon){
      cout << "FAILED: quaternion/matrix conversion.\n";
      break;
    }
  }

  // Speed tests
  const int iMax = 10000;
  const int i2 = iMax * 20;
  arMatrix4 mm[iMax];
  arMatrix4 am[iMax];
  arMatrix4 bm[iMax];
  ar_timeval time1 = ar_time();
  for (i=0; i<i2; i++){
    j = i%iMax;
    mm[j] = am[j]*bm[j];
  }
  ar_timeval time2 = ar_time();
  cout << "Matrix multiply time (microseconds) = " 
       << ar_difftime(time2, time1)/i2 << "\n";

  arVector3 v;
  time1 = ar_time();
  for (i=0; i<i2; i++){
    j = i%iMax;
    v = ar_extractEulerAngles(mm[j],AR_XYZ);
  }
  time2 = ar_time();
  cout << "Extract euler angles time (microseconds) = " 
       << ar_difftime(time2, time1)/i2 << "\n";

  for (i=0; i<i2; i++){
    j = i%iMax;
    am[j] = ar_rotationMatrix('y',ar_convertToRad(10 + j%170)) *
            ar_translationMatrix(1, 1, 1 + j%170);
  }
  time1 = ar_time();
  for (i=0; i<i2; i++){
    j = i%iMax;
    mm[j] = !am[j];
  }
  time2 = ar_time();
  cout << "Matrix inverse time (microseconds) = " 
       << ar_difftime(time2, time1)/i2 << "\n";
}
