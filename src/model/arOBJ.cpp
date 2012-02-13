//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arOBJ.h"
#include "arGraphicsAPI.h"
#include "arLogStream.h"

arOBJ::arOBJ() :
  _thisMaterial(0),
  _thisSG(0),
  _thisGroup(0),
  _searchPath(string("")) {
}

arOBJ::~arOBJ() {
}

// Reads in a .OBJ file, prints out an error if invalid
// @param inputFile file pointer to read in data
// \bug calling this twice on the same object has undefined behaviour
bool arOBJ::readOBJ(FILE* inputFile) {
  arOBJMaterial tempMaterial;
  _material.push_back(arOBJMaterial());
  sprintf(_material[0].name, "default");
  _material[0].Kd = arVector3(1, 1, 1);
  _normal.push_back(arVector3(0, 0, 0));
  _texCoord.push_back(arVector3(0, 0, 0));
  _smoothingGroup.push_back(arOBJSmoothingGroup());
  _smoothingGroup[0]._name = 0;
  _group.push_back(vector<int>());
  _groupName.push_back("default");

  if (!inputFile) {
    ar_log_error() << "arOBJ: NULL input file.";
    _invalidFile = true;
    return false;
  }

  // parse the entire file
  while (_parseOneLine(inputFile)) {}

  _generateNormals();
  return true;
}

// wrapper for the 3 parameter readOBJ(...)
bool arOBJ::readOBJ(const string& fileName, const string& path) {
  return readOBJ(fileName, "", path);
}

// opens a File and calls readOBJ(FILE*)
// @param fileName name of OBJ file (including extension) to read from
// @param subdirectory is the subdirectory of the search path in which
// we look for the files, which allows us to store stuff for a program in
// data_directory/my_program_name instead of just data_directory.
// @param path a search path upon which we look for the file. this is useful
// when data files need to be installed in a special directory
bool arOBJ::readOBJ(const string& fileName,
                    const string& subdirectory,
                    const string& path) {
  _fileName = string(fileName);
  _searchPath = path;
  _subdirectory = subdirectory;
  FILE* theFile = ar_fileOpen(fileName, subdirectory, path, "r", "readOBJ");
  if (!theFile) {
    _invalidFile = true;
    return false;
  }
  bool ok = readOBJ(theFile);
  ar_fileClose(theFile);
  return ok;
}

//@param name The name of the group of which you want the ID
//Gets the numerical ID of a geometry group, given the name
int arOBJ::groupID(const string& name) {
  for (unsigned i=0; i<_groupName.size(); ++i) {
    if (name == _groupName[i])
      return i;
  }
  return -1;
}

//@param transform The new root transformation matrix
// Sets the global transform matrix for the OBJ
void arOBJ::setTransform(const arMatrix4& transform) {
  _transform = transform;
}

bool arOBJ::attachMesh(const string& objectName, const string& parentName) {
  arGraphicsNode* g = dgGetNode(parentName);
  return g && attachMesh(g, objectName);
}

// @param where the scenegraph node to which we attach the OBJ.
// @param baseName The name of the entire object.
// Puts the OBJ into scenegraph w/o hierarchy or transforms.
bool arOBJ::attachMesh(arGraphicsNode* where, const string& baseName) {
  if (_invalidFile) {
    // already complained
    return false;
  }
  arGraphicsNode* pointsNode = attachPoints(where, baseName+".points");
  if (!pointsNode) {
    return false;
  }
  for (unsigned i=0; i<_group.size(); ++i) {
    if (!_group[i].empty() && !attachGroup(pointsNode, i, baseName)) {
      return false;
    }
  }
  return true;
}

// @param where The node to which we will attach the points information.
// @param nodeName The name to give the points node.
// Just attaches points (vertices) of the OBJ.  No geometry or transforms.
arGraphicsNode* arOBJ::attachPoints(arGraphicsNode* where,
                                    const string& nodeName) {
  if (_invalidFile) {
    // already complained
    return NULL;
  }
  if (!where) {
    ar_log_error() << "arOBJ cannot attach points: no base node.\n";
    return NULL;
  }

  arPointsNode* p = (arPointsNode*) where->newNode("points", nodeName);
  p->setPoints(_vertex);
  // Pass the node ID to the superclass.
  _vertexNodeID = p->getID();
  return p;

  /*const int numberPoints=_vertex.size();
  float* pointPositions = new float[3*numberPoints];
  int* pointIDs = new int[numberPoints];
  for (int i=0; i<numberPoints; i++) {
    pointPositions[3*i  ] = _vertex[i][0];
    pointPositions[3*i+1] = _vertex[i][1];
    pointPositions[3*i+2] = _vertex[i][2];
    pointIDs[i] = i;
  }
  _vertexNodeID =
  dgPoints(baseName, where,
           numberPoints, pointIDs, pointPositions);
  delete [] pointIDs;
  delete [] pointPositions;
 */
}

