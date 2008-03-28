//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMesh.h"
#include "arDrawableNode.h"

// (Private constants don't need heavy naming conventions.)
const int indicesPerTri = 3;
const int normalsPerTri = 3;
const int texcoordsPerTri = 3;

///// Helper functions used by each of the meshes ////////////////////

void ar_adjustPoints(const arMatrix4& m, const int number, float* data){
  for (int i=0; i<number; ++i){
    (m * arVector3(data+i*3)).get(data+i*3);
  }
}

void ar_adjustNormals(const arMatrix4& m, int number, float* data){
  for (int i=0; i<number; ++i){
    // Normals transform without the translational component.
    (m * arVector3(data+i*3) - m * arVector3(0,0,0)).get(data+i*3);
  }
}

bool ar_attachMesh(arGraphicsNode* parent, const string& name,
                   const int numberPoints, float* pointPositions,
                   const int numberVertices, int* triangleVertices,
		   const int numberNormals, float* normals,
		   const int numberTexCoord, float* texCoords,
		   const arDrawableType drawableType, const int numberPrimitives){
  bool success = false;
  arPointsNode* points = NULL;
  arIndexNode* index = NULL;
  arNormal3Node* normal3 = NULL;
  arTex2Node* tex2 = NULL;
  arDrawableNode* draw = NULL;

  points = (arPointsNode*) parent->newNodeRef("points", name+".points");
  // Error checking for thread-safety... nodes can be deleted out
  // from under us at any time.
  if (!points){
    goto LDone;
  }
  points->setPoints(numberPoints, pointPositions);
  index = (arIndexNode*) points->newNodeRef("index", name+".indices"); 
  if (!index){
    goto LDone;
  }
  index->setIndices(numberVertices, triangleVertices);
  normal3 = (arNormal3Node*) index->newNodeRef("normal3", name+".normal3");
  if (!normal3){
    goto LDone;
  }
  normal3->setNormal3(numberNormals, normals);
  tex2 = (arTex2Node*) normal3->newNodeRef("tex2", name+".tex2");
  if (!tex2){
    goto LDone;
  }
  tex2->setTex2(numberTexCoord, texCoords);
  draw = (arDrawableNode*) tex2->newNodeRef("drawable", name+".drawable");
  if (!draw){
    goto LDone;
  }
  draw->setDrawable(drawableType, numberPrimitives);
  success = true;

LDone:
  // Avoid memory leaks.
  if (points) points->unref();
  if (index) index->unref();
  if (normal3) normal3->unref();
  if (tex2) tex2->unref();
  if (draw) draw->unref();
  return success;
}

bool arMesh::attachMesh(const string& name,
			const string& parentName){
  arGraphicsNode* g = dgGetNode(parentName);
  return g && attachMesh(g, name);
}

/////////  CUBE  /////////////////////////////////////////////////////

bool arCubeMesh::attachMesh(arGraphicsNode* parent, const string& name){
  const int numberTriangles = 12;
  const int numberPoints = 8;
  ARfloat pointPositions[numberPoints*3] = {-0.5,0.5,0.5, -0.5,0.5,-0.5,
                                0.5,0.5,-0.5, 0.5,0.5,0.5,
                                -0.5,-0.5,0.5, -0.5,-0.5,-0.5,
                                0.5,-0.5,-0.5, 0.5,-0.5,0.5};
  ARint triangleVertices[numberTriangles*3] = {5,4,0, 5,0,1, 6,5,1, 6,1,2,
				7,6,2, 7,2,3, 4,7,3, 4,3,0,
				2,1,0, 2,0,3, 7,4,5, 7,5,6};
  ARfloat normals[3*numberTriangles*normalsPerTri] 
                      = {-1,0,0, -1,0,0, -1,0,0, -1,0,0, -1,0,0, -1,0,0,
			  0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
			  1,0,0, 1,0,0, 1,0,0, 1,0,0, 1,0,0, 1,0,0,
                          0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1,
			  0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0,
			  0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0};
  ARfloat texCoords[3*numberTriangles*texcoordsPerTri] 
                        = { 1,0, 0,0, 0,1, 1,0, 0,1, 1,1,
                            1,0, 0,0, 0,1, 1,0, 0,1, 1,1,
                            1,0, 0,0, 0,1, 1,0, 0,1, 1,1,
                            1,0, 0,0, 0,1, 1,0, 0,1, 1,1,
                            1,0, 0,0, 0,1, 1,0, 0,1, 1,1,
			    1,0, 0,0, 0,1, 1,0, 0,1, 1,1 };

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  return ar_attachMesh(parent, name,
                       numberPoints, pointPositions,
                       numberTriangles*indicesPerTri, triangleVertices,
		       numberTriangles*normalsPerTri, normals,
		       numberTriangles*texcoordsPerTri, texCoords,
		       DG_TRIANGLES, numberTriangles);
}

