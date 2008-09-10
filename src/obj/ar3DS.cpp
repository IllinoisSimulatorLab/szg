//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "ar3DS.h"
#include "arGraphicsAPI.h"

ar3DS::ar3DS() :
  _normalize(false),
  _file(NULL),
  _uniqueName(0) {
}

ar3DS::~ar3DS() {
#ifdef Enable3DS
  if (_file)
    delete (_file);
#endif
}

// Uses lib3ds to read in a .3ds file
// @param fileName name of the 3DS file (including extension)
bool ar3DS::read3DS(const string& fileName) {
#ifndef Enable3DS
  cerr << "ar3DS error: compiled without 3DS support.\n";
  return false;
  (void)fileName; // avoid compiler warning
#else
  char* name = new char[fileName.length()+1];
  strcpy(name, fileName.c_str());
  _file = lib3ds_file_load(name);
  if (!_file) {
    _invalidFile = true;
  }
  delete [] name;
#endif
  return !_invalidFile;
}

// Attaches mesh geometry to scenegraph
// \param baseName The name of the object
// \param where The name of the node to attach to in scenegraph
bool ar3DS::attachMesh(const string& baseName, const string& where) {
  arGraphicsNode* parent = dgGetNode(where);
  if (parent) {
    return attachMesh(parent, baseName);
  }
  return false;
}

// Attaches geometry to the scene graph.
// @param baseName: a template name for the node names of the object
// @parant parent: the node to which we will attach the object.
bool ar3DS::attachMesh(arGraphicsNode* parent, const string& baseName) {
#ifndef Enable3DS
  cerr << "ar3DS error: compiled without 3DS support.\n";
  (void)parent; // avoid compiler warning
  (void)baseName; // avoid compiler warning
  return false;
#else
  if (_invalidFile) {
    cerr<<"cannot attach mesh: No valid file!\n";
    return false;
  }
  _numMaterials = 0;
  for (Lib3dsMaterial* p=_file->materials; p!=0; p=p->next) {
    ++_numMaterials;
  }
  // make at least two material containers
  _numMaterials = (_numMaterials>2)?(_numMaterials):(2);

  arMatrix4 topMatrix;
  if (_normalize) {
    normalizationMatrix(topMatrix);
  }

  // NOTE HOW KLUGEY THIS CODE SEEMS. IT WOULD BE BETTER TO HAVE THE
  // NODES BE IN CHARGE OF THE NEW NODE CREATION.
  arDatabase* database = parent->getOwner();
  arTransformNode* transformNode =
    (arTransformNode*) database->newNode(parent, "transform", baseName);
  transformNode->setTransform(topMatrix);
  //dgTransform(baseName, where, topMatrix);
  Lib3dsNode* ptr = _file->nodes;
  if (ptr) {
    for (ptr=_file->nodes; ptr; ptr=ptr->next) {
      attachChildNode(baseName, transformNode, ptr);
    }
  } else {
    cerr << "ar3DS error: empty file. (Was the data saved with a supported version of 3D Studio Max?)\n";
  }

  return true;
#endif
}

#ifdef Enable3DS
// Attaches a lib3ds node
/** Called by attachMesh(); used for easy recursion
 *  @param baseName Prefix for this node
 *  @param node The lib3ds node to attach
 */