// @param where The parent node to which we will attach everything.
// @param groupNum The OBJ group number to attach
// @param base The base name of the group
// This attaches geometry, colors, etc. but NOT POINTS to node.
bool arOBJ::attachGroup(arGraphicsNode* where, int groupID, const string& base) {
  if (_invalidFile) {
    // already complained
    return false;
  }
  if (!where) {
    ar_log_error() << "arOBJ cannot attach group: no parent.\n";
    return false;
  }
  const string trianglesModifier(".triangles");
  const string normalsModifier  (".normals");
  const string colorsModifier   (".colors");
  const string texCoordModifier (".texCoords");
  const string textureModifier  (".texture:");
  const string baseName((base=="" ? "myOBJ." : base+".") + _groupName[groupID]);

  vector<int>& thisGroup = _group[groupID];

  // Attach triangles (faces)
  const int numberTriangles = thisGroup.size();

  // Attach colors and textures.
  // Sort triangles by material.
  vector<int>* triangleMaterialIDs = new vector<int>[_material.size()];
  for (unsigned tri=0; tri<unsigned(numberTriangles); ++tri) {
    triangleMaterialIDs[_triangle[thisGroup[tri]].material].push_back(tri);
  }

  for (unsigned matID=0; matID<_material.size(); ++matID) {
    vector<int>& thisMatTriangleIDs = triangleMaterialIDs[matID];
    if (thisMatTriangleIDs.empty()) {
      // No triangles use this material.
      continue;
    }

    unsigned numTriUsingMaterial = thisMatTriangleIDs.size();
    float* normals   = new float[9*numTriUsingMaterial];
    int*   indices   = new int[3*numTriUsingMaterial];
    float* texCoords = new float[6*numTriUsingMaterial];
    arOBJMaterial& thisMaterial = _material[matID];
    const bool useTexture = thisMaterial.map_Kd != "none";

    for (unsigned i=0; i<numTriUsingMaterial; ++i) {
      const arOBJTriangle currentTriangle = _triangle[thisGroup[thisMatTriangleIDs[i]]];
      indices[3*i  ] = currentTriangle.vertices[0];
      indices[3*i+1] = currentTriangle.vertices[1];
      indices[3*i+2] = currentTriangle.vertices[2];
      normals[9*i  ] = _normal[currentTriangle.normals[0]][0];
      normals[9*i+1] = _normal[currentTriangle.normals[0]][1];
      normals[9*i+2] = _normal[currentTriangle.normals[0]][2];
      normals[9*i+3] = _normal[currentTriangle.normals[1]][0];
      normals[9*i+4] = _normal[currentTriangle.normals[1]][1];
      normals[9*i+5] = _normal[currentTriangle.normals[1]][2];
      normals[9*i+6] = _normal[currentTriangle.normals[2]][0];
      normals[9*i+7] = _normal[currentTriangle.normals[2]][1];
      normals[9*i+8] = _normal[currentTriangle.normals[2]][2];
      if (useTexture) {
        texCoords[6*i  ] = _texCoord[currentTriangle.texCoords[0]][0];
        texCoords[6*i+1] = _texCoord[currentTriangle.texCoords[0]][1];
        texCoords[6*i+2] = _texCoord[currentTriangle.texCoords[1]][0];
        texCoords[6*i+3] = _texCoord[currentTriangle.texCoords[1]][1];
        texCoords[6*i+4] = _texCoord[currentTriangle.texCoords[2]][0];
        texCoords[6*i+5] = _texCoord[currentTriangle.texCoords[2]][1];
      }
    }

    const string matIDBuf(ar_intToString(matID));
    // Three colors per triangle.
    arIndexNode* indexNode = (arIndexNode*) where->newNode("index", baseName+".indices"+matIDBuf );
    indexNode->setIndices( 3*numTriUsingMaterial, indices );
    arNormal3Node* normalNode = (arNormal3Node*) indexNode->newNode("normal3", baseName+normalsModifier+matIDBuf );
    normalNode->setNormal3( 3*numTriUsingMaterial, normals );

    // Some exporters attach "pure black" color to anything textures.
    // Replace that with (1, 1, 1), so GL_MODULATE works with textures.
    arVector3 correctedDiffuse(thisMaterial.Kd);
    if (correctedDiffuse.magnitude() < 0.003)
      correctedDiffuse = arVector3(1, 1, 1);

    arMaterialNode* materialNode = (arMaterialNode*)
      normalNode->newNode("material", baseName+colorsModifier+matIDBuf );
    arMaterial tmp;
    tmp.diffuse = correctedDiffuse;
    tmp.ambient = thisMaterial.Ka;
    tmp.specular = thisMaterial.Ks;
    tmp.emissive = arVector3(0, 0, 0);
    tmp.exponent = thisMaterial.Ns;
    materialNode->setMaterial(tmp);

    if (useTexture) {
      arTex2Node* tex2Node = (arTex2Node*)
        materialNode->newNode("tex2", baseName+texCoordModifier+matIDBuf );
      tex2Node->setTex2( 3*numTriUsingMaterial, texCoords );
      arTextureNode* textureNode = (arTextureNode*)
        tex2Node->newNode("texture", baseName+textureModifier+matIDBuf );
      textureNode->setFileName( thisMaterial.map_Kd );
      arDrawableNode* drawableNode = (arDrawableNode*)
        textureNode->newNode("drawable", baseName+".geometry"+matIDBuf );
      drawableNode->setDrawable( DG_TRIANGLES, numTriUsingMaterial );
    } else {
      arDrawableNode* drawableNode = (arDrawableNode*) materialNode->newNode("drawable", baseName+".geometry"+matIDBuf );
      drawableNode->setDrawable( DG_TRIANGLES, numTriUsingMaterial );
    }

    delete [] texCoords;
    delete [] normals;
    delete [] indices;
  }

  delete [] triangleMaterialIDs;
  return true;
}


// @param groupID The OBJ internal group number
// returns a sphere centered at the average of all the triangles in the group
// with a radius that includes all the geometry
arBoundingSphere arOBJ::getGroupBoundingSphere(int groupID) {
  int i=0, j=0;
  // Traverse the triangle list, accumulating points' average position
  const int numberTriangles = _group[groupID].size();
  arVector3 center;
  for (i=0; i<numberTriangles; i++) {
    for (j=0; j<3; j++) {
      const arVector3& vertexCoord =
        _vertex[_triangle[_group[groupID][i]].vertices[j]];
      center += vertexCoord;
    }
  }
  center /= numberTriangles*3;

  // Retraverse the list: find the point farthest from the origin.
  float radius2 = 0;
  for (i=0; i<numberTriangles; i++) {
    for (j=0; j<3; j++) {
      const arVector3& vertexCoord =
        _vertex[_triangle[_group[groupID][i]].vertices[j]];
      const float dist2 = (center-vertexCoord).magnitude2();
      if (dist2 > radius2) {
        radius2 = dist2;
      }
    }
  }
  return arBoundingSphere(center, sqrt(radius2));
}

