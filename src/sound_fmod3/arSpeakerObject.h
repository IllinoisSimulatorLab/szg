//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPEAKER_OBJECT_H
#define AR_SPEAKER_OBJECT_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arSoundCalling.h"

// "Point of view" of who is listening to the sounds in the scene graph.

class SZG_CALL arSpeakerObject {
 public:
  arSpeakerObject();
  ~arSpeakerObject() {}
  bool configure(arSZGClient*);
  void loadMatrices(const arMatrix4&);
  void setUnitConversion(float unitConversion)
    { _unitConversion = unitConversion; }

 private:
  float _unitConversion;
  bool _demoMode;
  float _demoHeadUpAngle;
  arVector3 _normal;
  arVector3 _up;
  arVector3 _midEyeOffset; // vector from head tracker to midpoint of eyes, in feet
  			   // (todo: to ears, not eyes)

  arVector3 _posPrev, _upPrev, _forwardPrev;

  arMatrix4 demoHeadMatrix( const arMatrix4& );
};

#endif
