//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_HEAD_H
#define AR_HEAD_H

#include "arMath.h"

class SZG_CALL arHead{
 public:
  arHead(){ eyeSpacing = 0; nearClip = 1; farClip = 100; unitConversion = 1; }
  ~arHead(){}

  arMatrix4 transform;
  arVector3 midEyeOffset;
  arVector3 eyeDirection;
  float eyeSpacing;
  float nearClip;
  float farClip;
  float unitConversion;
};

#endif