arAxisAlignedBoundingBox arOBJ::getAxisAlignedBoundingBox(int groupID) {
  const int numberTriangles = _group[groupID].size();
  float xMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][0];
  float xMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][0];
  float yMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][1];
  float yMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][1];
  float zMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][2];
  float zMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][2];
  for (int i=0; i<numberTriangles;i++) {
    for (int j=0; j<3; j++) {
      arVector3 vertex = _vertex[_triangle[_group[groupID][i]].vertices[j]];
      if (vertex[0] < xMin) {
        xMin = vertex[0];
      }
      if (vertex[0] > xMax) {
        xMax = vertex[0];
      }
      if (vertex[1] < yMin) {
        yMin = vertex[1];
      }
      if (vertex[1] > yMax) {
        yMax = vertex[1];
      }
      if (vertex[2] < zMin) {
        zMin = vertex[2];
      }
      if (vertex[2] > zMax) {
        zMax = vertex[2];
      }
    }
  }
  arAxisAlignedBoundingBox box;
  box.center = arVector3( (xMin+xMax)/2.0, (yMin+yMax)/2.0, (zMin+zMax)/2.0);
  box.xSize = (xMax-xMin)/2.0;
  box.ySize = (yMax-yMin)/2.0;
  box.zSize = (zMax-zMin)/2.0;
  return box;
}

float arOBJ::intersectGroup(int groupID, const arRay& theRay) {
  float intersectionDistance = -1;
  int numberTriangles = _group[groupID].size();
  for (int i=0; i<numberTriangles; i++) {
    const arVector3& v1 = _vertex[_triangle[_group[groupID][i]].vertices[0]];
    const arVector3& v2 = _vertex[_triangle[_group[groupID][i]].vertices[1]];
    const arVector3& v3 = _vertex[_triangle[_group[groupID][i]].vertices[2]];
    const float newDist = ar_intersectRayTriangle(theRay, v1, v2, v3);
    if (intersectionDistance < 0 ||
        (newDist > 0 && newDist < intersectionDistance)) {
      intersectionDistance = newDist;
    }
  }
  return intersectionDistance;
}

// Inputs data on "face" line specified in OBJ file
// @param numTokens how many vertices in this face
// @param token the actual data on this line, separated nicely
void arOBJ::_parseFace(int numTokens, char *token[]) {
  const int howManyVertices = numTokens-1;
  int* vertexID = new int[numTokens-1];
  int* texCoordID = new int[numTokens-1];
  int* normalID = new int[numTokens-1];
  int i=0;
  char *vertexToken[4] = {0};
  for (i=0; i<numTokens-1; i++) {
    texCoordID[i] = normalID[i] = 0;

    int numVTokens = 0;
    vertexToken[numVTokens++] = strtok(token[i+1], "/");
    while (vertexToken[numVTokens-1]) {
      vertexToken[numVTokens++] = strtok(NULL, "/");
    }
    --numVTokens;

    if (numVTokens == 1) {
      vertexID[i] = atoi(vertexToken[0]);
    } else {
      vertexID[i] = atoi(vertexToken[0]);
      texCoordID[i] = vertexToken[1]?atoi(vertexToken[1]):0;
      normalID[i] = vertexToken[2]?atoi(vertexToken[2]):0;
    }
  }

  // adjust negative indices
  for (i=0; i<howManyVertices; i++) {
    if (vertexID[i] < 0)
      vertexID[i] = _vertex.size()+vertexID[i]-1;
    if (texCoordID[i] < 0)
      texCoordID[i] = _texCoord.size()+texCoordID[i]-1;
    if (normalID[i] < 0)
      normalID[i] = _normal.size()+normalID[i]-1;
  }

  // handle the geometry
  if (howManyVertices < 5) {
    for (i=0; i<howManyVertices-2; i++) {
      arOBJTriangle tempTriangle;
      tempTriangle.vertices[0] = vertexID[(2*i)%howManyVertices]-1;
      tempTriangle.vertices[1] = vertexID[(2*i+1)%howManyVertices]-1;
      tempTriangle.vertices[2] = vertexID[(2*i+2)%howManyVertices]-1;
      tempTriangle.normals[0] = normalID[(2*i)%howManyVertices]-1;
      tempTriangle.normals[1] = normalID[(2*i+1)%howManyVertices]-1;
      tempTriangle.normals[2] = normalID[(2*i+2)%howManyVertices]-1;
      tempTriangle.texCoords[0] = texCoordID[(2*i)%howManyVertices]-1;
      tempTriangle.texCoords[1] = texCoordID[(2*i+1)%howManyVertices]-1;
      tempTriangle.texCoords[2] = texCoordID[(2*i+2)%howManyVertices]-1;
      tempTriangle.material = _thisMaterial;
      _triangle.push_back(tempTriangle);
      if (_thisSG != 0) {
        _triangle.back().smoothingGroup = _thisSG;
        _smoothingGroup[_thisSG].add(_triangle.size()-1);
      }
      _group[_thisGroup].push_back(_triangle.size()-1);
    }
  } else { // more than 4 vertices
    arVector3 center(0, 0, 0), normalVec(0, 0, 0), texCoordVec(0, 0, 0);
    for (i=1; i<howManyVertices-1; i++) {
      arOBJTriangle tempTriangle;
      tempTriangle.vertices[0] = vertexID[0]-1;
      tempTriangle.vertices[1] = vertexID[i]-1;
      tempTriangle.vertices[2] = vertexID[i+1]-1;
      tempTriangle.normals[0] = normalID[0]-1;
      tempTriangle.normals[1] = normalID[i]-1;
      tempTriangle.normals[2] = normalID[i+1]-1;
      tempTriangle.texCoords[0] = texCoordID[0]-1;
      tempTriangle.texCoords[1] = texCoordID[i]-1;
      tempTriangle.texCoords[2] = texCoordID[i+1]-1;
      tempTriangle.material = _thisMaterial;
      _triangle.push_back(tempTriangle);
      if (_thisSG != 0) {
        _triangle.back().smoothingGroup = _thisSG;
        _smoothingGroup[_thisSG].add(_triangle.size()-1);
      }
      _group[_thisGroup].push_back(_triangle.size()-1);
    }
    //cerr << "polygon with " << howManyVertices << " found!" << endl;
  }
  delete [] vertexID;
  delete [] normalID;
  delete texCoordID;
}

