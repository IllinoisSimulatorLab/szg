//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LIGHT_FLOAT_BUFFER_H
#define AR_LIGHT_FLOAT_BUFFER_H

#include "arDataType.h"
#include "arLanguageCalling.h"

// Utility class.  A simple buffer of floats. This class is a little
// annoying. It is definitely a hack. First of all, it's a weird relic
// of the early days of szg and particularly the arGraphicsDatabase, whose
// nodes, for no real discernable reason, store all their internal
// information in an array of floats! Well, eventually that should be
// thrown out! And there is the (more useful) template class arBuffer...

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
