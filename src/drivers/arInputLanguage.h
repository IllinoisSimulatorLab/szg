//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_LANGUAGE
#define AR_INPUT_LANGUAGE

#include "arLanguage.h"
#include "arDataType.h"
#include "arDriversCalling.h"

// Formalism for input devices.

class SZG_CALL arInputLanguage: public arLanguage{
 public:
  arInputLanguage();
  ~arInputLanguage();

 protected:
  arDataTemplate _input;

 public:
  int _SIGNATURE; // how many buttons, axes, matrices
  int _TIMESTAMP;
  int _TYPES;
  int _INDICES;   // for subsets
  int _BUTTONS;   // bool
  int _AXES;      // double
  int _MATRICES;  // 4x4 matrix transformation

  int AR_INPUT;
};

#endif
