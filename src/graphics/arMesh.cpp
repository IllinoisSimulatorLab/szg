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

  // Precompute trig tables.
  float* cn = new float[2*_numberDivisions];
  float* sn = new float[2*_numberDivisions];
  int i = 0;
  for (i=0; i<2*_numberDivisions; ++i){
    cn[i] = cos( (2*M_PI * i) / _numberDivisions);
    sn[i] = sin( (2*M_PI * i) / _numberDivisions);
  }

  // Populate the points array.
  float* ppt = pointPositions-3;
  for (i=0; i<_numberDivisions; ++i){
    // std::iota would have been nice, had it not been deprecated.  Visual Studio 7 lacks it.
    pointIDs[i] = i;
    arVector3(_bottomRadius*cn[i], _bottomRadius*sn[i], -0.5).get(ppt+=3);
  }
  for (; i<2*_numberDivisions; ++i){
    pointIDs[i] = i;
    arVector3(_topRadius*cn[i], _topRadius*sn[i], 0.5).get(ppt+=3);
  }

  // Populate the triangle arrays.
  // std::iota would have been nice, had it not been deprecated.  Visual Studio 7 lacks it.
  for (i=0; i<2*_numberDivisions; ++i)
    triangleIDs[i] = i;

  int* pv = triangleVertices;
  float* pn = normals -3; // -3 cleverly offsets +=3 *pre*increment.
  float* pt = texCoords;
  for (i=0; i<_numberDivisions; ++i){
    const int i1 = (i+1) % _numberDivisions;
    *pv++ = i+_numberDivisions;
    *pv++ = i;
    *pv++ = i1;
    *pv++ = i+_numberDivisions;
    *pv++ = i1;
    *pv++ = i1+_numberDivisions;

    arVector3(cn[i ], sn[i ], 0).get(pn+=3);
    arVector3(cn[i ], sn[i ], 0).get(pn+=3);
    arVector3(cn[i1], sn[i1], 0).get(pn+=3);
    arVector3(cn[i ], sn[i ], 0).get(pn+=3);
    arVector3(cn[i1], sn[i1], 0).get(pn+=3);
    arVector3(cn[i1], sn[i1], 0).get(pn+=3);

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

  if (_useEnds) {
    // Construct polygons for the ends.
    const int iTopPt = 2*_numberDivisions;
    const int iBotPt = iTopPt + 1;
    pointIDs[iTopPt] = iTopPt;
    pointIDs[iBotPt] = iBotPt;
    arVector3(0, 0,  0.5).get(pointPositions + 3*iTopPt);
    arVector3(0, 0, -0.5).get(pointPositions + 3*iBotPt);

    for (i=0; i<_numberDivisions; ++i){
      const int i1 = (i+1) % _numberDivisions;
      const int iTopTri = 2*_numberDivisions+i;
      const int iBotTri = 3*_numberDivisions+i;
      triangleIDs[iTopTri] = iTopTri;
      triangleIDs[iBotTri] = iBotTri;
      triangleVertices[3*iTopTri  ] = i  + _numberDivisions;
      triangleVertices[3*iTopTri+1] = i1 + _numberDivisions;
      triangleVertices[3*iTopTri+2] = iTopPt;
      triangleVertices[3*iBotTri  ] = i;
      triangleVertices[3*iBotTri+1] = iBotPt;
      triangleVertices[3*iBotTri+2] = i1;

      float* pn = normals + 9*iTopTri;
      arVector3 v(0,0,1);
      v.get(pn);
      v.get(pn+=3);
      v.get(pn+=3);
      pn = normals + 9*iBotTri;
      v *= -1;
      v.get(pn);
      v.get(pn+=3);
      v.get(pn+=3);

      float* pt = texCoords + 6*iTopTri;
      *pt++ = 0.5 + 0.5*cn[i];
      *pt++ = 0.5 + 0.5*sn[i];
      *pt++ = 0.5 + 0.5*cn[i+1];
      *pt++ = 0.5 + 0.5*sn[i+1];
      *pt++ = 0.5;
      *pt++ = 0.5;
      pt = texCoords + 6*iBotTri;
      *pt++ = 0.5 + 0.5*cn[i];
      *pt++ = 0.5 + 0.5*sn[i];
      *pt++ = 0.5;
      *pt++ = 0.5;
      *pt++ = 0.5 + 0.5*cn[i+1];
      *pt++ = 0.5 + 0.5*sn[i+1];
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

/////////  FOUR-SIDED PYRAMID  //////////////////////////////////////////

bool arPyramidMesh::attachMesh(arGraphicsNode* parent, const string& name){
  const int numberPoints = 5;
  float pointPositions[3*numberPoints] = {
    0,0,0.5, -0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,0.5,-0.5, -0.5,0.5,-0.5 };
  const int numberTriangles = 6;
  int triangleVertices[3*numberTriangles] = {
    1,3,2, 1,4,3, 1,2,0, 2,3,0, 3,4,0, 4,1,0 };
  const arVector3 faceNormals[4] = { // for triangular sides
    (arVector3(.5,-.5,-1)  * arVector3(-.5,-.5,-1)).normalize(),
    (arVector3(.5,.5,-1)   * arVector3(.5,-.5,-1)) .normalize(),
    (arVector3(-.5,.5,-1)  * arVector3(.5,.5,-1))  .normalize(),
    (arVector3(-.5,-.5,-1) * arVector3(-.5,.5,-1)) .normalize() };
  float normals[9*numberTriangles] = {
    0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
    faceNormals[0].v[0], faceNormals[0].v[1], faceNormals[0].v[2], 
    faceNormals[0].v[0], faceNormals[0].v[1], faceNormals[0].v[2],
    faceNormals[0].v[0], faceNormals[0].v[1], faceNormals[0].v[2],
    faceNormals[1].v[0], faceNormals[1].v[1], faceNormals[1].v[2], 
    faceNormals[1].v[0], faceNormals[1].v[1], faceNormals[1].v[2], 
    faceNormals[1].v[0], faceNormals[1].v[1], faceNormals[1].v[2], 
    faceNormals[2].v[0], faceNormals[2].v[1], faceNormals[2].v[2],
    faceNormals[2].v[0], faceNormals[2].v[1], faceNormals[2].v[2],
    faceNormals[2].v[0], faceNormals[2].v[1], faceNormals[2].v[2],
    faceNormals[3].v[0], faceNormals[3].v[1], faceNormals[3].v[2],
    faceNormals[3].v[0], faceNormals[3].v[1], faceNormals[3].v[2],
    faceNormals[3].v[0], faceNormals[3].v[1], faceNormals[3].v[2]};
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

arSphereMesh::arSphereMesh(int n) :
  _sectionSkip(1)
{
  setAttributes(n);
}

arSphereMesh::arSphereMesh(const arMatrix4& transform, int n) :
  arMesh(transform),
  _sectionSkip(1)
{
  setAttributes(n);
}

void arSphereMesh::setAttributes(int n){
  if (n < 3)
    ar_log_error() << "arSphereMesh needs at least 3 divisions, not " << n << ".\n";
  else
    _numberDivisions = n;
}

// Show only every sectionSkip'th vertical band, as cheap transparency
void arSphereMesh::setSectionSkip(int sectionSkip){
  _sectionSkip = sectionSkip;
}

bool arSphereMesh::attachMesh(arGraphicsNode* parent, const string& name){
  const int numberPoints = (_numberDivisions+1) * _numberDivisions;

  // _sectionSkip may change the number of triangles.
  int numberTriangles = 2*_numberDivisions*_numberDivisions;

  float* pointPositions = new float[3*numberPoints];
  int* triangleVertices = new int[3*numberTriangles];
  float* texCoords = new float[3*texcoordsPerTri*numberTriangles];
  float* normals = new float[3*normalsPerTri*numberTriangles];

  // Precompute trig tables.
  double* cn = new double[_numberDivisions+1];
  double* sn = new double[_numberDivisions+1];
  int i;
  for (i=0; i<_numberDivisions+1; ++i) {
    cn[i] = cos((2*M_PI * i) / _numberDivisions);
    sn[i] = sin((2*M_PI * i) / _numberDivisions);
  }

  // Populate the points array:
  // n points at north pole (0,0,1),
  // n points along a circle of constant latitude (_,_,z),
  // ...
  // n points at south pole (0,0,-1).
  int j=0;
  int iPoint = 0;
  for (j=0; j<_numberDivisions+1; ++j){
    const double z = sin(M_PI * (.5 - double(j)/_numberDivisions));
    const double radius = sqrt(1. - z*z);
    for (i=0; i<_numberDivisions; ++i, iPoint+=3){
      arVector3(radius*cn[i], radius*sn[i], z).get(pointPositions+iPoint);
    }
  }

  // Todo: coalesce all j==0 points into one (they're all at the north pole),
  // and also for j==_numberDivisions south pole.
  //
  // Todo: omit degenerate triangles, n at each pole,
  // which have only 2 distinct vertices.

  // Populate the triangle arrays.
  int* pv = triangleVertices;
  float* pt = texCoords;
  float* pnDst = normals-3; // -3 cleverly counteracts +=3 being pre-increment unlike ++ postincrement
  // j is latitude-circles.
  // i is longitude-points along each circle.
  for (j=0; j<_numberDivisions; ++j){
    for (i=0; i<_numberDivisions; ++i){
      const int i1 = (i+1) % _numberDivisions;

      // Two triangles fill a quad between j'th and j+1'th circles.
      *pv++ = j     * _numberDivisions + i;
      *pv++ = (j+1) * _numberDivisions + i;
      *pv++ = (j+1) * _numberDivisions + i1;

      *pv++ = j     * _numberDivisions + i;
      *pv++ = (j+1) * _numberDivisions + i1;
      *pv++ = j     * _numberDivisions + i1;

      // 12 texture coords match 6 (i,j) vertex coords of those two triangles.
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

      // Normals have same coords as points on this unit sphere.
      // So just copy those coords, don't recompute them.
      #define pnSrc(lat,lon) (pointPositions + 3*((lat)*_numberDivisions + (lon)))
      memcpy(pnDst+=3, pnSrc(j  ,i  ), 3*sizeof(float));
      memcpy(pnDst+=3, pnSrc(j+1,i  ), 3*sizeof(float));
      memcpy(pnDst+=3, pnSrc(j+1,i+1), 3*sizeof(float));
      memcpy(pnDst+=3, pnSrc(j  ,i  ), 3*sizeof(float));
      memcpy(pnDst+=3, pnSrc(j+1,i+1), 3*sizeof(float));
      memcpy(pnDst+=3, pnSrc(j  ,i+1), 3*sizeof(float));
    }
  }

  delete [] sn;
  delete [] cn;

  if (_sectionSkip > 1) {
    // Decimate triangles (omit panels of constant longitude).
    // Shrink and coalesce arrays.
    int* triDst = triangleVertices - 6;
    float* ptDst = texCoords - 12;
    pnDst = normals - 18;
    for (j=0; j<_numberDivisions; ++j){
      for (i=0; i<_numberDivisions; i+=_sectionSkip){
	const int iTriangleSrc = 2 * (j*_numberDivisions + i);
	const int iVertex = iTriangleSrc * 3;
	const int iCoord  = iTriangleSrc * 6;
	const int iNormal = iTriangleSrc * 9;
	memcpy(triDst += 6, triangleVertices + iVertex, 6 * sizeof(int));
	memcpy(ptDst += 12, texCoords + iCoord, 12 * sizeof(float));
	memcpy(pnDst += 18, normals + iNormal, 18 * sizeof(float));
      }
    }
    numberTriangles /= _sectionSkip;
  }

  // Apply matrix to points and normals.
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
