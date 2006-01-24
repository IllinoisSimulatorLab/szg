//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arObject.h"
#include "arGraphicsAPI.h"

bool arObject::attachMesh(const string& objectName, const string& parentName){
  arGraphicsNode* g = dgGetNode(parentName);
  if (g){
    return attachMesh(g, objectName);
  }
  return false;
}
