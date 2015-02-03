//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include <sstream>

static string __version("");

string ar_versionString(bool fVerbose) {
  if (__version == "") {
    ostringstream os;
    os << SZG_MAJOR_VERSION << "."
       << SZG_MINOR_VERSION << "."
       << SZG_PATCH_VERSION
#ifdef AR_USE_DEBUG
       << " (DEBUG)"
#endif
    ;
    __version = os.str();
  }
  return fVerbose ? ("Syzygy version: " + __version + ".\n") : __version;
}

static const string __default_revinfo =
"Please install Python <http://python.org/> and bazaar <http://bazaar-vcs.org> and rebuild Syzygy.\n";
static string __git_revinfo("REPLACE_THIS");

string ar_versionInfo(bool fVerbose) {
  if (__git_revinfo == "REPLACE_THIS") {
    __git_revinfo = __default_revinfo;
  }
  return fVerbose ? "Version info:\n" + __git_revinfo + "\n" : __git_revinfo;
}
