//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arObjectUtilities.h"

/// Animates an OBJ with an HTR
/** This long-winded function puts an HTR transform hierarchy in
 *   theDatabase (attached to theNode), then attaches the groups
 *   in the OBJ to the corresponding segments in the HTR file
 * NOTE: Currently, the OBJ group and HTR segment must have the
 *   same name. A good idea would be an option to do a
 *   "best guess" based on centers of nodes and segments.
 */
/// NOTE: requires the OBJ to have group names that match those of the HTR segments
/// \todo change name to arAttach... and propogate
/// \todo remove the "same name" requirement between OBJ groups and HTR segments
/// @todo be EXTREMELY careful about doing the above todo's as these changes
///        will affect external software built on this foundation
bool attachOBJToHTRToNodeInDatabase(arOBJ* theOBJ, arHTR *theHTR, 
                                    const string &theNode){
  arMatrix4 inverseMatrix;
  string groupName, nodeName;
  int j, TID;

  theOBJ->attachPoints(theNode+"object.points", theNode);
  theHTR->attachMesh(theNode+"object", theNode+"object.points");
  for (int i=0; i<theOBJ->numberOfGroups(); i++){
    groupName = theOBJ->nameOfGroup(i);
    j = theHTR->numberOfSegment(groupName);
    TID = theHTR->inverseIDForSegment(j);
    if (theOBJ->numberInGroup(i)>0 && TID != -1){
      inverseMatrix = theHTR->inverseTransformForSegment(j);
      dgTransform(TID, inverseMatrix);
      nodeName = dgGetNodeName(TID);
      theOBJ->attachGroup(i, theNode+groupName, nodeName);
      int boundingSphereID = theHTR->boundingSphereIDForSegment(j);
      arBoundingSphere theSphere = theOBJ->getGroupBoundingSphere(i);
      dgBoundingSphere(boundingSphereID, 0, theSphere.radius,
		       theSphere.position);
    }
  }

return true;
}

#ifdef AR_USE_WIN_32
// this is a hack, it should really go into szg/language
#define strcasecmp(a,b) _stricmp(a,b)
#endif

/// Reads a file, determines the type, and returns a pointer to a newly-created arObject.
/** This function can only be used if the file ends with the
 *   appropriate (case-insensitive) file format modifier.
 * "path" shouldn't be a default parameter, since the function will fail then.
 */
arObject* arReadObjectFromFile(const char* fileName, const string& path) {
  char* dot = strrchr(fileName, '.');
  if (!dot) {
    cerr << "arObjUtil error: invalid file name \"" << fileName << "\".\n";
    return NULL;
  }
  if (strlen(++dot) <= 3) {
    const string theFileName(fileName);
    FILE* pFile = ar_fileOpen(theFileName, path, "r");
    if (!pFile){
      cerr << "arObjUtil error: failed to open file \""
	   << fileName << "\".\n";
      return NULL;
    }

    // Wavefront OBJ
    if (!strcasecmp(dot,"OBJ")) {
      arOBJ *theOBJ = new arOBJ;
      theOBJ->readOBJ(fileName,path);
      return theOBJ;
    }
    
    // Motion Analysis HTR
    if (!strcasecmp(dot,"HTR")) {
      arHTR* theHTR = new arHTR;
      theHTR->readHTR(pFile);
      return theHTR;
    }
    
    // 3D Studio
    if (!strcasecmp(dot,"3DS")) {
      ar3DS* the3DS = new ar3DS;
      char* temp = new char[strlen(fileName)];
      strcpy(temp,fileName);
      the3DS->read3DS(temp);
      delete temp;
      return the3DS;
    }
    /*
    // VRML97 
    if (!strcasecmp(dot,"wrl")) {
      arVRML *theVRML = new arVRML;
      arVRML->readVRML(pFile);
      return theVRML;
    } */
    /*
    // RenderMan Interface Bytestream
    if (!strcasecmp(dot,"RIB")) {
      arRIB *theRIB = new arRIB;
      theRIB->readRIB(pFile);
      return theRIB;
    }
    */
  }

  cerr << "arObjUtil error: unrecognized filename extension \""
       << dot << "\" in file name \""
       << fileName << "\".\n";
  return NULL;
}


/// Generates local frame for each vertex of object
/** Given a set of vertices, connectivity information, and texture coords,
 *  we can construct binormal and tangent vectors by taking a tangent
 *  along the "u" direction and binormal around the "v" direction of
 *  a surface.
 */
/// \param numVerts Number of vertices
/// \param vertices Array of vertices as 3 packed floats
/// \param normals Normals per vertex as 3 packed floats
/// \param texCoords Texture coordinates per vertex as 2 packed floats
/// @param numFaces How many faces in index list (or zero if vertices et. al.
///                 are in consecutive order)
/// @param index Array of indices into other arrays, every 3 ints representing
///              exactly one triangle, or NULL if in consecutive order
/// \param tangent3 (output) Pointer to array populated with per-vertex tangents
/// \param binormal3 (output) Pointer to array populated with per-vertex binormals
/*bool arGenerateLocalFrame(int numVerts, float* vertices, // input
			  float *normals, float *texCoords,
			  int numFaces, int *indices,
			  float* tangent3, float* binormal3) { // output
  // storage for new values
  arVector3 *duList = new arVector3[numVerts];
  //arVector3 *dvList = new arVector3[numVerts];

  arVector3 edge, tempN, tempT, tempB, duVec, dvVec;
  float du, dv;
  int i, j;
  // assume numFaces = 0 and indices = NULL
  // run through all the faces, finding gradients of u and v along surface
  for (i=0; i<numVerts; i++) { // for every face
    for (j=0; j<3; j++) { // for every vertex
      int nextV = 3*(i+(j+1)%3);
      int nextT = 2*(i+(j+1)%3);
      // vector pointing to next vertex
      edge = arVector3(vertices[nextV+0],vertices[nextV+1],vertices[nextV+2]) - 
	     arVector3(vertices[3*(i+j)+0],vertices[3*(i+j)+1],vertices[3*(i+j)+2]);
      // change in texCoord u value
      du = (texCoords[nextT+0]-texCoords[2*(i+j)+0]);
      //dv = (texCoords[nextT+1]-texCoords[2*(i+j)+1]);
      // gradient of u at this vertex
      duVec = arVector3(edge.x?du/edge.x:du, edge.y?du/edge.y:du, edge.z?du/edge.z:du);
      //dvVec = arVector3(edge.x?dv/edge.x:dv, edge.y?dv/edge.y:dv, edge.z?dv/edge.z:dv);

      // add gradient to both vertices
      duList[i+j] += duVec;
      duList[i+(j+1)%3] += duVec;
      //dvList[i+j] += dvVec;
      //dvList[i+(j+1)%3] += dvVec;
    }
  }

  tangents = new float[numVerts*3];
  binormals = new float[numVerts*3];

  // second pass; take du's and dv's and convert to tan and binorm
  for (i=0; i<numVerts; i++) {
    tempN = arVector3(normals[3*i], normals[3*i+1], normals[3*i+2]);
    tempT = -(tempN % duList[i])*tempN;
    tempT = ++tempT;
    tempB = tempN * tempT; // we take dot prod for rt. angle
    
    tangents[3*i]   = tempT.x;
    tangents[3*i+1] = tempT.y;
    tangents[3*i+2] = tempT.z
    binormals[3*i]   = tempB.x;
    binormals[3*i+1] = tempB.y;
    binormals[3*i+2] = tempB.z
  }

  return true; // no error
}
*/
