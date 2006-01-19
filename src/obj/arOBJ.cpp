//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arOBJ.h"
#include "arGraphicsAPI.h"

arOBJ::arOBJ() :
  _thisMaterial(0),
  _thisSG(0),
  _thisGroup(0),
  _searchPath(string("")) {
}

/// Reads in a .OBJ file, prints out an error if invalid
/// @param inputFile file pointer to read in data
/// \bug calling this twice on the same object has undefined behaviour
bool arOBJ::readOBJ(FILE* inputFile){
  arOBJMaterial tempMaterial;
  _material.push_back(arOBJMaterial());
  sprintf(_material[0].name, "default");
  _material[0].Kd = arVector3(1,1,1);
  _normal.push_back(arVector3(0,0,0));
  _texCoord.push_back(arVector3(0,0,0));
  _smoothingGroup.push_back(arOBJSmoothingGroup());
  _smoothingGroup[0]._name = 0;
  _group.push_back(vector<int>());
  _groupName.push_back("default");

  if (!inputFile){
    cerr << "arOBJ error: NULL input file pointer.";
    _invalidFile = true;
    return false;
  }

  // parse the entire file
  while (_parseOneLine(inputFile))
    ;

  _generateNormals();
  return true;
}

/// wrapper for the 3 parameter readOBJ(...)
bool arOBJ::readOBJ(const char* fileName, const string& path){
  return readOBJ(fileName, "", path);
}

/// opens a File and calls readOBJ(FILE*)
/// @param fileName name of OBJ file (including extension) to read from
/// @param subdirectory is the subdirectory of the search path in which
/// we look for the files, which allows us to store stuff for a program in
/// data_directory/my_program_name instead of just data_directory.
/// @param path a search path upon which we look for the file. this is useful
/// when data files need to be installed in a special directory
bool arOBJ::readOBJ(const char* fileName, 
                    const string& subdirectory, 
                    const string& path){
  if (!fileName){
    cerr << "arOBJ error: NULL filename.\n";
    _invalidFile = true;
    return false;
  }
  _fileName = string(fileName);
  _searchPath = path;
  _subdirectory = subdirectory;
  FILE* theFile = ar_fileOpen(fileName, subdirectory, path, "r");
  if (!theFile){
    cerr << "arOBJ error: failed to open file \"" << fileName << "\".\n";
    _invalidFile = true;
    return false;
  }
  bool state = readOBJ(theFile);
  ar_fileClose(theFile);
  return state;
}

///@param name The name of the group of which you want the ID
///Gets the numerical ID of a geometry group, given the name
int arOBJ::groupID(const string& name){
  for (unsigned int i=0; i<_groupName.size(); i++){
    if (name == _groupName[i])
      return i;
  }
  return -1;
}

///@param transform The new root transformation matrix
/// Sets the global transform matrix for the OBJ
void arOBJ::setTransform(const arMatrix4& transform) {
  _transform = transform;
}

/// @param baseName The name of the entire object
/// @param where the scenegraph node to which we attach the OBJ
/// puts the OBJ into scenegraph w/o hierarchy or transforms
/// (i.e., the "one fell swoop" approach)
void arOBJ::attachMesh(const string& baseName, const string& where){
  if (_invalidFile){
    cerr << "cannot attach mesh: No valid file!\n";
    return;
  }
  const string pointsModifier(".points");

  attachPoints(baseName+pointsModifier, where);
  for (unsigned int i=0; i<_group.size(); i++)
    if (_group[i].size() != 0)
      attachGroup(i, baseName, baseName+pointsModifier);
}

