//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include <sstream>

static string __version("");

string ar_versionString() {
  if (__version == "") {
    ostringstream os;
    os << SZG_MAJOR_VERSION << "."
       << SZG_MINOR_VERSION << "."
       << SZG_PATCH_VERSION;
#ifdef AR_USE_DEBUG
    os << " (DEBUG)";
#endif
    __version = os.str();
  }
  return __version;
}


static const string __default_revinfo =
"Please install Python <http://www.python.org/> and bazaar <http://bazaar-vcs/org> and rebuild szg.";
static string __bzr_revinfo("REPLACE_THIS");


string ar_versionInfo() {
  if (__bzr_revinfo == "REPLACE_THIS") {
    __bzr_revinfo = __default_revinfo;
  }
  return __bzr_revinfo;
}


