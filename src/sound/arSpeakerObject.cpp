//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Most of this is copypasted from graphics/arScreenObject.cpp

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

bool arSpeakerObject::configure(arSZGClient& cli){
  (void)cli.initResponse(); // like arGraphicsScreen::configure()

  const string renderMode(cli.getAttribute("SZG_SOUND", "render",
    "|fmod|fmod_plugins|vss|mmio|"));
  ar_log_debug() << "mode SZG_SOUND/render '" << renderMode << "'.\n";
  mode =
    renderMode == "fmod_plugins" ?  mode_fmodplugins :
    renderMode == "vss" ?  mode_vss :
    renderMode == "mmio" ? mode_mmio :
    mode_fmod;
  return true;
}

bool arSpeakerObject::loadMatrices(const arMatrix4& mHead) {
  const arMatrix4 head(_demoMode ? demoHeadMatrix(mHead) : mHead);
  const arVector3 midEye(_unitConversion * (head * _midEyeOffset));

  // Update listener's attributes.
  const arMatrix4 rot(ar_extractRotationMatrix(head));
  const arVector3 up((rot * arVector3(0,1,0)).normalize());
  const arVector3 forward((rot * arVector3(0,0,-1)).normalize());
  const arVector3 pos(midEye);

  if (mode == mode_fmodplugins) {
    // Transform listener and sources so that
    // listener is pos (0,0,0) fwd (0,0,-1) up (0,1,0).
    // This hides listener motion from the FMOD plugins.

    // ar_lookatMatrix does this?
    cerr << "yoyoyo get a list of sound sources and tweak them.\n";

    /*
    Better: FMOD_System_Set3DListenerAttributes just once.
    Where are 3D attributes of sound sources set?
    */
  }

  if (pos==_posPrev && up==_upPrev && forward==_forwardPrev)
    return true;

  _posPrev = pos;
  _upPrev = up;
  _forwardPrev = forward;
  // cout << "listenerpos: " << pos << "\n\t" << forward << "\n\t" << up << endl;;
  const arVector3 velocityNotUsed(0,0,0);
#ifndef EnableSound
  return true;
#else
  const FMOD_VECTOR fmod_pos(FmodvectorFromArvector(pos));
  const FMOD_VECTOR fmod_velocityNotUsed(FmodvectorFromArvector(velocityNotUsed));
  const FMOD_VECTOR fmod_forward(FmodvectorFromArvector(forward));
  const FMOD_VECTOR fmod_up(FmodvectorFromArvector(up));
  return ar_fmodcheck( FMOD_System_Set3DListenerAttributes( ar_fmod(), 0,
    &fmod_pos,
    &fmod_velocityNotUsed, // doppler NYI (units per second, not per frame!)
    &fmod_forward,
    &fmod_up)) &&
    ar_fmodcheck( FMOD_System_Update( ar_fmod() ));
#endif
}

arMatrix4 arSpeakerObject::demoHeadMatrix( const arMatrix4& /*mHead*/ ) {
#ifdef DISABLED_UNTIL_I_UNDERSTAND_THIS
  // copypaste from arVRCamera::_getFixedHeadModeMatrix and
  // ar_tileScreenOffset and ar_frustumMatrix
  const arVector3 demoHeadPos( ar_extractTranslation(mHead) );
  const arVector3 zHat(_normal.normalize());
  const arVector3 xHat(zHat * _up.normalize());
  const arVector3 yHat(xHat * zHat);
  const arVector3 yPrime(cos( _demoHeadUpAngle )*yHat - sin( _demoHeadUpAngle )*xHat);
  const arVector3 xPrime(zHat * yPrime);
 
  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4,j++) {
    demoRotMatrix[i  ] = xPrime.v[j];
    demoRotMatrix[i+1] = yPrime.v[j];
    demoRotMatrix[i+2] = zHat.v[j];
  }
  return ar_translationMatrix(demoHeadPos) * demoRotMatrix;
#else
  return ar_identityMatrix();
#endif
}
