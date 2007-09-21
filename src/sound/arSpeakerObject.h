//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPEAKER_OBJECT_H
#define AR_SPEAKER_OBJECT_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arSoundCalling.h"

// "Point of view" of listener of sounds in the scene graph.

class SZG_CALL arSpeakerObject {
 public:
  arSpeakerObject();
  bool configure(arSZGClient&);
  bool loadMatrices(const arMatrix4&);
  void setUnitConversion(float unitConversion)
    { _unitConversion = unitConversion; }

 private:
  float _unitConversion;
#ifdef DISABLED_UNTIL_I_UNDERSTAND_THIS
  bool _demoMode;
  float _demoHeadUpAngle;
  arVector3 _normal;
  arVector3 _up;
#endif
  arVector3 _midEyeOffset; // vector from head tracker to midpoint of eyes, in feet
  arMatrix4 _headPrev;
  bool _fFmodPluginInited;

  arMatrix4 demoHeadMatrix( const arMatrix4& );

  typedef enum { mode_fmod, mode_fmodplugins, mode_vss, mode_mmio } rendermode;
  rendermode _mode;
    // fmod:        thin wrapper around 'gamer' 2-speaker style.
    // fmodplugins: transform sources to compensate for stationary listener
    // vss:         todo, as library, not separate exe
    // mmio:        todo, windows legacy code (fallback if fmod's missing).
};

#endif