/////////  RECTANGLE  /////////////////////////////////////////////////////

bool arRectangleMesh::attachMesh(arGraphicsNode* parent,
                                 const string& name){
  const int numberTriangles = 2;
  const int numberPoints = 4;
  // [-.5,.5] square in X*Z;  Y is zero.
  ARfloat pointPositions[numberPoints*3] 
    =  {-.5,0,.5,  -.5,0,-.5,  .5,0,-.5,  .5,0,.5}; 
  ARint triangleVertices[3*numberTriangles] = {2,1,0, 2,0,3};
  ARfloat texCoords[3*numberTriangles*texcoordsPerTri] 
    = { 1,0, 0,0, 0,1, 1,0, 0,1, 1,1 };
  ARfloat normals[3*numberTriangles*normalsPerTri] 
    = {0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0};

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  return ar_attachMesh(parent, name,
                       numberPoints, pointPositions,
                       numberTriangles*indicesPerTri, triangleVertices,
		       numberTriangles*normalsPerTri, normals,
		       numberTriangles*texcoordsPerTri, texCoords,
		       DG_TRIANGLES, numberTriangles);
}

/////////  CYLINDER  /////////////////////////////////////////////////////

arCylinderMesh::arCylinderMesh() :
  _numberDivisions(10),
  _bottomRadius(1),
  _topRadius(1),
  _useEnds(false)
{
}

arCylinderMesh::arCylinderMesh(const arMatrix4& transform) :
  arMesh(transform),
  _numberDivisions(10),
  _bottomRadius(1),
  _topRadius(1),
  _useEnds(false)
{
}

void arCylinderMesh::setAttributes(int numberDivisions,
			           float bottomRadius,
			           float topRadius){
  _numberDivisions = numberDivisions;
  _bottomRadius = bottomRadius;
  _topRadius = topRadius;
}

void arCylinderMesh::toggleEnds(bool value){
  _useEnds = value;
}

