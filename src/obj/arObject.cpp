//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arObject.h"
//#include "arGraphicsAPI.h"

/*
bool arObject::attachMesh(const string& objectName, const string& parentName) {
  arGraphicsNode* g = dgGetNode(parentName);
  return g && attachMesh(g, objectName);
}
*/

int arObject::numberOfTriangles() {return -1;}
int arObject::numberOfNormals() {return -1;}
int arObject::numberOfVertices() {return -1;}
int arObject::numberOfMaterials() {return -1;}
int arObject::numberOfTexCoords() {return -1;}
int arObject::numberOfSmoothingGroups() {return -1;}
int arObject::numberOfGroups() {return -1;}
int arObject::numberOfFrames() const { return -1; }
int arObject::currentFrame() const { return -1; }
