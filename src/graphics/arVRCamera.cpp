//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGraphicsScreen.h"
#include "arHead.h"
#include "arVRCamera.h"

// Get the projection matrix as computed by this screen object,
// without putting it directly on the OpenGL matrix stack. This is useful
// for applications that don't use OpenGL.
// Not const: sets _complained.
arMatrix4 arVRCamera::getProjectionMatrix() {
  const arGraphicsScreen* screen = getScreen();
  if (!_head || !screen) {
    if (!_complained) {
      cerr << "arVRCamera error: getProjectionMatrix called with NULL head ("
           << _head << ") or screen (" << screen << ").\n"
           << "  (Suppressing further error messages).\n";
      _complained = true;
    }
    return ar_identityMatrix();
  }

  const bool demoMode =
    (_head->getFixedHeadMode() && !screen->getIgnoreFixedHeadMode()) ||
    screen->getAlwaysFixedHeadMode();
  const arMatrix4 headMatrix(demoMode ? _getFixedHeadModeMatrix(*screen) : _head->getMatrix());
  const arVector3 eyePosition(_head->getEyePosition(getEyeSign(), &headMatrix));
  const arVector3 midEyePosition(_head->getMidEyePosition(&headMatrix));
  const float zEye = (eyePosition - midEyePosition).dot( screen->getNormal() );
  // If this eye is the closer to the screen, use nearClip; if not, increase
  // _nearClip so it corresponds to the same physical plane.
  // This is the other quantity that needs to be computed. We are making sure
  // that 2 stereo eyes use the same clipping plane.
  const float nearClip = zEye >= 0. ? _head->getNearClip() : _head->getNearClip()-2*zEye;
  const float farClip = zEye >= 0. ? _head->getFarClip() : _head->getFarClip()-2*zEye;
  const float unitConversion = _head->getUnitConversion();
  arVector3 center(screen->getCenter());
  arVector3 normal(screen->getNormal());
  arVector3 up(screen->getUp());
  if (screen->getHeadMounted()) {
    center = unitConversion*(headMatrix*(center+_head->getMidEyeOffset()));
    const arMatrix4 orient(ar_extractRotationMatrix(headMatrix));
    normal = orient * normal;
    up = orient * up;
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

// Allows us to get the modelview matrix as computed by this screen object,
// without putting it directly on the OpenGL matrix stack. This is useful
// for authoring applications that use a different engine than OpenGL.
// Not const: sets _complained.
arMatrix4 arVRCamera::getModelviewMatrix() {
  arGraphicsScreen* screen = getScreen();
  if ((!_head)||(!screen)) {
    if (!_complained) {
      cerr << "arVRCamera error: getModelviewMatrix called with NULL head ("
           << _head << ") or screen (" << screen << ") pointer.\n"
           << "  (Suppressing further error messages).\n";
      _complained = true;
    }
    return arMatrix4();
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

// Puts the viewing matrices on the OpenGL stack.
void arVRCamera::loadViewMatrices() {
//  GLint drawBuffers = -1;
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
  if (!_head) {
    if (!_complained) {
      cerr << "arVRCamera error: _getFixedHeadModeMatrix called with NULL head "
           << " pointer. (Suppressing further error messages).\n";
      _complained = true;
    }
    return arMatrix4();
  }

  // arGraphicsScreen::configure already normalized getNormal() and getUp().
  const arVector3 zHat(screen.getNormal());
  const arVector3 xHat(zHat * screen.getUp());  // '*' = cross product
  const arVector3 yHat(xHat * zHat);
  const arVector3 yPrime(cos(screen.getFixedHeadHeadUpAngle())*yHat - sin(screen.getFixedHeadHeadUpAngle())*xHat);
  const arVector3 xPrime(zHat * yPrime);

  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4, j++) {
    demoRotMatrix[i  ] = xPrime.v[j];
    demoRotMatrix[i+1] = yPrime.v[j];
    demoRotMatrix[i+2] = zHat.v[j];
  }
  // NOTE: previously, the screen's fixed_head_pos specified where the head
  // _sensor_ should go in demo mode. The correct behavior is to use it to 
  // specify the mid-eye position; the lines below do this.
//  cerr << screen.getFixedHeadHeadPosition() << endl;
  arMatrix4 headMatrix = ar_translationMatrix(screen.getFixedHeadHeadPosition()) * demoRotMatrix;
  arMatrix4 headSensorMatrix = headMatrix * ar_translationMatrix( -1.*_head->getMidEyeOffset() );
//cerr << headSensorMatrix << endl;
  return headSensorMatrix;
}