/// @param baseName The name to give the points node
/// @param where Which scenegraph node to attach the points to
/// Just attaches points (vertices) of the OBJ.  No geometry or transforms.
/// Make sure points are above triangles et al in hierarchy.
void arOBJ::attachPoints(const string& baseName, const string& where){
  if (_invalidFile){
    cerr << "cannot attach points: No valid file!\n";
    return;
  }

  const int numberPoints=_vertex.size();
  float* pointPositions = new float[3*numberPoints];
  int* pointIDs = new int[numberPoints];
  for (int i=0; i<numberPoints; i++){
    pointPositions[3*i  ] = _vertex[i][0];
    pointPositions[3*i+1] = _vertex[i][1];
    pointPositions[3*i+2] = _vertex[i][2];
    pointIDs[i] = i;
  }
  _vertexNodeID = // pass the ID to the superclass
  dgPoints(baseName, where,
           numberPoints, pointIDs, pointPositions);
  delete [] pointIDs;
  delete [] pointPositions;
}

/// @param group The OBJ group number to attach
/// @param base The base name of the group
/// @param where Which scenegraph node to attach the group to
/// this attaches geometry, colors, etc. but NOT POINTS to node
/// also can specify base name (recommended: parent.child.child...)
void arOBJ::attachGroup(int group, const string& base, const string& where){
  if (_invalidFile){
    cerr << "cannot attach group: No valid file!\n";
    return;
  }
  const string trianglesModifier(".triangles");
  const string normalsModifier  (".normals");
  const string colorsModifier   (".colors");
  const string texModifier      (".texCoords");
  const string textureModifier  (".texture:");
  const string baseName((base=="" ? "myOBJ." : base+".") + _groupName[group]);
  
  // Attach triangles (faces)
  const int numberTriangles=_group[group].size();

  // Attach colors and textures
  // sort triangles by material
  vector<int>* matIDs = new vector<int>[_material.size()];
  unsigned int j = 0;
  for (j=0; j<(unsigned int)numberTriangles; j++){
    matIDs[_triangle[_group[group][j]].material].push_back(j);
  }
  char temp[32];
  bool useTexture, useBump;
  for (j=0; j<_material.size(); j++){
    if (matIDs[j].size() != 0) {
      if (_material[j].map_Kd == "none")
	useTexture = false;	// there is no texture map
      else
        useTexture = true;
      if (_material[j].map_Bump == "none")
	useBump = false;	// there is no bump map
      else
        useBump = true;
      
      //printf("map_Kd=\"%s\"\n",_material[j].map_Kd);
      float* normals = new float[9*matIDs[j].size()];
      int* indices = new int[3*matIDs[j].size()];
      float* texCoords = new float[6*matIDs[j].size()];
      for (int i=0; i<(int)matIDs[j].size(); i++){
        const arOBJTriangle currentTriangle = _triangle[_group[group][matIDs[j][i]]];
        indices[3*i]   = currentTriangle.vertices[0];
	indices[3*i+1] = currentTriangle.vertices[1];
	indices[3*i+2] = currentTriangle.vertices[2];
        normals[9*i]   = _normal[currentTriangle.normals[0]][0];
	normals[9*i+1] = _normal[currentTriangle.normals[0]][1];
	normals[9*i+2] = _normal[currentTriangle.normals[0]][2];
        normals[9*i+3] = _normal[currentTriangle.normals[1]][0];
	normals[9*i+4] = _normal[currentTriangle.normals[1]][1];
	normals[9*i+5] = _normal[currentTriangle.normals[1]][2];
        normals[9*i+6] = _normal[currentTriangle.normals[2]][0];
	normals[9*i+7] = _normal[currentTriangle.normals[2]][1];
	normals[9*i+8] = _normal[currentTriangle.normals[2]][2];
	if (useTexture){
          texCoords[6*i  ] = 1.-_texCoord[currentTriangle.texCoords[0]][0];
          texCoords[6*i+1] =    _texCoord[currentTriangle.texCoords[0]][1];
          texCoords[6*i+2] = 1.-_texCoord[currentTriangle.texCoords[1]][0];
          texCoords[6*i+3] =    _texCoord[currentTriangle.texCoords[1]][1];
          texCoords[6*i+4] = 1.-_texCoord[currentTriangle.texCoords[2]][0];
          texCoords[6*i+5] =    _texCoord[currentTriangle.texCoords[2]][1];
	}
      }
      sprintf(temp, "%i", j);
      // three colors per triangle
      dgIndex(baseName+".indices"+temp, where, 3*matIDs[j].size(), indices);
      dgNormal3(baseName+normalsModifier+temp, baseName+".indices"+temp,
                3*matIDs[j].size(), normals);
      // NOTE: some exporters will attach a "pure black" color to anything
      // with a texture. Since by default we use GL_MODULATE with textures,
      // this will be a problem. Instead, use (1,1,1).
      arVector3 correctedDiffuse = _material[j].Kd;
      if (correctedDiffuse[0] < 0.001 
	  && correctedDiffuse[1] < 0.001 
	  && correctedDiffuse[2] < 0.001){
	correctedDiffuse = arVector3(1,1,1);
      }
      dgMaterial(baseName+colorsModifier+temp, baseName+normalsModifier+temp,
		 correctedDiffuse, _material[j].Ka, _material[j].Ks,
		 arVector3(0,0,0), _material[j].Ns);
      if (useTexture){
        dgTex2(baseName+texModifier+temp,baseName+colorsModifier+temp,
               3*matIDs[j].size(), texCoords);
        dgTexture(baseName+textureModifier+temp,baseName+texModifier+temp, 
                  _material[j].map_Kd);
	if (useBump){
          dgBumpMap(baseName+".bump"+temp,baseName+textureModifier+temp, 
                    _material[j].map_Bump);
          dgDrawable(baseName+".geometry"+temp, baseName+".bump"+temp,
		     DG_TRIANGLES, matIDs[j].size());
	} else
	  dgDrawable(baseName+".geometry"+temp, baseName+textureModifier+temp,
		     DG_TRIANGLES, matIDs[j].size());
      }
      else{
        dgDrawable(baseName+".geometry"+temp, baseName+colorsModifier+temp,
		   DG_TRIANGLES, matIDs[j].size());
      }


      delete [] texCoords;
      delete [] normals;
      delete [] indices;
    }
  }

  delete [] matIDs;
  //printf("Materials: %i\n", _material.size());
  //printf("Smoothing Groups: %i\n", _smoothingGroup.size());
}

