//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arRoutableTemplate.h"

arRoutableTemplate::arRoutableTemplate(const string& name){
  setName(name);
  add("szg_router_id", AR_INT);
}

arRoutableTemplate::~arRoutableTemplate(){
}
