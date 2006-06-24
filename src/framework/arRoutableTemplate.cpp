//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arRoutableTemplate.h"

arRoutableTemplate::arRoutableTemplate(const string& name){
  setName(name);
  add("szg_router_id", AR_INT);
}

arRoutableTemplate::~arRoutableTemplate(){
}
