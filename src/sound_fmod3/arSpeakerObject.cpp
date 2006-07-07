//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Most of this is copypasted from graphics/arScreenObject.cpp
// and should be folded back in!
#include "arPrecompiled.h"
#include "arSpeakerObject.h"
#include "fmodStub.h"

arSpeakerObject::arSpeakerObject() :
  _unitConversion(1.),
  _demoMode(false),
  _demoHeadUpAngle(0.),
  _normal( 0,0,-1 ),
  _up( 0,1,0 ),
  _midEyeOffset(-6./(2.54*12),0,0),
  _posPrev(0,0,0),
  _upPrev(0,0,0),
  _forwardPrev(0,0,0)
{}

bool arSpeakerObject::configure(arSZGClient*){
  // IMPORTANT NOTE: if any configuration ever occurs here, be sure to
  // push the resulting messages on szgClient's initResponse stream...
  // see arScreenObject::configure(...)
  return true;
}

// This belongs in arMath.h, not here.
static inline void Normalize(float* v) {
  float invlen = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  if (invlen < 1e-6)
    return;
  invlen = 1./invlen;
  v[0] *= invlen;
  v[1] *= invlen;
  v[2] *= invlen;
}
static inline void Normalize(arVector3& a) {
  Normalize(a.v);
}

void arSpeakerObject::loadMatrices(const arMatrix4& headMatrix) {
  arMatrix4 locHeadMatrix = _demoMode ? demoHeadMatrix(headMatrix) : headMatrix;
#ifdef UNUSED
  // these might be more correct than using raw locHeadMatrix
  arVector3 eyeOffsetVector = _midEyeOffset;
  arVector3 eyePosition = _unitConversion * (locHeadMatrix * eyeOffsetVector);
  arVector3 headPosition = _unitConversion * ar_extractTranslation( locHeadMatrix );
  arVector3 midEyePosition = _unitConversion * (locHeadMatrix * _midEyeOffset);
#endif

  // Update listener's attributes.
  const arMatrix4 rot(ar_extractRotationMatrix(locHeadMatrix));
  arVector3 pos(locHeadMatrix[12],locHeadMatrix[13],locHeadMatrix[14]);
  arVector3 up(rot * arVector3(0,1,0));
  arVector3 forward(rot * arVector3(0,0,-1));
  // negate z, to swap handedness of coord systems between Syzygy and FMOD
  pos[2] *= -1.;
  up[2] *= -1.;
  forward[2] *= -1.;
  Normalize(up);
  Normalize(forward);

  if (pos!=_posPrev || up!=_upPrev || forward!=_forwardPrev) {
    _posPrev = pos;
    _upPrev = up;
    _forwardPrev = forward;
    // cout << "listenerpos: " << pos << "\n\t" << forward << "\n\t" << up << endl;;
    arVector3 temp(0,0,0);
    FSOUND_3D_Listener_SetAttributes(
      pos.v,
      temp.v, // doppler NYI (units per second, not per frame!)
      forward[0], forward[1], forward[2],
      up[0], up[1], up[2]);
    FSOUND_Update();
  }
}

arMatrix4 arSpeakerObject::demoHeadMatrix( const arMatrix4& headMatrix ) {
#ifdef DISABLED_UNTIL_I_UNDERSTAND_THIS
  const arVector3 demoHeadPos( ar_extractTranslation(headMatrix) );
  const arVector3 zHat = _normal/++_normal; // '++' = magnitude
  const arVector3 xHat = zHat * _up/++_up;  // '*' = cross product
  const arVector3 yHat = xHat * zHat;
  const arVector3 yPrime = cos( _demoHeadUpAngle )*yHat - sin( _demoHeadUpAngle )*xHat;
  const arVector3 xPrime = zHat * yPrime;
 
  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4) {
    demoRotMatrix[i] = xPrime[j];
    demoRotMatrix[i+1] = yPrime[j];
    demoRotMatrix[i+2] = zHat[j++];
  }
  return ar_translationMatrix(demoHeadPos) * demoRotMatrix;
#else
  return ar_identityMatrix();
#endif
}