bool arCylinderMesh::attachMesh(arGraphicsNode* parent, const string& name){
  int numberPoints = 2*_numberDivisions;
  int numberTriangles = 2*_numberDivisions;
  if (_useEnds){
    numberPoints += 2;
    numberTriangles *= 2;
  }
  int* pointIDs = new int[numberPoints];
  float* pointPositions = new float[3*numberPoints];
  int* triangleIDs = new int[numberTriangles];
  int* triangleVertices = new int[3*numberTriangles];
  float* texCoords = new float[2*texcoordsPerTri*numberTriangles];
  float* normals = new float[3*normalsPerTri*numberTriangles];

  // Precompute trig tables
  float* cn = new float[2*_numberDivisions];
  float* sn = new float[2*_numberDivisions];
  int i = 0;
  for (i=0; i<2*_numberDivisions; ++i){
    cn[i] = cos( (2*M_PI * i) / _numberDivisions);
    sn[i] = sin( (2*M_PI * i) / _numberDivisions);
  }

  // fill in the common triangles
  // populate the points array
  arVector3 location;
  for (i=0; i<2*_numberDivisions; ++i){
    pointIDs[i] = i;
    if (i<_numberDivisions){
      location.set(_bottomRadius*cn[i], _bottomRadius*sn[i], -0.5);
    }
    else{
      location.set(_topRadius*cn[i], pointPositions[3*i+1] = _topRadius*sn[i], 0.5);
    }
    location.get(pointPositions+3*i);
  }
  // populate the triangles array
  for (i=0; i<2*_numberDivisions; ++i){
    triangleIDs[i] = i;
  }
  for (i=0; i<_numberDivisions; ++i){
    const int j = (i+1) % _numberDivisions;
    int* pv = triangleVertices + 6*i;
    *pv++ = i+_numberDivisions;
    *pv++ = i;
    *pv++ = j;
    *pv++ = i+_numberDivisions;
    *pv++ = j;
    *pv++ = j+_numberDivisions;

    // note i vs j!
    float* pn = normals+18*i;
    arVector3(cn[i], sn[i], 0).get(pn);
    arVector3(cn[i], sn[i], 0).get(pn+=3);
    arVector3(cn[j], sn[j], 0).get(pn+=3);
    arVector3(cn[i], sn[i], 0).get(pn+=3);
    arVector3(cn[j], sn[j], 0).get(pn+=3);
    arVector3(cn[j], sn[j], 0).get(pn+=3);

    float* pt = texCoords + 12*i;
    *pt++ = float(i) / _numberDivisions;
    *pt++ = 0.;
    *pt++ = float(i) / _numberDivisions;
    *pt++ = 1.;
    *pt++ = float(i+1) / _numberDivisions;
    *pt++ = 1.;
    *pt++ = float(i) / _numberDivisions;
    *pt++ = 0.;
    *pt++ = float(i+1) / _numberDivisions;
    *pt++ = 1.;
    *pt++ = float(i+1) / _numberDivisions;
    *pt++ = 0.;
  }

  if (_useEnds){
    // construct polygons for the ends
    const int topPoint = 2*_numberDivisions;
    const int bottomPoint = topPoint + 1;
    pointIDs[topPoint] = topPoint;
    pointIDs[bottomPoint] = bottomPoint;
    arVector3(0, 0,  0.5).get(pointPositions + 3*topPoint);
    arVector3(0, 0, -0.5).get(pointPositions + 3*bottomPoint);
    for (i=0; i<_numberDivisions; ++i){
      const int j = (i+1) % _numberDivisions;
      const int topTriangle = 2*_numberDivisions+i;
      const int bottomTriangle = 3*_numberDivisions+i;
      triangleIDs[topTriangle] = topTriangle;
      triangleIDs[bottomTriangle] = bottomTriangle;
      triangleVertices[3*topTriangle] = i+_numberDivisions;
      triangleVertices[3*topTriangle+1] = j+_numberDivisions;
      triangleVertices[3*topTriangle+2] = topPoint;
      triangleVertices[3*bottomTriangle] = i;
      triangleVertices[3*bottomTriangle+1] = bottomPoint;
      triangleVertices[3*bottomTriangle+2] = j;

      float* pn = normals + 9*topTriangle;
      const arVector3 vTop(0,0,1);
      vTop.get(pn);
      vTop.get(pn+=3);
      vTop.get(pn+=3);
      pn = normals + 9*bottomTriangle;
      const arVector3 vBottom(0,0,-1);
      vBottom.get(pn);
      vBottom.get(pn+=3);
      vBottom.get(pn+=3);

      float* pt = texCoords + 6*topTriangle;
      *pt++ = 0.5+0.5*cn[i];
      *pt++ = 0.5+0.5*sn[i];
      *pt++ = 0.5+0.5*cn[i+1];
      *pt++ = 0.5+0.5*sn[i+1];
      *pt++ = 0.5;
      *pt++ = 0.5;
      pt = texCoords + 6*bottomTriangle;
      *pt++ = 0.5+0.5*cn[i];
      *pt++ = 0.5+0.5*sn[i];
      *pt++ = 0.5;
      *pt++ = 0.5;
      *pt++ = 0.5+0.5*cn[i+1];
      *pt++ = 0.5+0.5*sn[i+1];
    }
  }

  delete [] cn;
  delete [] sn;

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  const bool ok = ar_attachMesh(parent, name,
    numberPoints, pointPositions,
    numberTriangles*indicesPerTri, triangleVertices,
    numberTriangles*normalsPerTri, normals,
    numberTriangles*texcoordsPerTri, texCoords,
    DG_TRIANGLES, numberTriangles);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] texCoords;
  delete [] normals;
  return ok;
}

/////////  PYRAMID  /////////////////////////////////////////////////////