void ar3DS::attachChildNode(const string &baseName,
                            arGraphicsNode* parent,
                            Lib3dsNode* node) {
  // Disambiguate 3DS nodes which share names.
  const string newName(
    baseName + "." + node->name + ":" + ar_intToString(++_uniqueName));

  // Bug: lib3DS does not use a matrix stack,
  // so that the matrix value at a node is the absolute value,
  // not the value relative to the branch on which it resides.
  // Syzygy's scene graph does things differently.
  const arMatrix4 nodeTransform(
    node->matrix[0][0], node->matrix[0][1],
    node->matrix[0][2], node->matrix[0][3],
    node->matrix[1][0], node->matrix[1][1],
    node->matrix[1][2], node->matrix[1][3],
    node->matrix[2][0], node->matrix[2][1],
    node->matrix[2][2], node->matrix[2][3],
    node->matrix[3][0], node->matrix[3][1],
    node->matrix[3][2], node->matrix[3][3]);
  //dgTransform(newName, baseName, nodeTransform);
  arDatabase* database = parent->getOwner();
  arTransformNode* newParent =
    (arTransformNode*) database->newNode(parent, "transform", newName);
  newParent->setTransform(nodeTransform);

  // Recurse on children.
  for (Lib3dsNode* p=node->childs; p; p=p->next) {
    attachChildNode(newName, newParent, p);
  }

  if (node->type!=LIB3DS_OBJECT_NODE || !strcmp(node->name, "$$$DUMMY"))
    return;

  Lib3dsMesh *mesh = lib3ds_file_mesh_by_name(_file, node->name);
  ASSERT(mesh);
  if (!mesh)
    return;

  const string transformModifier(".transform");
  const string pointsModifier   (".points");
  const string indexModifier    (".index.");
  const string normalsModifier  (".normals");
  const string materialsModifier(".materials.");
  const string geometryModifier (".geometry.");

  unsigned int j=0, i=0;
  // Faces with indexed material.  +1 is for the default material.
  vector<int>*        materialFaces = new vector<int>[_numMaterials+1];
  // names of the materials
  Lib3dsMatrix invMeshMatrix;                // matrix for the mesh from Lib3DS
  lib3ds_matrix_copy(invMeshMatrix, mesh->matrix);
  lib3ds_matrix_inv(invMeshMatrix);

  // Put a default material on the front of the material stack
  vector<Lib3dsMaterial*> materials(1, new Lib3dsMaterial);         // pointers to actual materials
  materials.back()->ambient[0] = 0.2; materials.back()->ambient[1] = 0.2;
  materials.back()->ambient[2] = 0.2; materials.back()->ambient[3] = 1.0;
  materials.back()->diffuse[0] = 0.8; materials.back()->diffuse[1] = 0.8;
  materials.back()->diffuse[2] = 0.8; materials.back()->diffuse[3] = 1.0;
  materials.back()->specular[0] = 0.0; materials.back()->specular[1] = 0.0;
  materials.back()->specular[2] = 0.0; materials.back()->specular[3] = 1.0;
  materials.back()->shininess = 0.0;
  string* materialNames = new string[_numMaterials+1];
  materialNames[0] = "(default)";        // no name on default material

  // Calculate normals
  Lib3dsVector *normalL = new Lib3dsVector[3*mesh->faces];
  lib3ds_mesh_calculate_normals(mesh, normalL);
  float *norms = new float[mesh->faces*9];
  float *points = new float[mesh->faces*9];
  for (i=0; i<mesh->faces*9; i++)
    points[i] = norms[i] = -1.;

  Lib3dsVector vPoint;
  for (j=0; j<mesh->faces; ++j) {
    Lib3dsFace &f=mesh->faceL[j];
    // Handle material
    if (f.material[0]) {
      // see if the material exists already
      for (i=1; i<materials.size(); ++i) {
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
    else        // no material specified, so add it to the default list
      materialFaces[0].push_back(j);

    // Add the point and normals to respective temp. storage
    for (i=0; i<3; ++i) {
      lib3ds_vector_transform( vPoint, invMeshMatrix, mesh->pointL[f.points[i]].pos);
      memcpy(points + 9*j+3*i, &vPoint[0], 3*sizeof(float));
      memcpy(norms  + 9*j+3*i, &normalL[3*j+i][0], 3*sizeof(float));
    }
  }
  delete [] normalL;

  // Point Positions
  //_vertexNodeID =
  // dgPoints(newName + pointsModifier,
  //         newName,
  //           mesh->faces*3, points);
  arPointsNode* pointsNode = (arPointsNode*)
    database->newNode(newParent, "points", newName + pointsModifier);
  pointsNode->setPoints(mesh->faces*3, points);
  // pass the node ID to superclass
  _vertexNodeID = pointsNode->getID();
  delete [] points;

  // (per-face) Normals into array
  //dgNormal3(newName + normalsModifier,
  //          newName + pointsModifier,
  //          mesh->faces*3, norms);
  arNormal3Node* normalNode = (arNormal3Node*)
    database->newNode(pointsNode, "normal3", newName + normalsModifier);
  normalNode->setNormal3(mesh->faces*3, norms);
  delete [] norms;

#if 0
      dgDrawable(newName + geometryModifier,
                 newName + normalsModifier,
                 DG_TRIANGLES, mesh->faces/3);
#endif
#if 1
  // Put all triangles with same material into one group
  //   and attach to single material
  const arVector3 emmissive(0, 0, 0);
  for (i=0; i<materials.size(); i++) {
    if (materialFaces[i].empty())
      continue;

    const arVector3 diffuse (materials[i]->diffuse);
    const arVector3 ambient (materials[i]->ambient);
    const arVector3 specular(materials[i]->specular);
    int* drawIndices = new int[materialFaces[i].size()*3];
    for (j=0; j<materialFaces[i].size(); j++) {
      drawIndices[3*j+0] = materialFaces[i][j]*3+0;
      drawIndices[3*j+1] = materialFaces[i][j]*3+1;
      drawIndices[3*j+2] = materialFaces[i][j]*3+2;
    }

    //dgIndex(newName + indexModifier + materialNames[i],
    //              newName + normalsModifier,
    //              materialFaces[i].size()*3, drawIndices);
    arIndexNode* indexNode
      = (arIndexNode*) database->newNode(normalNode, "index",
                         newName+indexModifier+materialNames[i]);
    indexNode->setIndices(materialFaces[i].size()*3, drawIndices);
    //dgMaterial(newName + materialsModifier + materialNames[i],
    //           newName + indexModifier     + materialNames[i],
    //           diffuse, ambient, specular, emmissive,
    //                 11.-0.1*materials[i]->shininess);
    // todo: constructor for next 5 lines
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
#endif
  delete [] materialFaces;
  delete [] materialNames;
}
#endif

// Put a 3DS object inside a unit sphere.
// @param theMatrix the matrix to be returned (by reference)
bool ar3DS::normalizationMatrix(arMatrix4 &m) {
#ifndef Enable3DS
  cerr << "ar3DS error: compiled without 3DS support.\n";
  (void)m; // avoid compiler warning
#else
  if (_invalidFile) {
    cerr << "Cannot normalize model size: invalid file!\n";
    return false;
  }

  _minVec = arVector3( 10000,  10000,  10000);
  _maxVec = arVector3(-10000, -10000, -10000);

  // collect min/max data
  for (Lib3dsNode* p=_file->nodes; p!=NULL; p=p->next)
    subNormalizationMatrix(p, _minVec, _maxVec);

  const arVector3 _center = (_maxVec+_minVec)/2.;
  const float maxDist = 2./sqrt((_maxVec-_minVec)%(_maxVec-_minVec));
  m = ar_scaleMatrix(maxDist) * ar_translationMatrix(-_center);
#endif
  return true;
}

#ifdef Enable3DS
// helper function for normalizationMatrix() recursion
// @param node Lib3dsNode to start from
// @param _minVec smallest (x, y, z) position of all points in entire file
// @param _maxVec largest (x, y, z) position of all points in entire file
void ar3DS::subNormalizationMatrix(Lib3dsNode* node,
                                   arVector3 &_minVec, arVector3 &_maxVec) {
  // recurse through children
  for (Lib3dsNode* p=node->childs; p!=NULL; p=p->next)
    subNormalizationMatrix(p, _minVec, _maxVec);
  if (node->type != LIB3DS_OBJECT_NODE || !strcmp(node->name, "$$$DUMMY"))
    return;

  Lib3dsMesh *mesh = lib3ds_file_mesh_by_name(_file, node->name);
  ASSERT(mesh);
  if (!mesh)
    return;

  Lib3dsMatrix invMeshMatrix;
  lib3ds_matrix_copy(invMeshMatrix, mesh->matrix);
  lib3ds_matrix_inv(invMeshMatrix);
  Lib3dsVector v;
  for (unsigned int j=0; j<mesh->faces; ++j) {
    Lib3dsFace& f = mesh->faceL[j];
    for (unsigned i=0; i<3; ++i) {
      lib3ds_vector_transform(v, invMeshMatrix, mesh->pointL[f.points[i]].pos);
      for (unsigned k=0; k<3; k++) {
        if (v[k] > _maxVec[k])
          _maxVec[k] = v[k];
        if (v[k] < _minVec[k])
          _minVec[k] = v[k];
      }
    }
  }
}
#endif

// Put object inside unit sphere.
// Flag attachMesh() to use a normalized matrix as the root
// node in the scenegraph representation of the mesh
void ar3DS::normalizeModelSize() {
  _normalize = true;
}

///// Animation functions /////

// Set the current frame of animation
// @param newFrame The frame to jump to in the animation
// todo: check bounds on newFrame
bool ar3DS::setFrame(int newFrame) {
/*  if (newFrame >= numFrames || newFrame < 0) return false;

  for(unsigned int i=0; i<segmentData.size(); i++) {
    dgTransform(segmentData[i]->transformID, segmentData[i]->frame[newFrame]->trans);
  }
*/
  _currentFrame = newFrame;
  return true;
}

// Goes to next frame in animation; returns false if already at the end
bool ar3DS::nextFrame() {
  return setFrame(_currentFrame+1);
}

// Goes to previous frame in animation; returns false if already at the beginning
bool ar3DS::prevFrame() {
  return setFrame(_currentFrame-1);
}