// Adds normals if there are none, smoothes normals in smoothing group, and
// adjust backwards-facing normals
void arOBJ::_generateNormals() {
  unsigned i=0, j=0, k=0;
  _normal.erase(_normal.begin(), _normal.begin()+1);
  _texCoord.erase(_texCoord.begin(), _texCoord.begin()+1);
  for (i=0; i<_triangle.size(); i++) {
    // no normals
    if (_triangle[i].normals[0] == -1 ||
        _triangle[i].normals[1] == -1 ||
        _triangle[i].normals[2] == -1) {
      _normal.push_back(
        (_vertex[_triangle[i].vertices[1]] - _vertex[_triangle[i].vertices[0]]) *
        (_vertex[_triangle[i].vertices[2]] - _vertex[_triangle[i].vertices[0]]) );
      _normal.back() /= ++_normal.back();
      _triangle[i].normals[0] =
      _triangle[i].normals[1] =
      _triangle[i].normals[2] = _normal.size()-1;
    }
  }

  for (i=0; i<_triangle.size(); ++i) {
    arVector3 naturalDirection =
      (_vertex[_triangle[i].vertices[1]] - _vertex[_triangle[i].vertices[0]]) *
      (_vertex[_triangle[i].vertices[2]] - _vertex[_triangle[i].vertices[0]]);
    if (_normal[_triangle[i].normals[0]]%naturalDirection < 0) {
      // reversed normal
      _normal[_triangle[i].normals[0]] = -_normal[_triangle[i].normals[0]];
    }
    if (_normal[_triangle[i].normals[1]]%naturalDirection < 0) {
      // reversed normal
      _normal[_triangle[i].normals[1]] = -_normal[_triangle[i].normals[1]];
    }
    if (_normal[_triangle[i].normals[2]]%naturalDirection < 0) {
      // reversed normal
      _normal[_triangle[i].normals[2]] = -_normal[_triangle[i].normals[2]];
    }
  }

  // List of vertex-indexed smoothing group triangles.
  vector<int>* sgVertex = new vector<int>[_vertex.size()];
  vector<int>* sgVertexNum = new vector<int>[_vertex.size()];
  for (i=0; i<_triangle.size(); i++) {
    for (j=0; j<3; j++) {
      const unsigned k = _triangle[i].vertices[j];
      sgVertex   [k].push_back(i);
      sgVertexNum[k].push_back(j);
    }
  }
  // Traverse vertices and add normals in same SG.
  for (i=0; i<_vertex.size(); i++) {
    if (sgVertex[i].size() != 0)
    for (j=0; j<sgVertex[i].size()-1; j++) {
      if (sgVertex[i][j] != -1 && _triangle[sgVertex[i][j]].smoothingGroup) {
        arVector3 tempNorm = _normal[_triangle[sgVertex[i][j]].normals[sgVertexNum[i][j]]];
        for (k=j+1; k<sgVertex[i].size(); k++) {
           if (sgVertex[i][k] != -1 && _triangle[sgVertex[i][j]].smoothingGroup ==
              _triangle[sgVertex[i][k]].smoothingGroup) {
                  if (tempNorm%_normal[_triangle[sgVertex[i][k]].normals[sgVertexNum[i][k]]]>0)
              tempNorm += _normal[_triangle[sgVertex[i][k]].normals[sgVertexNum[i][k]]];
            else
              tempNorm -= _normal[_triangle[sgVertex[i][k]].normals[sgVertexNum[i][k]]];
            _triangle[sgVertex[i][k]].normals[sgVertexNum[i][k]] = _normal.size();
            sgVertex[i][k] = -1;
          }
        }
        _triangle[sgVertex[i][j]].normals[sgVertexNum[i][j]] = _normal.size();
        tempNorm /= ++tempNorm;
        _normal.push_back(tempNorm);
        sgVertex[i][j] = -1;
      }
    }
  }
}

// Make the object fit in a unit sphere.
// Move its vertices, don't just add a transformation matrix.
void arOBJ::normalizeModelSize() {
  arVector3 maxVec(-1000000, -1000000, -1000000);
  arVector3 minVec(1000000, 1000000, 1000000);
  unsigned i = 0;
  const unsigned iMax = _vertex.size();
  for (i=0; i<iMax; i++) {
    for (unsigned j=0; j<3; j++) {
      if (_vertex[i].v[j] > maxVec.v[j])
        maxVec.v[j] = _vertex[i].v[j];
      if (_vertex[i].v[j] < minVec.v[j])
        minVec.v[j] = _vertex[i].v[j];
    }
  }
  const arVector3 center = (maxVec+minVec)/2.;
  const float maxDist = sqrt((maxVec-minVec)%(maxVec-minVec))/2.;
  for (i=0; i<iMax; ++i)
    _vertex[i] = (_vertex[i] - center) / maxDist;
}

arOBJGroupRenderer::arOBJGroupRenderer() : _name("NULL") {}

arOBJGroupRenderer::~arOBJGroupRenderer() {
  clear();
}

void arOBJGroupRenderer::clear() {
  _name = "NULL";
  _renderer = NULL;
  vector<float*>::iterator fiter;
  for (fiter = _normalsByMaterial.begin(); fiter != _normalsByMaterial.end(); ++fiter) {
    if (*fiter) {
      delete[] *fiter;
    }
  }
  _normalsByMaterial.clear();
  for (fiter = _texCoordsByMaterial.begin(); fiter != _texCoordsByMaterial.end(); ++fiter) {
    if (*fiter) {
      delete[] *fiter;
    }
  }
  _texCoordsByMaterial.clear();
  vector<int*>::iterator iiter;
  for (iiter = _vertexIndicesByMaterial.begin(); iiter != _vertexIndicesByMaterial.end(); ++iiter) {
    if (*iiter) {
      delete[] *iiter;
    }
  }
  _vertexIndicesByMaterial.clear();
  _numTriUsingMaterial.clear();
  _vertexIndices.clear();
}

