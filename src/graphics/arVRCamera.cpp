//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGraphicsScreen.h"
#include "arHead.h"
#include "arVRCamera.h"

/// Allows us to get the projection matrix as computed by this screen object,
/// without putting it directly on the OpenGL matrix stack. This is useful
/// for authoring applications that use a different graphics engine than
/// OpenGL
arMatrix4 arVRCamera::getProjectionMatrix() {
  arGraphicsScreen* screen = getScreen();
  if ((!_head)||(!screen)) {
    if (_complained) {
      cerr << "arVRCamera error: getProjectionMatrix called with NULL head ("
           << _head << ") or screen (" << screen << ") pointer.\n"
           << "  (Suppressing further error messages).\n";
      _complained = true;
      return ar_identityMatrix();
    }
  }
  bool demoMode 
    = (_head->getFixedHeadMode() && !screen->getIgnoreFixedHeadMode())
       || screen->getAlwaysFixedHeadMode();
  arMatrix4 headMatrix = _head->getMatrix();
  if (demoMode) {
    headMatrix = _getFixedHeadModeMatrix( *screen );
  }
  arVector3 eyePosition = _head->getEyePosition( getEyeSign(), &headMatrix );
  const arVector3 midEyePosition = _head->getMidEyePosition( &headMatrix );
  const float zEye = (eyePosition - midEyePosition).dot( screen->getNormal() );
  // If this eye is the closer to the screen, use nearClip; if not, increase
  // _nearClip so it corresponds to the same physical plane.
  // This is the other quantity that needs to be computed. We are making sure
  // that 2 stereo eyes use the same clipping plane.
  float nearClip = zEye >= 0. ? _head->getNearClip() : _head->getNearClip()-2*zEye;
  float farClip = zEye >= 0. ? _head->getFarClip() : _head->getFarClip()-2*zEye;
  float unitConversion = _head->getUnitConversion();
  arVector3 center = screen->getCenter();
  arVector3 normal = screen->getNormal();
  arVector3 up = screen->getUp();
  if (screen->getHeadMounted()) {
    center = unitConversion*(headMatrix*(center+_head->getMidEyeOffset()));
    arMatrix4 orient(ar_extractRotationMatrix(headMatrix)); 
    normal = orient*normal; 
    up = orient*up; 
  } else {
    center *= unitConversion;
  }
//  cerr << center << normal << up << endl <<  unitConversion*0.5*screen->getWidth()
//       << ", " << unitConversion*0.5*screen->getHeight() << ", " << nearClip << ", "
//       << farClip << ", " << eyePosition << endl;
  return ar_frustumMatrix( center, normal, up,
                           unitConversion*0.5*screen->getWidth(),
                           unitConversion*0.5*screen->getHeight(),
                           nearClip, farClip,
                           eyePosition );
}

/// Allows us to get the modelview matrix as computed by this screen object,
/// without putting it directly on the OpenGL matrix stack. This is useful
/// for authoring applications that use a different engine than OpenGL.
arMatrix4 arVRCamera::getModelviewMatrix(){
  arGraphicsScreen* screen = getScreen();
  if ((!_head)||(!screen)) {
    if (_complained) {
      cerr << "arVRCamera error: getModelviewMatrix called with NULL head ("
           << _head << ") or screen (" << screen << ") pointer.\n"
           << "  (Suppressing further error messages).\n";
      _complained = true;
      return ar_identityMatrix();
    }
  }
  bool demoMode = (_head->getFixedHeadMode() && !screen->getIgnoreFixedHeadMode())
                      || screen->getAlwaysFixedHeadMode();
  arMatrix4 headMatrix = _head->getMatrix();
  if (demoMode) {
    headMatrix = _getFixedHeadModeMatrix( *screen );
  }
  arVector3 eyePosition = _head->getEyePosition( getEyeSign(), &headMatrix );
//  cerr << "Head:\n" << headMatrix << "Eye: " << eyePosition << endl;
  arVector3 normal = screen->getNormal();
  arVector3 up = screen->getUp();
  if (screen->getHeadMounted()) {
    arMatrix4 orient(ar_extractRotationMatrix(headMatrix)); 
    normal = orient*normal; 
    up = orient*up; 
  }
  return ar_lookatMatrix( eyePosition, eyePosition+normal, up);
}

/// Puts the viewing matrices on the OpenGL stack.
void arVRCamera::loadViewMatrices(){
//  GLint drawBuffers;
//  glGetIntegerv( GL_DRAW_BUFFER, &drawBuffers );
//  cerr << "Eye sign\n" << getEyeSign() << ", drawBuffers = " << drawBuffers << endl;
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  arMatrix4 temp = getProjectionMatrix();
//  cerr << "Projection\n" << temp << endl;
  glMultMatrixf( temp.v );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  temp = getModelviewMatrix();
//  cerr << "ModelView\n" << temp << endl << "--------------------------------------------------\n\n";
  glMultMatrixf( temp.v );
}

arMatrix4 arVRCamera::_getFixedHeadModeMatrix( const arGraphicsScreen& screen ) {
  const arVector3 zHat = screen.getNormal().normalize();
  const arVector3 xHat = zHat * screen.getUp().normalize();  // '*' = cross product
  const arVector3 yHat = xHat * zHat;
  const arVector3 yPrime = cos(screen.getFixedHeadHeadUpAngle())*yHat - sin(screen.getFixedHeadHeadUpAngle())*xHat;
  const arVector3 xPrime = zHat * yPrime;

  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4) {
    demoRotMatrix[i] = xPrime.v[j];
    demoRotMatrix[i+1] = yPrime.v[j];
    demoRotMatrix[i+2] = zHat.v[j++];
  }
  return ar_translationMatrix(screen.getFixedHeadHeadPosition()) * demoRotMatrix;
}



