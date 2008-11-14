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
  bool loadMatrices(const arMatrix4&, const int rendermode);
  void setUnitConversion(const float unitConversion);

 private:
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
};

#endif