bool arOBJGroupRenderer::build( arOBJRenderer* renderer,
    const string& groupName,
    vector<int>& thisGroup,
    vector<arVector3>& texCoords,
    vector<arVector3>& normals,
    vector<arOBJTriangle>& triangles ) {

  clear();
  if (!renderer) {
    ar_log_error() << "arOBJGroupRenderer::build() ignoring NULL renderer.\n";
    return false;
  }
  _renderer = renderer;
  _name = groupName;
  const unsigned numberTriangles = thisGroup.size();
  const unsigned numMaterials = _renderer->_textures.size();

  // Attach colors and textures
  // sort triangles by material
  vector< vector<unsigned> > triangleMaterialIDs( numMaterials, vector<unsigned>() );
  for (unsigned tri=0; tri<unsigned(numberTriangles); ++tri) {
    triangleMaterialIDs[triangles[thisGroup[tri]].material].push_back(tri);
  }

  _normalsByMaterial.insert( _normalsByMaterial.begin(), numMaterials, NULL );
  _vertexIndicesByMaterial.insert( _vertexIndicesByMaterial.begin(), numMaterials, NULL );
  _texCoordsByMaterial.insert( _texCoordsByMaterial.begin(), numMaterials, NULL );
  _numTriUsingMaterial.insert( _numTriUsingMaterial.begin(), numMaterials, 0 );

  unsigned numTrianglesInGroup = 0;
  vector< vector<unsigned> >::iterator vi_iter;
  for (vi_iter = triangleMaterialIDs.begin(); vi_iter != triangleMaterialIDs.end(); ++vi_iter) {
    numTrianglesInGroup += vi_iter->size();
  }
  _vertexIndices.reserve( numTrianglesInGroup );

  bool matHasTexture;

  for (unsigned matID=0; matID<numMaterials; ++matID) {
    vector<unsigned>& thisMatTriangleIDs = triangleMaterialIDs[matID];
    unsigned numTriUsingMaterial = thisMatTriangleIDs.size();
    ar_log_debug() << numTriUsingMaterial << " triangles using material #" << matID << ar_endl;
    matHasTexture = _renderer->_textures[matID];

    if (numTriUsingMaterial > 0) {
      float* normalsThisMat = new float[ 9*numTriUsingMaterial ];
      int* indicesThisMat = new int[ 3*numTriUsingMaterial ];
      float* texCoordsThisMat = NULL;
      if (matHasTexture) {
        texCoordsThisMat = new float[ 6*numTriUsingMaterial ];
      }
      if (!normalsThisMat || !indicesThisMat || (matHasTexture && !texCoordsThisMat)) {
        ar_log_error() << "arOBJGroupRenderer failed to allocate buffers.\n";
        return false;
      }
      _normalsByMaterial[matID] = normalsThisMat;
      _vertexIndicesByMaterial[matID] = indicesThisMat;
      _texCoordsByMaterial[matID] = texCoordsThisMat;
      _numTriUsingMaterial[matID] = numTriUsingMaterial;

      vector<unsigned>::const_iterator iter;
      unsigned i, j;
      for (iter = thisMatTriangleIDs.begin(); iter != thisMatTriangleIDs.end(); ++iter) {
        const arOBJTriangle currentTriangle = triangles[thisGroup[*iter]];
        for (j=0; j<3; ++j) {
          *indicesThisMat++ = currentTriangle.vertices[j];
          _vertexIndices.push_back( currentTriangle.vertices[j] );
        }
        for (i=0; i<3; ++i) {
          arVector3& thisNormal = normals[currentTriangle.normals[i]];
          for (j=0; j<3; ++j) {
            *normalsThisMat++ = thisNormal[j];
          }
        }
        if (matHasTexture) {
          for (i=0; i<3; ++i) {
            arVector3& thisTexCoord = texCoords[currentTriangle.texCoords[i]];
            //  This was originally being flipped. I have _no_ idea why. Some
            //  simple test cases were clearly wrong. -JAC 121506.
//            *texCoordsThisMat++ = 1.-thisTexCoord[0];
            *texCoordsThisMat++ = thisTexCoord[0];
            *texCoordsThisMat++ = thisTexCoord[1];
          }
        }
      }
    }
  }
  return true;
}

void arOBJGroupRenderer::draw() {
  if (!_renderer) {
    ar_log_error() << "arOBJGroupRenderer::draw() ignoring NULL renderer.\n";
    return;
  }

  glPushAttrib( GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT );
  const vector<arVector3>& vertices = _renderer->_vertices;
  const int iVertexMax = int(vertices.size());
  vector<arMaterial>& materials = _renderer->_materials;
  const unsigned iMatMax = materials.size();
  vector<arTexture*>& textures = _renderer->_textures;
  vector<bool>& fOpacityMap = _renderer->_fOpacityMap;

  for (unsigned matID = 0; matID < iMatMax; ++matID) {
    const unsigned numVertex = _numTriUsingMaterial[matID] * 3;
    if (numVertex == 0) {
      continue;
    }

    materials[matID].activateMaterial();
    if (fOpacityMap[matID]) {
      glEnable( GL_BLEND );
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }
    if (textures[matID]) {
      textures[matID]->activate();
    }
    glBegin(GL_TRIANGLES);
    const float* const normalsThisMat = _normalsByMaterial[matID];
    const int* const iVertices = _vertexIndicesByMaterial[matID];
    const float* const texCoordsThisMat = _texCoordsByMaterial[matID];

    // Copypaste for performance - test texCoordsThisMat outside loop, not inside.
    if (texCoordsThisMat) {
      for (unsigned i=0; i<numVertex; ++i) {
      glNormal3fv( normalsThisMat+3*i );
      glTexCoord2fv( texCoordsThisMat+2*i );
      const int iVertex = iVertices[i];
      if (iVertex < iVertexMax) {
        glVertex3fv( vertices.begin()[iVertex].v );
        }
      }
    }
    else {
      for (unsigned i=0; i<numVertex; ++i) {
        glNormal3fv( normalsThisMat+3*i );
        const int iVertex = iVertices[i];
        if (iVertex < iVertexMax) {
          glVertex3fv( vertices.begin()[iVertex].v );
        }
      }
    }

    glEnd();
    if (textures[matID]) {
      textures[matID]->deactivate();
    }
  }
  glPopAttrib();
}

