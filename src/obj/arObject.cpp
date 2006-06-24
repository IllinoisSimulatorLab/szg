//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arObject.h"
#include "arGraphicsAPI.h"

bool arObject::attachMesh(const string& objectName, const string& parentName){
  arGraphicsNode* g = dgGetNode(parentName);
  return g && attachMesh(g, objectName);
}
