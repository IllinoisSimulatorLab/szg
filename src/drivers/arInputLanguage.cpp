//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arInputLanguage.h" 

arInputLanguage::arInputLanguage():
  _input("input"),
  _SIGNATURE(_input.add("signature",AR_INT)),
  _TIMESTAMP(_input.add("timestamp",AR_INT)),
  _TYPES(_input.add("types",AR_INT)),
  _INDICES(_input.add("indices",AR_INT)),
  _BUTTONS(_input.add("buttons",AR_INT)),
  _AXES(_input.add("axes",AR_FLOAT)),
  _MATRICES(_input.add("matrices",AR_FLOAT)),
  AR_INPUT(_dictionary.add(&_input))
{}

arInputLanguage::~arInputLanguage(){
  // does nothing so far!
}