// returns a sphere centered at the average of all the triangles in the group
// with a radius that includes all the geometry
arBoundingSphere arOBJGroupRenderer::getBoundingSphere() {
  if (!_renderer) {
    ar_log_error() << "arOBJGroupRenderer::getBoundingSphere() ignoring NULL renderer.\n";
    return arBoundingSphere();
  }
  vector<arVector3>& vertices = _renderer->_vertices;
  if (vertices.size() == 0) {
    ar_log_warning() << "arOBJGroupRenderer::getBoundingSphere() ignoring zero-vertex group.\n";
    return arBoundingSphere();
  }
  if (_vertexIndices.size() == 0) {
    ar_log_warning() << "arOBJGroupRenderer::getBoundingSphere() found no vertex indices.\n";
    return arBoundingSphere();
  }
  // first, walk through the triangle list and accumulate the average position
  arVector3 center(0, 0, 0);
  vector<int>::iterator iter;
  for (iter = _vertexIndices.begin(); iter != _vertexIndices.end(); ++iter) {
    center += vertices[*iter];
  }
  center *= (1./_vertexIndices.size());
  // next, walk through the list a second time, figuring out the maximum
  // distance of a point from the origin
  float radius = 0;
  for (iter = _vertexIndices.begin(); iter != _vertexIndices.end(); ++iter) {
    const float dist = (center-vertices[*iter]).magnitude();
    if (dist > radius) {
      radius = dist;
    }
  }
  return arBoundingSphere(center, radius);
}

arAxisAlignedBoundingBox arOBJGroupRenderer::getAxisAlignedBoundingBox() {
  if (!_renderer) {
    ar_log_error() << "arOBJGroupRenderer::getAxisAlignedBoundingBox() ignoring NULL renderer.\n";
    return arAxisAlignedBoundingBox();
  }
  vector<arVector3>& vertices = _renderer->_vertices;
  if (vertices.size() == 0) {
    ar_log_warning() << "arOBJGroupRenderer::getAxisAlignedBoundingBox() ignoring zero-vertex group.\n";
    return arAxisAlignedBoundingBox();
  }
  if (_vertexIndices.size() == 0) {
    ar_log_warning() << "arOBJGroupRenderer::getAxisAlignedBoundingBox() found no vertex indices.\n";
    return arAxisAlignedBoundingBox();
  }
  if ((_vertexIndices[0] < 0) || ((unsigned)_vertexIndices[0] >= vertices.size())) {
    ar_log_error() << "arOBJGroupRenderer::getAxisAlignedBoundingBox() ignoring invalid vertex index.\n";
    return arAxisAlignedBoundingBox();
  }
  arVector3& firstCoord = vertices[_vertexIndices[0]];
  float xMin = firstCoord[0];
  float xMax = firstCoord[0];
  float yMin = firstCoord[1];
  float yMax = firstCoord[1];
  float zMin = firstCoord[2];
  float zMax = firstCoord[2];
  vector<int>::iterator iter;
  for (iter = _vertexIndices.begin(); iter != _vertexIndices.end(); ++iter) {
    arVector3& vertex = vertices[*iter];
    if (vertex[0] < xMin) {
      xMin = vertex[0];
    }
    if (vertex[0] > xMax) {
      xMax = vertex[0];
    }
    if (vertex[1] < yMin) {
      yMin = vertex[1];
    }
    if (vertex[1] > yMax) {
      yMax = vertex[1];
    }
    if (vertex[2] < zMin) {
      zMin = vertex[2];
    }
    if (vertex[2] > zMax) {
      zMax = vertex[2];
    }
  }
  arAxisAlignedBoundingBox box;
  box.center = 0.5*arVector3( xMin+xMax, yMin+yMax, zMin+zMax );
  box.xSize = 0.5*(xMax-xMin);
  box.ySize = 0.5*(yMax-yMin);
  box.zSize = 0.5*(zMax-zMin);
  return box;
}

float arOBJGroupRenderer::getIntersection( const arRay& theRay ) {
  if (!_renderer) {
    ar_log_error() << "arOBJGroupRenderer::getIntersection() ignoring NULL renderer.\n";
    return 0.;
  }
  vector<arVector3>& vertices = _renderer->_vertices;
  float intersectionDistance = -1;
  const unsigned numberTriangles = _vertexIndices.size()/3;
  vector<int>::const_iterator iter = _vertexIndices.begin();
  for (unsigned i=0; i<numberTriangles; ++i) {
    const arVector3& v1 = vertices[*iter++];
    const arVector3& v2 = vertices[*iter++];
    const arVector3& v3 = vertices[*iter++];
    const float newDist = ar_intersectRayTriangle(theRay, v1, v2, v3 );
    if (intersectionDistance < 0 ||
        (newDist > 0 && newDist < intersectionDistance)) {
      intersectionDistance = newDist;
    }
  }
  return intersectionDistance;
}


arOBJRenderer::arOBJRenderer() :
  _name(""),
  _subdirectory(""),
  _searchPath(""),
  _mipmapTextures(true)
{
}

arOBJRenderer::~arOBJRenderer() {
  clear();
}

