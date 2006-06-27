//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arObjectUtilities.h"
#include "arLogStream.h"

// DEPRECATED. THIS IS MERELY PRESENT TO PROVIDE BACKWARDS COMPATIBILITY!
bool ar_mergeOBJandHTR(arOBJ* theOBJ, arHTR* theHTR, const string& where){
  arGraphicsNode* n = dgGetNode(where);
  return n ? ar_mergeOBJandHTR(n, theOBJ, theHTR, "") : false;
}

/**  Attaches an HTR transform hierarchy to parent, then attaches the groups
 *   in the OBJ to the corresponding segments in the HTR.
 * NOTE: Currently, the OBJ group and HTR segment must have the
 *   same name. A good idea would be an option to do a
 *   "best guess" based on centers of nodes and segments.
 */
bool ar_mergeOBJandHTR(arGraphicsNode* parent, arOBJ* theOBJ, arHTR* theHTR, const string& objectName){
  string name = (objectName == "") ? "object" : objectName;
  arGraphicsNode* n = theOBJ->attachPoints(parent, name+".points");
  theHTR->attachMesh(n, name);
  for (int i=0; i<theOBJ->numberOfGroups(); i++){
    const string groupName(theOBJ->nameOfGroup(i));
    const int j = theHTR->numberOfSegment(groupName);
    arTransformNode* inverseTransformNode = theHTR->inverseForSegment(j);
    if (theOBJ->numberInGroup(i)>0 && inverseTransformNode){
      inverseTransformNode->setTransform(theHTR->inverseTransformForSegment(j));
      theOBJ->attachGroup(inverseTransformNode, i, name+"."+groupName);
      arBoundingSphereNode* b = theHTR->boundingSphereForSegment(j);
      arBoundingSphere sphere = theOBJ->getGroupBoundingSphere(i);
      sphere.visibility = false;
      b->setBoundingSphere(sphere);
    }
  }
  return true;
}

#ifdef AR_USE_WIN_32
// this is a hack, it should really go into szg/language
#define strcasecmp(a,b) _stricmp(a,b)
#endif

// Reads a file, determines the type, and returns a pointer to a newly-created arObject.
/** This function can only be used if the file ends with the
 *  appropriate (case-insensitive) file format modifier.
 *  "path" shouldn't be a default parameter, since the function will fail then.
 */
arObject* ar_readObjectFromFile(const string& fileName, const string& path) {
  unsigned int pos = fileName.find('.');
  if (pos == string::npos) {
    ar_log_error() << "arObjUtil invalid file name \"" << fileName << "\".\n";
    return NULL;
  }
  string fullName = ar_fileFind(fileName, "", path);
  if (fullName == "NULL"){
    ar_log_error() << "arObjUtil failed to find file \""
	           << fileName << "\".\n";
  }
  string suffix = fileName.substr(pos, fileName.length()-pos);

  // Wavefront OBJ
  if (suffix == ".obj" || suffix == ".OBJ") {
    arOBJ *theOBJ = new arOBJ;
    theOBJ->readOBJ(fullName);
    return theOBJ;
  }
  // Motion Analysis HTR
  if (suffix == ".htr" || suffix == ".HTR" || suffix == ".htr2" || suffix == ".HTR2") {
    arHTR* theHTR = new arHTR;
    theHTR->readHTR(fullName);
    return theHTR;
  }
    
  // 3D Studio
  if (suffix == ".3ds" || suffix == ".3DS") {
    ar3DS* the3DS = new ar3DS;
    the3DS->read3DS(fileName);
    return the3DS;
  }

  ar_log_error() << "arObjUtil unrecognized filename extension \""
                 << suffix << "\" in file name \""
                 << fileName << "\".\n";
  return NULL;
}


// Generates local frame for each vertex of object
/** Given a set of vertices, connectivity information, and texture coords,
 *  we can construct binormal and tangent vectors by taking a tangent
 *  along the "u" direction and binormal around the "v" direction of
 *  a surface.
 */
// \param numVerts Number of vertices
// \param vertices Array of vertices as 3 packed floats
// \param normals Normals per vertex as 3 packed floats
// \param texCoords Texture coordinates per vertex as 2 packed floats
// @param numFaces How many faces in index list (or zero if vertices et. al.
//                 are in consecutive order)
// @param index Array of indices into other arrays, every 3 ints representing
//              exactly one triangle, or NULL if in consecutive order
// \param tangent3 (output) Pointer to array populated with per-vertex tangents
// \param binormal3 (output) Pointer to array populated with per-vertex binormals
/*bool arGenerateLocalFrame(int numVerts, float* vertices, // input
			  float *normals, float *texCoords,
			  int numFaces, int *indices,
			  float* tangent3, float* binormal3) { // output
  // storage for new values
  arVector3 *duList = new arVector3[numVerts];
  //arVector3 *dvList = new arVector3[numVerts];

  int i=0;
  // assume numFaces = 0 and indices = NULL
  // run through all the faces, finding gradients of u and v along surface
  for (i=0; i<numVerts; i++) { // for every face
    for (int j=0; j<3; j++) { // for every vertex
      const int nextV = 3*(i+(j+1)%3);
      const int nextT = 2*(i+(j+1)%3);
      // vector pointing to next vertex
      const arVector3 edge = arVector3(vertices[nextV+0],vertices[nextV+1],vertices[nextV+2]) - 
	     arVector3(vertices[3*(i+j)+0],vertices[3*(i+j)+1],vertices[3*(i+j)+2]);
      // change in texCoord u value
      const float du = (texCoords[nextT+0]-texCoords[2*(i+j)+0]);
      //const float dv = (texCoords[nextT+1]-texCoords[2*(i+j)+1]);
      // gradient of u at this vertex
      const arVector3 duVec = arVector3(edge.x?du/edge.x:du, edge.y?du/edge.y:du, edge.z?du/edge.z:du);
      //const arVector3 dvVec = arVector3(edge.x?dv/edge.x:dv, edge.y?dv/edge.y:dv, edge.z?dv/edge.z:dv);

      // add gradient to both vertices
      duList[i+j] += duVec;
      duList[i+(j+1)%3] += duVec;
      //dvList[i+j] += dvVec;
      //dvList[i+(j+1)%3] += dvVec;
    }
  }

  // second pass: convert du's and dv's to tan and binorm
  tangents = new float[numVerts*3];
  binormals = new float[numVerts*3];
  for (i=0; i<numVerts; i++) {
    const arvector3 tempN(normals[3*i], normals[3*i+1], normals[3*i+2]);
    const arvector3 tempT = ++(-(tempN % duList[i])*tempN);
    const arvector3 tempB = tempN * tempT; // dot prod for right angle
    tangents[3*i]   = tempT.x;
    tangents[3*i+1] = tempT.y;
    tangents[3*i+2] = tempT.z
    binormals[3*i]   = tempB.x;
    binormals[3*i+1] = tempB.y;
    binormals[3*i+2] = tempB.z
  }

  delete [] duList;
  // delete [] dvList;
  return true;
}
*/
