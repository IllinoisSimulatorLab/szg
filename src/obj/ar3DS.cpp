//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "ar3DS.h"
#include "arGraphicsAPI.h"

ar3DS::ar3DS() :
  _normalize(false),
  _file(NULL),
  _uniqueName(0){
}

ar3DS::~ar3DS() {
#ifdef Enable3DS
  if (_file)
    delete (_file);
#endif
}

/// Uses lib3ds to read in a .3ds file
/// @param fileName name of the 3DS file (including extension)
bool ar3DS::read3DS(char *fileName) {
#ifdef Enable3DS
  _file = lib3ds_file_load(fileName);
  if (!_file)
    _invalidFile = true;
#else
    cerr << "ar3DS error: failed to read \""
	 << fileName
	 << "\", since compiled without 3DS support.\n";
#endif
  return !_invalidFile;
}

/// Attaches mesh geometry to scenegraph
/// \param baseName The name of the object
/// \param where The name of the node to attach to in scenegraph
void ar3DS::attachMesh(const string& baseName, const string& where){
  arGraphicsNode* parent = dgGetNode(where);
  if (parent){
    attachMesh(baseName, parent);
  }
}

/// Attaches geometry to the scene graph.
/// @param baseName: a template name for the node names of the object
/// @parant parent: the node to which we will attach the object.
void ar3DS::attachMesh(const string& baseName, arGraphicsNode* parent){
#ifndef Enable3DS
  cerr << "ar3DS error: compiled without 3DS support.\n";
#else
  if (_invalidFile){
    cerr<<"cannot attach mesh: No valid file!\n";
    return;
  }
  _numMaterials = 0;
  for (Lib3dsMaterial* p=_file->materials; p!=0; p=p->next){
    ++_numMaterials;
  }
  // make at least two material containers
  _numMaterials = _numMaterials>2?_numMaterials:2;

  arMatrix4 topMatrix;
  if (_normalize)
    normalizationMatrix(topMatrix);

  // NOTE HOW KLUGEY THIS CODE SEEMS. IT WOULD BE BETTER TO HAVE THE
  // NODES BE IN CHARGE OF THE NEW NODE CREATION.
  arDatabase* database = parent->getOwningDatabase();
  arTransformNode* transformNode 
    = (arTransformNode*) database->newNode(parent, "transform", baseName);
  transformNode->setTransform(topMatrix);
  //dgTransform(baseName, where, topMatrix);
  Lib3dsNode* ptr = _file->nodes;
  if (ptr == NULL)
    cerr << "ar3DS error: File contains no data!\n"
	 << "(are you using a supported version of 3D Studio Max "
	 << "to save data?)\n";
  else
    for (ptr=_file->nodes; ptr!=NULL; ptr=ptr->next) {
      attachChildNode(baseName, transformNode, ptr);
    }
#endif
}

#ifdef Enable3DS
/// Attaches a lib3ds node
/** Called by attachMesh(); used for easy recursion
 *  @param baseName Prefix for this node
 *  @param node The lib3ds node to attach
 */