// opens a File and calls readOBJ(FILE*)
// @param fileName name of OBJ file (including extension) to read from
// @param subdirectory is the subdirectory of the search path in which
// we look for the files, which allows us to store stuff for a program in
// data_directory/my_program_name instead of just data_directory.
// @param path a search path upon which we look for the file. this is useful
// when data files need to be installed in a special directory
bool arOBJRenderer::readOBJ(const string& fileName,
                    const string& subdirectory,
                    const string& path) {
  _subdirectory = subdirectory;
  _searchPath = path;
  FILE* theFile = ar_fileOpen(fileName, subdirectory, path, "r", "arOBJRenderer readOBJ");
  if (!theFile) {
    ar_log_error() << "ar_fileOpen() failed.\n";
    return false;
  }
  ar_log_debug() << "arOBJRenderer::readOBJ('" << fileName << "') beginning.\n";
  bool ok = readOBJ(theFile);
  ar_log_debug() << "arOBJRenderer::readOBJ('" << fileName << "') finished.\n";
  ar_fileClose(theFile);
  return ok;
}

bool arOBJRenderer::readOBJ(const string& fileName, const string& path) {
  return readOBJ(fileName, "", path);
}

bool arOBJRenderer::readOBJ(FILE* inputFile) {
  arOBJ theFile;
  theFile._subdirectory = _subdirectory;
  theFile._searchPath = _searchPath;
  ar_log_debug() << "arOBJRenderer::readOBJ() beginning to parse file.\n";
  if (!theFile.readOBJ( inputFile )) {
    ar_log_error() << "arOBJRenderer::readOBJ() failed to parse file.\n";
    return false;
  }
  ar_log_debug() << "arOBJRenderer::readOBJ() done parsing file.\n";
  clear();

  // Copy out the name
  _name = theFile.name();

  // Copy out the vertices.
  _vertices.insert( _vertices.begin(), theFile._vertex.begin(), theFile._vertex.end() );
  ar_log_debug() << "arOBJRenderer::readOBJ() done extracting vertices.\n";

  // Copy out the materials
  const unsigned numMaterials = theFile._material.size();
  _textures.insert( _textures.begin(), numMaterials, NULL );
  _fOpacityMap.insert( _fOpacityMap.begin(), numMaterials, false );
  arTexture* tex;
  for (unsigned matID=0; matID<numMaterials; ++matID) {
    arOBJMaterial& thisMaterial = theFile._material[matID];
    ar_log_debug() << "arOBJRenderer::readOBJ() preparing material #"
                   << matID << " '" << thisMaterial.name << "'" << ar_endl;
    // copypaste -- look for "exporters" elsewhere in this source file
    // NOTE: some exporters will attach a "pure black" color to anything
    // with a texture. Since by default we use GL_MODULATE with textures,
    // this will be a problem. Instead, use (1, 1, 1).
    arVector3 correctedDiffuse = thisMaterial.Kd;
    if (correctedDiffuse[0] < 0.001
        && correctedDiffuse[1] < 0.001
        && correctedDiffuse[2] < 0.001) {
      correctedDiffuse = arVector3(1, 1, 1);
    }

    arMaterial tmp;
    tmp.diffuse = correctedDiffuse;
    tmp.ambient = thisMaterial.Ka;
    tmp.specular = thisMaterial.Ks;
    tmp.emissive = arVector3(0, 0, 0);
    tmp.exponent = thisMaterial.Ns;
    _materials.push_back( tmp );

    ar_log_debug() << "diffuse color map: " << thisMaterial.map_Kd << ar_endl;
    if (thisMaterial.map_Kd != "none") {
      tex = new arTexture();
      if (!tex) {
        ar_log_error() << "arOBJRenderer::readOBJ() failed to allocate arTexture object.\n";
        return false;
      }
      bool alpha = thisMaterial.map_Opacity != "none";
      if (!tex->readImage( thisMaterial.map_Kd, _subdirectory, _searchPath, alpha )) {
        ar_log_error() << "arOBJRenderer::readOBJ() failed to read diffuse map "
                       << thisMaterial.map_Kd << ar_endl;
        return false;
      }
      tex->setTextureFunc( GL_MODULATE );
      _textures[matID] = tex;
      ar_log_remark() << "arOBJRenderer::readOBJ() read diffuse map "
                      << thisMaterial.map_Kd << ar_endl;
    }
    ar_log_debug() << "opacity map: " << thisMaterial.map_Opacity << ar_endl;
    if (thisMaterial.map_Opacity != "none") {
      arTexture* tex = _textures[matID];
      if (!tex) {
        tex = new arTexture();
        if (!tex) {
          ar_log_error() << "arOBJRenderer::readOBJ() failed to allocate arTexture object.\n";
          return false;
        }
      }
      if (!tex->readAlphaImage( thisMaterial.map_Opacity, _subdirectory, _searchPath )) {
        ar_log_error() << "arOBJRenderer::readOBJ() failed to read opacity map "
                       << thisMaterial.map_Opacity << ar_endl;
        return false;
      }
      tex->setTextureFunc( GL_MODULATE );
      _textures[matID] = tex;
      _fOpacityMap[matID] = true;
      ar_log_remark() << "arOBJRenderer::readOBJ() read opacity map "
                      << thisMaterial.map_Opacity << ar_endl;
    }
    arTexture* tex = _textures[matID];
    if (tex) {
      tex->repeating(true);
      tex->mipmap(_mipmapTextures);
    }
    ar_log_debug() << "arOBJRenderer::readOBJ() done preparing material #" << matID << ar_endl;
  }

  // Build the vertex groups
  arOBJGroupRenderer* group;
  for (unsigned i=0; i<theFile._group.size(); ++i) {
    ar_log_debug() << "Preparing group " << i << ar_endl;
    group = new arOBJGroupRenderer();
    if (!group) {
      ar_log_error() << "arOBJGroupRenderer::prepareToDraw() failed to allocate memory.\n";
      return false;
    }
    if (!group->build( this, theFile._groupName[i], theFile._group[i],
         theFile._texCoord, theFile._normal, theFile._triangle )) {
      ar_log_error() << "arOBJGroupRenderer::prepareToDraw() failed to build render group.\n";
      delete group;
      return false;
    }
    _renderGroups.push_back( group );
  }
  return true;
}

