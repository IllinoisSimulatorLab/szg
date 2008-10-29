//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
//#include "arObject.h"
//#include "arGraphicsAPI.h"

// mingw win64 g++ 3.4.5 says:
// warning: non-inline function 'virtual bool arObject::attachMesh(const std::string&, const std::string&)' is defined after prior declaration as dllimport: attribute ignored
// This eventually produces subtle link errors in obj/ar*.o:
//   undefined reference to `__imp___ZTV8arObject'
//   undefined reference to `vtable for arObject'
// The error vanishes when attachMesh is inlined in arObject.h.
//
// Other SZG_CALL base-class defaults for virtual functions don't warn like this,
// e.g. drivers/arIOFilter.cpp arIOFilter::configure(), which
// is overridden properly by arFaroCalFilter.cpp.

/*
bool arObject::attachMesh(const string& objectName, const string& parentName) {
  arGraphicsNode* g = dgGetNode(parentName);
  return g && attachMesh(g, objectName);
}
*/