void ar3DS::attachChildNode(const string &baseName, 
                            arGraphicsNode* parent,
                            Lib3dsNode* node){
  // 3DS nodes can have the same name. Consequently, we need to go ahead
  // and add a uniqueness number
  _uniqueName++;
  char uniquenessBuffer[128];
  sprintf(uniquenessBuffer,":%i",_uniqueName);
  const string newName(baseName + "." + string(node->name)+uniquenessBuffer);
  // POTENTIAL BUG!!! Note that lib3DS does not use a matrix stack....
  // so that the matrix value at a node is, indeedm the absolute value,
  // not the value relative to the branch on which it resides.
  // Of course, the syzygy scene graph does things differently
  arMatrix4 nodeTransform =
    arMatrix4(node->matrix[0][0],node->matrix[0][1],
	      node->matrix[0][2],node->matrix[0][3],
              node->matrix[1][0],node->matrix[1][1],
	      node->matrix[1][2],node->matrix[1][3],
              node->matrix[2][0],node->matrix[2][1],
	      node->matrix[2][2],node->matrix[2][3],
              node->matrix[3][0],node->matrix[3][1],
	      node->matrix[3][2],node->matrix[3][3]);
  //dgTransform(newName, baseName, nodeTransform);
  arDatabase* database = parent->getOwningDatabase();
  arTransformNode* newParent 
    = (arTransformNode*) database->newNode(parent, "transform", newName);
  newParent->setTransform(nodeTransform);
  
  // recurse through children  
  Lib3dsNode *p;
  for (p=node->childs; p!=NULL; p=p->next){
    attachChildNode(newName, newParent, p);
  }

  if (node->type!=LIB3DS_OBJECT_NODE || !strcmp(node->name,"$$$DUMMY"))
    return;

  Lib3dsMesh *mesh = lib3ds_file_mesh_by_name(_file, node->name);
  ASSERT(mesh);
  if (!mesh) {
    return;
  }
 
  const string transformModifier(".transform");
  const string pointsModifier   (".points");
  const string indexModifier    (".index.");
  const string normalsModifier  (".normals");
  const string materialsModifier(".materials.");
  const string geometryModifier (".geometry.");

  unsigned int j, i;
  vector<Lib3dsMaterial*> materials; 	///< pointers to actual materials
  ///< faces with indexed material
  // once again, the default material causes us to want to have plus one here
  vector<int>*	materialFaces = new vector<int>[_numMaterials+1]; 	
  ///< names of the materials
  // don't forget that we have the default material, hence the plus one
  string*	materialNames = new string[_numMaterials+1]; 	
  Lib3dsMatrix	invMeshMatrix;		///< matrix for the mesh from Lib3DS
  lib3ds_matrix_copy(invMeshMatrix, mesh->matrix);
  lib3ds_matrix_inv(invMeshMatrix);

  /// Put a default material on the front of the material stack
  materials.push_back(new Lib3dsMaterial);
  materials.back()->ambient[0] = 0.2; materials.back()->ambient[1] = 0.2;
  materials.back()->ambient[2] = 0.2; materials.back()->ambient[3] = 1.0;
  materials.back()->diffuse[0] = 0.8; materials.back()->diffuse[1] = 0.8;
  materials.back()->diffuse[2] = 0.8; materials.back()->diffuse[3] = 1.0;
  materials.back()->specular[0] = 0.0; materials.back()->specular[1] = 0.0;
  materials.back()->specular[2] = 0.0; materials.back()->specular[3] = 1.0;
  materials.back()->shininess = 0.0;
  materialNames[0] = "(default)";	// no name on default material
  
  /// Calculate normals
  Lib3dsVector *normalL = new Lib3dsVector[3*mesh->faces];
  lib3ds_mesh_calculate_normals(mesh, normalL);
  float *norms = new float[mesh->faces*9];
  float *points = new float[mesh->faces*9];
  for (i=0; i<mesh->faces*9; i++)
    points[i] = norms[i] = -1.;

  Lib3dsVector vPoint;
  for (j=0; j<mesh->faces; ++j) {
    Lib3dsFace &f=mesh->faceL[j];
    /// Handle material
    if (f.material[0]) {
      // see if the material exists already
      for (i=1; i<materials.size(); i++) {
      	if (materialNames[i] == string(f.material)) {
	  materialFaces[i].push_back(j);
          break;
	}
      }
      	// if no matching material found, create a new one
      if (i == materials.size()) {  
        materials.push_back(lib3ds_file_material_by_name(_file, f.material));
	materialNames[i] = string(f.material);
        materialFaces[materials.size()-1].push_back(j);
      }
    }
    else	// no material specified, so add it to the default list
      materialFaces[0].push_back(j);
     
    /// Add the point and normals to respective temp. storage 
    for (i=0; i<3; ++i) {
      lib3ds_vector_transform(vPoint, invMeshMatrix, 
                              mesh->pointL[f.points[i]].pos);
      points[9*j+3*i+0] = vPoint[0];
      points[9*j+3*i+1] = vPoint[1];
      points[9*j+3*i+2] = vPoint[2];
      norms[9*j+3*i+0] = normalL[3*j+i][0];
      norms[9*j+3*i+1] = normalL[3*j+i][1];
      norms[9*j+3*i+2] = normalL[3*j+i][2];
    }
  }
  delete [] normalL;
  
  /// Point Positions
  //_vertexNodeID =
  // dgPoints(newName + pointsModifier,
  //         newName,
  //	   mesh->faces*3, points);
  arPointsNode* pointsNode 
    = (arPointsNode*)database->newNode(newParent, "points", 
                                       newName + pointsModifier);
  pointsNode->setPoints(mesh->faces*3, points);
  // pass the node ID to superclass
  _vertexNodeID = pointsNode->getID();
  delete [] points;
  
  /// (per-face) Normals into array
  //dgNormal3(newName + normalsModifier,
  //          newName + pointsModifier,
  //          mesh->faces*3, norms);
  arNormal3Node* normalNode 
    = (arNormal3Node*) database->newNode(pointsNode, "normal3",
					 newName + normalsModifier);
  normalNode->setNormal3(mesh->faces*3, norms);
  delete [] norms;

#if 0
      dgDrawable(newName + geometryModifier,
                 newName + normalsModifier,
                 DG_TRIANGLES, mesh->faces/3);
#endif
#if 1
  /// Put all triangles with same material into one group
  ///   and attach to single material
  arVector3 diffuse, specular, ambient, emmissive(0,0,0);
  int *drawIndices = NULL;
  for (i=0; i<materials.size(); i++) {
    if (materialFaces[i].size() > 0) {
      diffuse  = arVector3(materials[i]->diffuse);
      ambient  = arVector3(materials[i]->ambient);
      specular = arVector3(materials[i]->specular);

      drawIndices = new int[materialFaces[i].size()*3];
      for (j=0; j<materialFaces[i].size(); j++) {
        drawIndices[3*j+0] = materialFaces[i][j]*3+0;
        drawIndices[3*j+1] = materialFaces[i][j]*3+1;
        drawIndices[3*j+2] = materialFaces[i][j]*3+2;
      }

      //dgIndex(newName + indexModifier + materialNames[i],
      //	      newName + normalsModifier,
      //	      materialFaces[i].size()*3, drawIndices);
      arIndexNode* indexNode 
        = (arIndexNode*) database->newNode(normalNode, "index",
			   newName+indexModifier+materialNames[i]);
      indexNode->setIndices(materialFaces[i].size()*3, drawIndices);
      //dgMaterial(newName + materialsModifier + materialNames[i],
      //           newName + indexModifier     + materialNames[i],
      //           diffuse, ambient, specular, emmissive,
      //	         11.-0.1*materials[i]->shininess);
      arMaterial material;
      material.diffuse = diffuse;
      material.ambient = ambient;
      material.specular = specular;
      material.emissive = emmissive;
      material.exponent = 11.-0.1*materials[i]->shininess;
      arMaterialNode* materialNode
	= (arMaterialNode*) database->newNode(indexNode, "material", 
			      newName + materialsModifier + materialNames[i]);
      materialNode->setMaterial(material);
      //dgDrawable(newName + geometryModifier  + materialNames[i],
      //           newName + materialsModifier + materialNames[i],
      //           DG_TRIANGLES, materialFaces[i].size());
      arDrawableNode* drawableNode 
	= (arDrawableNode*) database->newNode(materialNode, "drawable",
			      newName + geometryModifier + materialNames[i]);
      drawableNode->setDrawable(DG_TRIANGLES, materialFaces[i].size());
      delete [] drawIndices;
    }
  }
#endif
  // delete some of the remaining dynamic storage
  delete [] materialFaces;
  delete [] materialNames;
}
#endif