arOBJGroupRenderer* arOBJRenderer::getGroup( unsigned i ) {
  return i < _renderGroups.size() ? _renderGroups[i] : NULL;
}

arOBJGroupRenderer* arOBJRenderer::getGroup( const string& name ) {
  vector<arOBJGroupRenderer*>::iterator iter;
  for (iter = _renderGroups.begin(); iter != _renderGroups.end(); ++iter) {
    if ((*iter)->getName() == name) {
      return *iter;
    }
  }
  return NULL;
}

arMaterial* arOBJRenderer::getMaterial( unsigned i ) {
  return i < _materials.size() ? &_materials[i] : NULL;
}

arTexture* arOBJRenderer::getTexture( unsigned i ) {
  return i < _textures.size() ? _textures[i] : NULL;
}

void arOBJRenderer::activateTextures() {
  vector<arTexture*>::iterator titer;
  for (titer = _textures.begin(); titer != _textures.end(); ++titer) {
    if (*titer) {
      (*titer)->activate();
    }
  }
}

void arOBJRenderer::draw() {
  vector<arOBJGroupRenderer*>::iterator iter;
  for (iter = _renderGroups.begin(); iter != _renderGroups.end(); ++iter) {
    arOBJGroupRenderer* ptr = *iter;
    if (ptr) {
      ptr->draw();
    } else {
      ar_log_error() << "arOBJ::draw() ignoring NULL renderer.\n";
    }
  }
}

void arOBJRenderer::clear() {
  vector<arOBJGroupRenderer*>::iterator iter;
  for (iter = _renderGroups.begin(); iter != _renderGroups.end(); ++iter) {
    if (*iter) {
      delete *iter;
    }
  }
  _renderGroups.clear();
  vector<arTexture*>::iterator titer;
  for (titer = _textures.begin(); titer != _textures.end(); ++titer) {
    if (*titer) {
      delete *titer;
    }
  }
  _textures.clear();
  _materials.clear();
  _fOpacityMap.clear();
}


// Move vertices to fit the object fit into a unit sphere.
void arOBJRenderer::normalizeModelSize() {
  arVector3 maxVec(-1.e6, -1.e6, -1.e6);
  arVector3 minVec(1.e6, 1.e6, 1.e6);
  vector<arVector3>::iterator iter;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    arVector3& vert = *iter;
    for (unsigned j=0; j<3; j++) {
      if (vert[j] > maxVec[j]) {
        maxVec[j] = vert[j];
      }
      if (vert[j] < minVec[j]) {
        minVec[j] = vert[j];
      }
    }
  }
  const arVector3 center = (maxVec+minVec)/2.;
  const float maxDist = (maxVec-minVec).magnitude()/2.;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    *iter = (*iter - center)/maxDist;
  }
}


// Transform vertices in-place.
void arOBJRenderer::transformVertices( const arMatrix4& matrix ) {
  vector<arVector3>::iterator iter;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    *iter = matrix * (*iter);
  }
}

// Return a sphere centered at the average of all the triangles in the group,
// with a radius that includes all the points.
arBoundingSphere arOBJRenderer::getBoundingSphere() {
  if (_vertices.size() == 0) {
    ar_log_warning() << "arOBJRenderer::getBoundingSphere() ignoring zero-vertex OBJ.\n";
    return arBoundingSphere();
  }
  // copypaste -- look for radius2 in this source file
  // first, walk through the triangle list and accumulate the average position
  arVector3 center(0, 0, 0);
  vector<arVector3>::iterator iter;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    center += *iter;
  }
  center *= (1./_vertices.size());
  // next, walk through the list a second time, figuring out the maximum
  // distance of a point from the origin
  float radius = 0;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    const float dist = (center-*iter).magnitude();
    if (dist > radius) {
      radius = dist;
    }
  }
  return arBoundingSphere(center, radius);
}

arAxisAlignedBoundingBox arOBJRenderer::getAxisAlignedBoundingBox() {
  arVector3& firstCoord = _vertices[0];
  float xMin = firstCoord[0];
  float xMax = firstCoord[0];
  float yMin = firstCoord[1];
  float yMax = firstCoord[1];
  float zMin = firstCoord[2];
  float zMax = firstCoord[2];
  vector<arVector3>::iterator iter;
  for (iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
    arVector3& vertex = *iter;
    if (vertex[0] < xMin) {
      xMin = vertex[0];
    }
    if (vertex[0] > xMax) {
      xMax = vertex[0];
    }
    if (vertex[1] < yMin) {
      yMin = vertex[1];
    }
    if (vertex[1] > yMax) {
      yMax = vertex[1];
    }
    if (vertex[2] < zMin) {
      zMin = vertex[2];
    }
    if (vertex[2] > zMax) {
      zMax = vertex[2];
    }
  }
  arAxisAlignedBoundingBox box;
  box.center = 0.5*arVector3( xMin+xMax, yMin+yMax, zMin+zMax );
  box.xSize = 0.5*(xMax-xMin);
  box.ySize = 0.5*(yMax-yMin);
  box.zSize = 0.5*(zMax-zMin);
  return box;
}

float arOBJRenderer::getIntersection( const arRay& theRay ) {
  float intersectionDistance = -1;
  const unsigned numberTriangles = _vertices.size()/3;
  vector<arVector3>::const_iterator iter = _vertices.begin();
  for (unsigned i=0; i<numberTriangles; ++i) {
    const arVector3& v1 = *iter++;
    const arVector3& v2 = *iter++;
    const arVector3& v3 = *iter++;
    const float newDist = ar_intersectRayTriangle(theRay, v1, v2, v3 );
    if (intersectionDistance < 0 ||
        (newDist > 0 && newDist < intersectionDistance)) {
      intersectionDistance = newDist;
    }
  }
  return intersectionDistance;
}
