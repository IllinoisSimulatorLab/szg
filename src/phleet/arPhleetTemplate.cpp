//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPhleetTemplate.h"

arPhleetTemplate::arPhleetTemplate(const string& name) {
  setName(name);
  (void)add("phleet_user", AR_CHAR);
  (void)add("phleet_context", AR_CHAR);
  (void)add("phleet_auth", AR_CHAR);
  (void)add("phleet_match", AR_INT);
}