/// Is used to put a 3DS object inside a sphere of radius 1
/// @param theMatrix the matrix to be returned (by reference)
bool ar3DS::normalizationMatrix(arMatrix4 &theMatrix) {
#ifndef Enable3DS
  cerr << "ar3DS error: compiled without 3DS support.\n";
#else
  if (_invalidFile) {
    cerr << "Cannot normalize model size: invalid file!\n";
    return false;
  }
  Lib3dsNode *p;
  _minVec = arVector3( 10000, 10000, 10000);
  _maxVec = arVector3(-10000,-10000,-10000);

  // collect data (min/max)
  for (p=_file->nodes; p!=NULL; p=p->next)
    subNormalizationMatrix(p, _minVec, _maxVec);

//cout<<_minVec<<", "<<_maxVec<<endl;

  arVector3 _center = (_maxVec+_minVec)/2.;
  float maxDist = 2./sqrt((_maxVec-_minVec)%(_maxVec-_minVec));

//cout<<_center<<endl;

  theMatrix = //ar_translationMatrix(-_center)*
              ar_scaleMatrix(maxDist) * ar_translationMatrix(-_center);

#endif
  return true;
}

#ifdef Enable3DS
/// helper function for normalizationMatrix() recursion
/// @param node Lib3dsNode to start from
/// @param _minVec smallest (x,y,z) position of all points in entire file
/// @param _maxVec largest (x,y,z) position of all points in entire file
void ar3DS::subNormalizationMatrix(Lib3dsNode* node,
                                   arVector3 &_minVec, arVector3 &_maxVec) {
  Lib3dsNode *p;
  // recurse through children
  for (p=node->childs; p!=NULL; p=p->next)
    subNormalizationMatrix(p, _minVec, _maxVec);

  if (node->type==LIB3DS_OBJECT_NODE) {
    if (!strcmp(node->name,"$$$DUMMY"))
    return;

    Lib3dsMesh *mesh=lib3ds_file_mesh_by_name(_file, node->name);
    ASSERT(mesh);
    if (!mesh)
      return;

    unsigned int k, j, i;
    Lib3dsVector v;
    Lib3dsMatrix invMeshMatrix;

    lib3ds_matrix_copy(invMeshMatrix, mesh->matrix);
    lib3ds_matrix_inv(invMeshMatrix);

    for (j=0; j<mesh->faces; ++j) {
      Lib3dsFace &f=mesh->faceL[j];
      for (i=0; i<3; ++i) {
        lib3ds_vector_transform(v, invMeshMatrix, mesh->pointL[f.points[i]].pos);

        for (k=0; k<3; k++) {
          if (v[k] > _maxVec[k])
            _maxVec[k] = v[k];
          if (v[k] < _minVec[k])
            _minVec[k] = v[k];
        }
      }
    } // end face loop
  }

}
#endif

/// Puts object inside a sphere of radius 1
/** Sets a flag for attachMesh() to use a normalized matrix as the root
 *  node in the scenegraph representation of the mesh
 */
void ar3DS::normalizeModelSize() {
  _normalize = true;
}



///// Animation functions /////

/// Sets the current frame of animation
/// @param newFrame The frame to jump to in the animation
/// \todo check bounds on newFrame
bool ar3DS::setFrame(int newFrame){
/*  if (newFrame >= numFrames || newFrame < 0) return false;

  for(unsigned int i=0; i<segmentData.size(); i++){
    dgTransform(segmentData[i]->transformID, segmentData[i]->frame[newFrame]->trans);
  }
*/
  _currentFrame = newFrame;
  return true;
  
}

/// Goes to next frame in animation; returns false if already at the end
bool ar3DS::nextFrame(){
  return setFrame(_currentFrame+1);
}

/// Goes to previous frame in animation; returns false if already at the beginning
bool ar3DS::prevFrame(){
  return setFrame(_currentFrame-1);
}
