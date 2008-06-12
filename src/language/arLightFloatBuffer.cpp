//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLightFloatBuffer.h"
#include <string>
#include <iostream>
using namespace std;

arLightFloatBuffer::arLightFloatBuffer(int numFloats) {
  if (numFloats<1) {
    cerr << "warning: arLightFloatBuffer initialized with nonpositive size "
         << numFloats << endl;
    numFloats = 1;
  }
  _numFloats = numFloats;
  v = new ARfloat[_numFloats];
  _type = -1;
}

arLightFloatBuffer::~arLightFloatBuffer() {
  delete [] v;
}

void arLightFloatBuffer::resize(int numFloats) {
  if (numFloats<1) {
    cerr << "warning: arLightFloatBuffer resized with nonpositive size "
         << numFloats << endl;
    numFloats=1;
  }
  ARfloat* newPtr = new ARfloat[numFloats];
  memset(newPtr, 0,  numFloats * sizeof(ARfloat));
  if (numFloats < _numFloats)
    {
      // DO NOT WARN HERE. THIS IS NOT NECESSARILY BAD.
      //cerr << "warning: arLightFloatBuffer shrinking from "
      //   << _numFloats << " to " << numFloats << " elements.\n";
    _numFloats = numFloats; // we're shrinking, not growing!
    }
  memcpy(newPtr, v, _numFloats * sizeof(ARfloat));
  delete [] v;
  v = newPtr;
  _numFloats = numFloats;
}

void arLightFloatBuffer::grow(int numFloats) {
  if (numFloats > size())
    resize(numFloats);
}

void arLightFloatBuffer::setType(int theType) {
  _type = theType;
}