bool arPyramidMesh::attachMesh(arGraphicsNode* parent, 
                               const string& name){
  const int numberPoints = 5;
  const int numberTriangles = 6;
  float pointPositions[3*numberPoints] = {
    0,0,0.5, -0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,0.5,-0.5, -0.5,0.5,-0.5 };
  int triangleVertices[3*numberTriangles] = {
    1,3,2, 1,4,3, 1,2,0, 2,3,0, 3,4,0, 4,1,0 };
  arVector3 faceNormals[4]; // normals for the triangular sides of the pyramid
  faceNormals[0] = arVector3(0.5,-0.5,-1)*arVector3(-0.5,-0.5,-1);
  faceNormals[0].normalize();
  faceNormals[1] = arVector3(0.5,0.5,-1)*arVector3(0.5,-0.5,-1);
  faceNormals[1].normalize();
  faceNormals[2] = arVector3(-0.5,0.5,-1)*arVector3(0.5,0.5,-1);
  faceNormals[2].normalize();
  faceNormals[3] = arVector3(-0.5,-0.5,-1)*arVector3(-0.5,0.5,-1);
  faceNormals[3].normalize();
  float normals[9*numberTriangles] = {
    0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
       faceNormals[0][0], faceNormals[0][1], faceNormals[0][2], 
       faceNormals[0][0], faceNormals[0][1], faceNormals[0][2],
       faceNormals[0][0], faceNormals[0][1], faceNormals[0][2],
       faceNormals[1][0], faceNormals[1][1], faceNormals[1][2], 
       faceNormals[1][0], faceNormals[1][1], faceNormals[1][2], 
       faceNormals[1][0], faceNormals[1][1], faceNormals[1][2], 
       faceNormals[2][0], faceNormals[2][1], faceNormals[2][2],
       faceNormals[2][0], faceNormals[2][1], faceNormals[2][2],
       faceNormals[2][0], faceNormals[2][1], faceNormals[2][2],
       faceNormals[3][0], faceNormals[3][1], faceNormals[3][2],
       faceNormals[3][0], faceNormals[3][1], faceNormals[3][2],
       faceNormals[3][0], faceNormals[3][1], faceNormals[3][2]};
  float texCoords[6*numberTriangles] = {
    0,0,1,1,1,0,
    0,0,0,1,1,1,
    0,0,1,0,0.5,0.5,
    1,0,1,1,0.5,0.5,
    1,1,0,1,0.5,0.5,
    0,1,0,0,0.5,0.5 };
  
  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  return ar_attachMesh(parent, name,
                       numberPoints, pointPositions,
                       numberTriangles*indicesPerTri, triangleVertices,
		       numberTriangles*normalsPerTri, normals,
		       numberTriangles*texcoordsPerTri, texCoords,
		       DG_TRIANGLES, numberTriangles);
}

/////////  SPHERE  /////////////////////////////////////////////////////

arSphereMesh::arSphereMesh(int numberDivisions) :
  _numberDivisions(numberDivisions),
  _sectionSkip(1)
{}

arSphereMesh::arSphereMesh(const arMatrix4& transform, int numberDivisions) :
  arMesh(transform),
  _numberDivisions(numberDivisions),
  _sectionSkip(1)
{}

void arSphereMesh::setAttributes(int numberDivisions){
  _numberDivisions = numberDivisions;
}

// Hide some vertical bands, as cheap transparency
void arSphereMesh::setSectionSkip(int sectionSkip){
  _sectionSkip = sectionSkip;
}

