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
  bool _demoMode;
  float _demoHeadUpAngle;
  arVector3 _normal;
  arVector3 _up;
  arVector3 _midEyeOffset; // vector from head tracker to midpoint of eyes, in feet
  arVector3 _posPrev, _upPrev, _forwardPrev;

  arMatrix4 demoHeadMatrix( const arMatrix4& );

  typedef enum { mode_fmod, mode_fmodplugins, mode_vss, mode_mmio } rendermode;
  rendermode mode;
};

#endif
