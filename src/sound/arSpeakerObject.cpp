//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Most of this is copypasted from graphics/arScreenObject.cpp

#include "arPrecompiled.h"
#include "arSpeakerObject.h"
#include "arSoundDatabase.h" // for mode_vss etc.
#include "fmodStub.h"

arSpeakerObject::arSpeakerObject() :
#ifdef DISABLED_UNTIL_I_UNDERSTAND_THIS
  _demoMode(false),
  _demoHeadUpAngle(0.),
  _normal( 0, 0, -1 ),
  _up( 0, 1, 0 ),
#endif
  _midEyeOffset(-6./(2.54*12), 0, 0),
  _fFmodPluginInited(false)
{
  // Force first loadMatrices() to run.
  memset(&_headPrev, 0, sizeof(_headPrev));
}

bool arSpeakerObject::configure(arSZGClient& cli) {
  (void)cli.initResponse(); // like arGraphicsScreen::configure()
  return true;
}

arMatrix4 __globalSoundListener;
extern arMatrix4 __globalSoundListener;

const float FEET_PER_METER(3.28);

void arSpeakerObject::setUnitConversion(const float unitConversion) {
  float dopplerscale;
  float distancefactor;
  float rolloffscale;
  FMOD_System_Get3DSettings( ar_fmod(), &dopplerscale, &distancefactor, &rolloffscale );
  distancefactor = FEET_PER_METER*unitConversion;
//  ar_log_debug() << "arSpeakerObject sound units: " << distancefactor << ar_endl;
  FMOD_System_Set3DSettings( ar_fmod(), dopplerscale, distancefactor, rolloffscale );
}


bool arSpeakerObject::loadMatrices(const arMatrix4& mHead, const int mode) {
  arMatrix4 head(/*_demoMode ? demoHeadMatrix(mHead) :*/ mHead);

  if (head == _headPrev)
    return true;
  _headPrev = head;

  // Listener moved, so update listener's attributes.

  switch (mode) {

  case mode_fmodplugins:
    // Hide listener motion from the FMOD plugins:
    // transform listener and sources so listener is
    // pos (0, 0, 0) fwd (0, 0, -1) up (0, 1, 0).
    __globalSoundListener = head.inverse();

    if (!_fFmodPluginInited) {
      _fFmodPluginInited = true;
      head = arMatrix4();
      // Set listener exactly once.
      goto LFmod;
    }
    return true;

  case mode_fmod:
  LFmod: {
    const arMatrix4 rot(ar_extractRotationMatrix(head));
    // safe to normalize because rot can't be zero
    const arVector3 up((rot * arVector3(0, 1, 0)).normalize());
    const arVector3 forward((rot * arVector3(0, 0, -1)).normalize());
    const arVector3 pos( ar_extractTranslation(head) );

    //cout << "mode_fmod listenerpos:\n\t" << pos << "\n\t" << forward << "\n\t" << up << "\n\n";;
    const arVector3 velocityNotUsed(0, 0, 0);
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

  case mode_vss:
    ar_log_error() << "mode SZG_SOUND/render vss NYI.\n";
    return false;

  case mode_mmio:
    ar_log_error() << "mode SZG_SOUND/render mmio NYI.\n";
    return false;

  default:
    ar_log_error() << "internal error with SZG_SOUND/render.\n";
    return false;
  }
}

arMatrix4 arSpeakerObject::demoHeadMatrix( const arMatrix4& /*mHead*/ ) {
#ifdef DISABLED_UNTIL_I_UNDERSTAND_THIS
  // copypaste from arVRCamera::_getFixedHeadModeMatrix and
  // ar_tileScreenOffset and ar_frustumMatrix
  // bug: test or prove that _normal and _up are not zero.
  const arVector3 demoHeadPos( ar_ET(mHead) );
  const arVector3 zHat(_normal.normalize());
  const arVector3 xHat(zHat * _up.normalize());
  const arVector3 yHat(xHat * zHat);
  const arVector3 yPrime(cos( _demoHeadUpAngle )*yHat - sin( _demoHeadUpAngle )*xHat);
  const arVector3 xPrime(zHat * yPrime);

  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4, j++) {
    demoRotMatrix[i  ] = xPrime.v[j];
    demoRotMatrix[i+1] = yPrime.v[j];
    demoRotMatrix[i+2] = zHat.v[j];
  }
  return ar_translationMatrix(demoHeadPos) * demoRotMatrix;
#else
  return arMatrix4();
#endif
}