/// @param groupID The OBJ internal group number
/// returns a sphere centered at the average of all the triangles in the group
/// with a radius that includes all the geometry
arBoundingSphere arOBJ::getGroupBoundingSphere(int groupID){
  int i=0,j=0;
  // first, walk through the triangle list and accumulate the average position
  int numberTriangles = _group[groupID].size();
  arVector3 center(0,0,0);
  for (i=0; i<numberTriangles; i++){
    for (j=0; j<3; j++){
      const arVector3& vertexCoord =
        _vertex[_triangle[_group[groupID][i]].vertices[j]];
      center += vertexCoord/(numberTriangles*3);
    }
  }

  // next, walk through the list a second time, figuring out the maximum
  // distance of a point from the origin
  float radius = 0;
  for (i=0; i<numberTriangles; i++){
    for (j=0; j<3; j++){
      const arVector3& vertexCoord =
        _vertex[_triangle[_group[groupID][i]].vertices[j]];
      const float& dist = ++(center-vertexCoord);
      if (dist > radius){
        radius = dist;
      }
    }
  }
  return arBoundingSphere(center, radius);
}

arAxisAlignedBoundingBox arOBJ::getAxisAlignedBoundingBox(int groupID){
  const int numberTriangles = _group[groupID].size();
  float xMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][0];
  float xMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][0];
  float yMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][1];
  float yMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][1];
  float zMin = _vertex[_triangle[_group[groupID][0]].vertices[0]][2];
  float zMax = _vertex[_triangle[_group[groupID][0]].vertices[0]][2];
  for (int i=0; i<numberTriangles;i++){
    for (int j=0; j<3; j++){
      arVector3 vertex = _vertex[_triangle[_group[groupID][i]].vertices[j]];
      if (vertex[0] < xMin){
	xMin = vertex[0];
      }
      if (vertex[0] > xMax){
	xMax = vertex[0];
      }
      if (vertex[1] < yMin){
	yMin = vertex[1];
      }
      if (vertex[1] > yMax){
	yMax = vertex[1];
      }
      if (vertex[2] < zMin){
	zMin = vertex[2];
      }
      if (vertex[2] > zMax){
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

float arOBJ::intersectGroup(int groupID, const arRay& theRay){
  float intersectionDistance = -1;
  int numberTriangles = _group[groupID].size();
  for (int i=0; i<numberTriangles; i++){
    const arVector3& vert1 
      = _vertex[_triangle[_group[groupID][i]].vertices[0]];
    const arVector3& vert2 
      = _vertex[_triangle[_group[groupID][i]].vertices[1]];
    const arVector3& vert3 
      = _vertex[_triangle[_group[groupID][i]].vertices[2]];
    float newDist = ar_intersectRayTriangle(theRay.getOrigin(),
					    theRay.getDirection(),
					    vert1,
					    vert2,
					    vert3);
    if (intersectionDistance < 0 || 
        (newDist > 0 && newDist < intersectionDistance)){
      intersectionDistance = newDist;
    }
  }
  return intersectionDistance;
}

/// Inputs data on "face" line specified in OBJ file
/// @param numTokens how many vertices in this face
/// @param token the actual data on this line, separated nicely
void arOBJ::_parseFace(int numTokens, char *token[]){
  const int howManyVertices = numTokens-1;
  int* vertexID = new int[numTokens-1];
  int* texCoordID = new int[numTokens-1];
  int* normalID = new int[numTokens-1];
  int i=0;
  char *vertexToken[4] = {0};
  for (i=0; i<numTokens-1; i++){
    texCoordID[i] = normalID[i] = 0;
  
    int numVTokens = 0;
    vertexToken[numVTokens++] = strtok(token[i+1], "/");
    while (vertexToken[numVTokens-1]){
      vertexToken[numVTokens++] = strtok(NULL, "/");
    }
    --numVTokens;

    if (numVTokens == 1)
      vertexID[i] = atoi(vertexToken[0]);
    else{
      vertexID[i] = atoi(vertexToken[0]);
      texCoordID[i] = vertexToken[1]?atoi(vertexToken[1]):0;
      normalID[i] = vertexToken[2]?atoi(vertexToken[2]):0;
    }
  }

  // adjust negative indices
  for (i=0; i<howManyVertices; i++){
    if (vertexID[i] < 0)
      vertexID[i] = _vertex.size()+vertexID[i]-1;
    if (texCoordID[i] < 0)
      texCoordID[i] = _texCoord.size()+texCoordID[i]-1;
    if (normalID[i] < 0)
      normalID[i] = _normal.size()+normalID[i]-1;
  }

  // now, handle the geometry....
  if (howManyVertices < 5){
    for (i=0; i<howManyVertices-2; i++){
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
      if (_thisSG != 0){
        _triangle.back().smoothingGroup = _thisSG;
        _smoothingGroup[_thisSG].add(_triangle.size()-1);
      }
      _group[_thisGroup].push_back(_triangle.size()-1);
    }
  }
  else{ // more than 4 vertices
    arVector3 center(0,0,0), normalVec(0,0,0), texCoordVec(0,0,0);
    for (i=1; i<howManyVertices-1; i++){
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
      if (_thisSG != 0){
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

/// Adds normals if there are none, smoothes normals in smoothing group, and
/// adjust backwards-facing normals
void arOBJ::_generateNormals(){
  unsigned int i=0, j=0, k=0;
  _normal.erase(_normal.begin(),_normal.begin()+1);
  _texCoord.erase(_texCoord.begin(), _texCoord.begin()+1);
  for (i=0; i<_triangle.size(); i++)
  { // no normals
    if (_triangle[i].normals[0] == -1 || _triangle[i].normals[1] == -1 ||
        _triangle[i].normals[2] == -1){
      _normal.push_back(
      (_vertex[_triangle[i].vertices[1]] - _vertex[_triangle[i].vertices[0]]) *
      (_vertex[_triangle[i].vertices[2]] - _vertex[_triangle[i].vertices[0]]) );
      _normal.back() /= ++_normal.back();
      _triangle[i].normals[0] = _normal.size()-1;
      _triangle[i].normals[1] = _normal.size()-1;
      _triangle[i].normals[2] = _normal.size()-1;
    }
  }
  // could be that normals are in the opposite direction they should be
  // i.e. relative to vertex orientation of the triangle
  for (i=0; i<_triangle.size(); i++){
    arVector3 naturalDirection = 
      (_vertex[_triangle[i].vertices[1]] - _vertex[_triangle[i].vertices[0]])
      *(_vertex[_triangle[i].vertices[2]] - _vertex[_triangle[i].vertices[0]]);
    if (_normal[_triangle[i].normals[0]]%naturalDirection < 0){
      // reversed
      _normal[_triangle[i].normals[0]] = -_normal[_triangle[i].normals[0]];
    }
    if (_normal[_triangle[i].normals[1]]%naturalDirection < 0){
      // reversed
      _normal[_triangle[i].normals[1]] = -_normal[_triangle[i].normals[1]];
    }
    if (_normal[_triangle[i].normals[2]]%naturalDirection < 0){
      // reversed
      _normal[_triangle[i].normals[2]] = -_normal[_triangle[i].normals[2]];
    }
  }
  // First, a list of vertex-indexed smoothing group triangles
  vector<int>* sgVertex = new vector<int>[_vertex.size()];
  vector<int>* sgVertexNum = new vector<int>[_vertex.size()];
  for (i=0; i<_triangle.size(); i++){
    for (j=0; j<3; j++){
      sgVertex[_triangle[i].vertices[j]].push_back(i);
      sgVertexNum[_triangle[i].vertices[j]].push_back(j);
    }
  }
  // Now go through vertices and add normals in same SG
  arVector3 tempNorm;
  for (i=0; i<_vertex.size(); i++){
    if (sgVertex[i].size() != 0)
    for (j=0; j<sgVertex[i].size()-1; j++){
      if (sgVertex[i][j] != -1 && _triangle[sgVertex[i][j]].smoothingGroup){
        tempNorm = _normal[_triangle[sgVertex[i][j]].normals[sgVertexNum[i][j]]];
	for (k=j+1; k<sgVertex[i].size(); k++){
 	  if (sgVertex[i][k] != -1 && _triangle[sgVertex[i][j]].smoothingGroup ==
              _triangle[sgVertex[i][k]].smoothingGroup){
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

/// Make the object fit in a sphere of radius 1
/** This actually adjust the vertex positions,
 *  not just add a normalization matrix
 */
void arOBJ::normalizeModelSize(){
  arVector3 maxVec(-10000,-10000,-10000);
  arVector3 minVec(10000,10000,10000);
  unsigned int i = 0;
  for (i=0; i<_vertex.size(); i++) {
    for (unsigned int j=0; j<3; j++) {
      if (_vertex[i].v[j] > maxVec.v[j])
        maxVec.v[j] = _vertex[i].v[j];
      if (_vertex[i].v[j] < minVec.v[j])
        minVec.v[j] = _vertex[i].v[j];
    }
  }
  const arVector3 center = (maxVec+minVec)/2.;
  const float maxDist = sqrt((maxVec-minVec)%(maxVec-minVec))/2.;
  for (i=0; i<_vertex.size(); i++)
    _vertex[i] = (_vertex[i] - center) / maxDist;
}
