//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LIGHT_FLOAT_BUFFER_H
#define AR_LIGHT_FLOAT_BUFFER_H

#include "arDataType.h"
#include "arLanguageCalling.h"

// Utility class, a buffer of floats.
// Relic of early design, where arGraphicsDatabase nodes store
// state in an array of floats.  See class arBuffer instead.

class SZG_CALL arLightFloatBuffer{
 // Needs assignment operator and copy constructor, for pointer member.
 public:
  arLightFloatBuffer(int numFloats = 1);
  ~arLightFloatBuffer();

  int size() const { return _numFloats; }
  void resize(int);
  void grow(int);
  void setType(int);
  int getType() const { return _type; }

  ARfloat* v;
 private:
  int _numFloats;
  // Sometimes we want to be able to attach a type identifier,
  // for instance to embed decoding information in the data.
  int _type;
};

#endif