bool arSphereMesh::attachMesh(arGraphicsNode* parent, const string& name){
  // Decimation may change the number of triangles, later in the function.
  int numberTriangles = 2*_numberDivisions*_numberDivisions;
  const int numberPoints = (_numberDivisions+1)*_numberDivisions;

  float* pointPositions = new float[3*numberPoints];
  int* triangleVertices = new int[3*numberTriangles];
  float* texCoords = new float[3*texcoordsPerTri*numberTriangles];
  float* normals = new float[3*normalsPerTri*numberTriangles];

  // todo: in other arXXXMeshes too
  // Precompute trig tables
  float* cn = new float[_numberDivisions+1];
  float* sn = new float[_numberDivisions+1];
  float* s2 = new float[_numberDivisions+1];
  int i;
  for (i=0; i<_numberDivisions+1; ++i){
    cn[i] = cos((2*M_PI * i) / _numberDivisions);
    sn[i] = sin((2*M_PI * i) / _numberDivisions);
    s2[i] = sin(M_PI * (.5 - i/_numberDivisions));
  }

  // populate the points array
  int j=0;
  int iPoint = 0;
  for (j=0; j<_numberDivisions+1; ++j){
    for (i=0; i<_numberDivisions; ++i, iPoint+=3){
      const float z = s2[j];
      const float radius = (z*z>=1) ? 0. : sqrt(1-z*z);
      arVector3(radius*cn[i], radius*sn[i], z).get(pointPositions+iPoint);
    }
  }

  // populate the triangles arrays
  int iTriangle = 0;
  for (j=0; j<_numberDivisions; ++j){
    for (i=0; i<_numberDivisions; ++i, iTriangle++){
      const int k = (i+1) % _numberDivisions;
      int* pv = triangleVertices + 6*iTriangle;
      *pv++ = j    *_numberDivisions + i;
      *pv++ = (j+1)*_numberDivisions + i;
      *pv++ = (j+1)*_numberDivisions + k;
      *pv++ = j    *_numberDivisions + i;
      *pv++ = (j+1)*_numberDivisions + k;
      *pv++ = j    *_numberDivisions + k;

      float* pn = normals + 18*iTriangle;
      float z = s2[j];
      float radius = sqrt(1-z*z);
      arVector3(radius*cn[i], radius*sn[i], z).get(pn);
      arVector3(radius*cn[i], radius*sn[i], z).get(pn+9);
      arVector3(radius*cn[k], radius*sn[k], z).get(pn+15);

      z = s2[j+1];
      radius = sqrt(1-z*z);
      arVector3(radius*cn[i], radius*sn[i], z).get(pn+3);
      arVector3(radius*cn[k], radius*sn[k], z).get(pn+6);
      arVector3(radius*cn[k], radius*sn[k], z).get(pn+12);

      float* pt = texCoords + 12*iTriangle;
      *pt++ = float(i)   / _numberDivisions;
      *pt++ = float(j)   / _numberDivisions;
      *pt++ = float(i)   / _numberDivisions;
      *pt++ = float(j+1) / _numberDivisions;
      *pt++ = float(i+1) / _numberDivisions;
      *pt++ = float(j+1) / _numberDivisions;
      *pt++ = float(i)   / _numberDivisions;
      *pt++ = float(j)   / _numberDivisions;
      *pt++ = float(i+1) / _numberDivisions;
      *pt++ = float(j+1) / _numberDivisions;
      *pt++ = float(i+1) / _numberDivisions;
      *pt++ = float(j)   / _numberDivisions;
    }
  }

  delete [] s2;
  delete [] sn;
  delete [] cn;

  // Decimate triangles, if _sectionSkip.
  for (j=0; j<_numberDivisions; ++j){
    for (i=0; i<_numberDivisions; i+=_sectionSkip){
      const int whichTriangle = 2 * (j*_numberDivisions + i);
      const int farVertex = whichTriangle * 3;
      const int farNormal = whichTriangle * 6;
      const int farCoord  = whichTriangle * 9;
      memcpy(triangleVertices, triangleVertices + farVertex, 6 * sizeof(int));
      memcpy(normals, normals + farNormal, 18 * sizeof(float));
      memcpy(texCoords, texCoords + farCoord, 12 * sizeof(float));
    }
  }
  numberTriangles = 2*_numberDivisions * _numberDivisions/_sectionSkip;

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  const bool ok = ar_attachMesh(parent, name, numberPoints, pointPositions,
    numberTriangles*indicesPerTri, triangleVertices, numberTriangles*normalsPerTri,
    normals, numberTriangles*texcoordsPerTri, texCoords, DG_TRIANGLES, numberTriangles);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] texCoords;
  delete [] normals;
  return ok;
}

/////////  TORUS  /////////////////////////////////////////////////////

arTorusMesh::arTorusMesh(int numberBigAroundQuads, 
                         int numberSmallAroundQuads,
                         float bigRadius, 
                         float smallRadius) :
  _pointPositions(NULL),
  _triangleVertices(NULL),
  _surfaceNormals(NULL),
  _textureCoordinates(NULL)
  
{
  _reset(numberBigAroundQuads,numberSmallAroundQuads,bigRadius,smallRadius);
}

arTorusMesh::~arTorusMesh(){
  _destroy();
}

void arTorusMesh::_destroy(){
  if (_pointPositions){
    delete [] _pointPositions;
    delete [] _triangleVertices;
    delete [] _textureCoordinates;
    delete [] _surfaceNormals;
  }
}

void arTorusMesh::reset(int numberBigAroundQuads, 
                        int numberSmallAroundQuads,
                        float bigRadius, 
                        float smallRadius){
  _reset(numberBigAroundQuads,numberSmallAroundQuads,bigRadius,smallRadius);
}

bool arTorusMesh::attachMesh(arGraphicsNode* parent, const string& name){
  ar_adjustPoints(_matrix, _numberPoints, _pointPositions);
  ar_adjustNormals(_matrix, _numberTriangles*normalsPerTri, _surfaceNormals);
  return ar_attachMesh(parent, name, _numberPoints, _pointPositions,
    _numberTriangles*indicesPerTri, _triangleVertices,
    _numberTriangles*normalsPerTri, _surfaceNormals, _numberTriangles*texcoordsPerTri,
    _textureCoordinates, DG_TRIANGLES, _numberTriangles);
}

