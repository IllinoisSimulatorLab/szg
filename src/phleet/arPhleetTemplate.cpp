//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPhleetTemplate.h"

arPhleetTemplate::arPhleetTemplate(const string& name){
  setName(name);
  add("phleet_user",AR_CHAR);
  add("phleet_context",AR_CHAR);
  add("phleet_auth",AR_CHAR);
  add("phleet_match",AR_INT);
}

arPhleetTemplate::~arPhleetTemplate(){

}