static inline int modAdd(const int base, const int x) {
  return (x < 0) ? x+base : (x >= base) ? x-base : x;
}

void arTorusMesh::_reset(const int numberBigAroundQuads, const int numberSmallAroundQuads,
                         const float bigRadius, const float smallRadius){
  _numberBigAroundQuads = numberBigAroundQuads;
  _numberSmallAroundQuads = numberSmallAroundQuads;
  _bigRadius = bigRadius;
  _smallRadius = smallRadius;

  _numberPoints = numberBigAroundQuads * numberSmallAroundQuads;
  _numberTriangles = 2 * _numberPoints;
  _destroy();

  _pointPositions = new float[3*_numberPoints];
  _triangleVertices = new int[3*_numberTriangles];
  _surfaceNormals = new float[3*normalsPerTri*_numberTriangles];
  _textureCoordinates = new float[2*texcoordsPerTri*_numberTriangles];

  for (int i=0; i<numberBigAroundQuads;++i){
    const int smalli = modAdd(numberBigAroundQuads, i+1);
    const float sin_i = sin(2*M_PI*i/numberBigAroundQuads);
    const float cos_i = cos(2*M_PI*i/numberBigAroundQuads);
    const float sin_smalli = sin(2*M_PI*smalli/numberBigAroundQuads);
    const float cos_smalli = cos(2*M_PI*smalli/numberBigAroundQuads);

    for (int j=0; j<numberSmallAroundQuads; ++j){
      // todo: precompute in tables sin_j[j] etc.
      const int smallj = modAdd(numberSmallAroundQuads, j+1);
      const float sin_j = sin(2*M_PI*j/numberSmallAroundQuads);
      const float cos_j = cos(2*M_PI*j/numberSmallAroundQuads);
      const float sin_smallj = sin(2*M_PI*smallj/numberSmallAroundQuads);
      const float cos_smallj = cos(2*M_PI*smallj/numberSmallAroundQuads);
      const int whichPoint = i*numberSmallAroundQuads+j;

      arVector3(
	  bigRadius*cos_i + smallRadius*cos_i*cos_j,
	  bigRadius*sin_i + smallRadius*sin_i*cos_j,
	  smallRadius*sin_j
	).get(_pointPositions + 3*whichPoint);

      int* pv = _triangleVertices + 6*whichPoint;
      *pv++ =      i * numberSmallAroundQuads +      j;
      *pv++ = smalli * numberSmallAroundQuads +      j;
      *pv++ =      i * numberSmallAroundQuads + smallj;
      *pv++ = smalli * numberSmallAroundQuads +      j;
      *pv++ = smalli * numberSmallAroundQuads + smallj;
      *pv++ =      i * numberSmallAroundQuads + smallj;

      // normal for each vertex of each triangle
      float* pn = _surfaceNormals + 18*whichPoint;
      arVector3(cos_i*cos_j, sin_i*cos_j, sin_j)                         .get(pn);
      arVector3(cos_smalli *cos_j, sin_smalli *cos_j, sin_j)             .get(pn += 3);
      arVector3(cos_i*cos_smallj, sin_i*cos_smallj, sin_smallj)          .get(pn += 3);
      arVector3(cos_smalli*cos_j, sin_smalli*cos_j, sin_j)               .get(pn += 3);
      arVector3(cos_smalli*cos_smallj, sin_smalli*cos_smallj, sin_smallj).get(pn += 3);
      arVector3(cos_i*cos_smallj, sin_i*cos_smallj, sin_smallj)          .get(pn += 3);

      float* pt = _textureCoordinates + 12*whichPoint;
      *pt++ = float(i  ) / numberBigAroundQuads;
      *pt++ = float(j  ) / numberSmallAroundQuads;
      *pt++ = float(i+1) / numberBigAroundQuads;
      *pt++ = float(j  ) / numberSmallAroundQuads;
      *pt++ = float(i  ) / numberBigAroundQuads;
      *pt++ = float(j+1) / numberSmallAroundQuads;
      *pt++ = float(i+1) / numberBigAroundQuads;
      *pt++ = float(j  ) / numberSmallAroundQuads;
      *pt++ = float(i+1) / numberBigAroundQuads;
      *pt++ = float(j+1) / numberSmallAroundQuads;
      *pt++ = float(i  ) / numberBigAroundQuads;
      *pt++ = float(j+1) / numberSmallAroundQuads;
    }
  }
}
