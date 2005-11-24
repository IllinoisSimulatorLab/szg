//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMesh.h"
#include "arDrawableNode.h"
#include <iostream>

// (Private constants don't need heavy naming conventions.)
const int indicesPerTri = 3;
const int normalsPerTri = 3;
const int texcoordsPerTri = 3;

///// Helper functions used by each of the meshes ////////////////////

void ar_adjustPoints(const arMatrix4& m, int number, float* data){
  for (int i=0; i<number; i++){
    arVector3 v = m * arVector3(data[i*3], data[i*3+1], data[i*3+2]);
    data[i*3  ] = v[0];
    data[i*3+1] = v[1];
    data[i*3+2] = v[2];
  }
}

void ar_adjustNormals(const arMatrix4& m, int number, float* data){
  for (int i=0; i<number; i++){
    // Note that normals transform without the translational component.
    arVector3 v = m * arVector3(data[i*3], data[i*3+1], data[i*3+2])
      - m * arVector3(0,0,0);
    data[i*3  ] = v[0];
    data[i*3+1] = v[1];
    data[i*3+2] = v[2];
  }
}

void ar_attachMesh(arGraphicsNode* parent, const string& name,
                   int numberPoints, float* pointPositions,
                   int numberVertices, int* triangleVertices,
		   int numberNormals, float* normals,
		   int numberTexCoord, float* texCoords,
		   arDrawableType drawableType, int numberPrimitives){
  arPointsNode* points = NULL;
  arIndexNode* index = NULL;
  arNormal3Node* normal3 = NULL;
  arTex2Node* tex2 = NULL;
  arDrawableNode* draw = NULL;

  points = (arPointsNode*) parent->newNodeRef("points", name+".points");
  // Must do error checking for thread-safety... nodes can be deleted out
  // from under us at any time.
  if (!points){
    goto cube_cleanup;
  }
  points->setPoints(numberPoints, pointPositions);
  index = (arIndexNode*) points->newNodeRef("index", name+".indices"); 
  if (!index){
    goto cube_cleanup;
  }
  index->setIndices(numberVertices, triangleVertices);
  normal3 = (arNormal3Node*) index->newNodeRef("normal3", name+".normal3");
  if (!normal3){
    goto cube_cleanup;
  }
  normal3->setNormal3(numberNormals, normals);
  tex2 = (arTex2Node*) normal3->newNodeRef("tex2", name+".tex2");
  if (!tex2){
    goto cube_cleanup;
  }
  tex2->setTex2(numberTexCoord, texCoords);
  draw = (arDrawableNode*) tex2->newNodeRef("drawable", name+".drawable");
  if (!draw){
    goto cube_cleanup;
  }
  draw->setDrawable(drawableType, numberPrimitives);
  
  // Must release our references to the nodes! Otherwise there will be a 
  // memory leak.
cube_cleanup:
  if (points) points->unref();
  if (index) index->unref();
  if (normal3) normal3->unref();
  if (tex2) tex2->unref();
  if (draw) draw->unref();
}

void arMesh::attachMesh(const string& name,
			const string& parentName){
  arGraphicsNode* g = dgGetNode(parentName);
  if (g){
    attachMesh(g, name);
  }
}

/////////  CUBE  /////////////////////////////////////////////////////

void arCubeMesh::attachMesh(arGraphicsNode* parent, const string& name){
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

  ar_attachMesh(parent, name,
                numberPoints, pointPositions,
                numberTriangles*indicesPerTri, triangleVertices,
		numberTriangles*normalsPerTri, normals,
		numberTriangles*texcoordsPerTri, texCoords,
		DG_TRIANGLES, numberTriangles);
}

/////////  RECTANGLE  /////////////////////////////////////////////////////

void arRectangleMesh::attachMesh(arGraphicsNode* parent,
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

  ar_attachMesh(parent, name,
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

void arCylinderMesh::attachMesh(arGraphicsNode* parent, const string& name){
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

  int i = 0;
  // fill in the common triangles first
  // populate the points array
  arVector3 location;
  for (i=0; i<2*_numberDivisions; i++){
    pointIDs[i] = i;
    if (i<_numberDivisions){
      // bottom
      location = arVector3
        (_bottomRadius*cos( (6.283*i)/_numberDivisions ),
         _bottomRadius*sin( (6.283*i)/_numberDivisions ),
         -0.5);
    }
    else{
      // top
      location = arVector3
	(_topRadius*cos( (6.283*i)/_numberDivisions ),
	 pointPositions[3*i+1] = _topRadius*sin( (6.283*i)/_numberDivisions),
         0.5);
    }
    //location = _matrix*location;
    pointPositions[3*i  ] = location[0];
    pointPositions[3*i+1] = location[1];
    pointPositions[3*i+2] = location[2];
  }
  // populate the triangles array
  for (i=0; i<2*_numberDivisions; i++){
    triangleIDs[i] = i;
  }
  for (i=0; i<_numberDivisions; i++){
    const int j = (i+1) % _numberDivisions;
    triangleVertices[6*i  ] = i+_numberDivisions;
    triangleVertices[6*i+1] = i;
    triangleVertices[6*i+2] = j;
    triangleVertices[6*i+3] = i+_numberDivisions;
    triangleVertices[6*i+4] = j;
    triangleVertices[6*i+5] = j+_numberDivisions;

    arVector3 n = arVector3(cos((6.283*i)/_numberDivisions),
                            sin((6.283*i)/_numberDivisions), 0);
    memcpy(normals+18*i, n.v, 3*sizeof(float));
    n = arVector3(cos((6.283*i)/_numberDivisions),
                  sin((6.283*i)/_numberDivisions), 0);
    memcpy(normals+18*i+3, n.v, 3*sizeof(float));
    n = arVector3(cos((6.283*j)/_numberDivisions),
                  sin((6.283*j)/_numberDivisions), 0);
    memcpy(normals+18*i+6, n.v, 3*sizeof(float));

    n = arVector3(cos((6.283*i)/_numberDivisions),
                  sin((6.283*i)/_numberDivisions), 0);
    memcpy(normals+18*i+9, n.v, 3*sizeof(float));
    n = arVector3(cos((6.283*j)/_numberDivisions),
                  sin((6.283*j)/_numberDivisions), 0);
    memcpy(normals+18*i+12, n.v, 3*sizeof(float));
    n = arVector3(cos((6.283*j)/_numberDivisions),
                  sin((6.283*j)/_numberDivisions), 0);
    memcpy(normals+18*i+15, n.v, 3*sizeof(float));

    texCoords[12*i  ] = float(i)/_numberDivisions;
    texCoords[12*i+1] = 0.;
    texCoords[12*i+2] = float(i)/_numberDivisions;
    texCoords[12*i+3] = 1.;
    texCoords[12*i+4] = float(i+1)/_numberDivisions;
    texCoords[12*i+5] = 1.;
    texCoords[12*i+6] = float(i)/_numberDivisions;
    texCoords[12*i+7] = 0.;
    texCoords[12*i+8] = float(i+1)/_numberDivisions;
    texCoords[12*i+9] = 1.;
    texCoords[12*i+10] = float(i+1)/_numberDivisions;
    texCoords[12*i+11] = 0.;
  }

  if (_useEnds){
    // we are constructing polygons for the ends
    const int topPoint = 2*_numberDivisions;
    const int bottomPoint = topPoint + 1;
    pointIDs[topPoint] = topPoint;
    pointIDs[bottomPoint] = bottomPoint;
    location = arVector3(0,0,0.5);
    pointPositions[3*topPoint  ] = location[0];
    pointPositions[3*topPoint+1] = location[1];
    pointPositions[3*topPoint+2] = location[2];
    location = arVector3(0,0,-0.5);
    pointPositions[3*bottomPoint  ] = location[0];
    pointPositions[3*bottomPoint+1] = location[1];
    pointPositions[3*bottomPoint+2] = location[2];
    for (i=0; i<_numberDivisions; i++){
      const int j=(i+1) % _numberDivisions;
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

      normals[9*topTriangle  ] = 0;
      normals[9*topTriangle+1] = 0;
      normals[9*topTriangle+2] = 1;
      normals[9*topTriangle+3] = 0;
      normals[9*topTriangle+4] = 0;
      normals[9*topTriangle+5] = 1;
      normals[9*topTriangle+6] = 0;
      normals[9*topTriangle+7] = 0;
      normals[9*topTriangle+8] = 1;
      normals[9*bottomTriangle  ] = 0;
      normals[9*bottomTriangle+1] = 0;
      normals[9*bottomTriangle+2] = -1;
      normals[9*bottomTriangle+3] = 0;
      normals[9*bottomTriangle+4] = 0;
      normals[9*bottomTriangle+5] = -1;
      normals[9*bottomTriangle+6] = 0;
      normals[9*bottomTriangle+7] = 0;
      normals[9*bottomTriangle+8] = -1;

      texCoords[6*topTriangle  ] = 0.5+0.5*cos((6.283*i)/_numberDivisions);
      texCoords[6*topTriangle+1] = 0.5+0.5*sin((6.283*i)/_numberDivisions);
      texCoords[6*topTriangle+2] = 0.5+0.5*cos((6.283*(i+1))/_numberDivisions);
      texCoords[6*topTriangle+3] = 0.5+0.5*sin((6.283*(i+1))/_numberDivisions);
      texCoords[6*topTriangle+4] = 0.5;
      texCoords[6*topTriangle+5] = 0.5;

      texCoords[6*bottomTriangle  ] = 0.5+0.5*cos((6.283*i)/_numberDivisions);
      texCoords[6*bottomTriangle+1] = 0.5+0.5*sin((6.283*i)/_numberDivisions);
      texCoords[6*bottomTriangle+2] = 0.5;
      texCoords[6*bottomTriangle+3] = 0.5;
      texCoords[6*bottomTriangle+4] =
        0.5+0.5*cos((6.283*(i+1))/_numberDivisions);
      texCoords[6*bottomTriangle+5] =
        0.5+0.5*sin((6.283*(i+1))/_numberDivisions);
      
    }
  }

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  ar_attachMesh(parent, name,
                numberPoints, pointPositions,
                numberTriangles*indicesPerTri, triangleVertices,
		numberTriangles*normalsPerTri, normals,
		numberTriangles*texcoordsPerTri, texCoords,
		DG_TRIANGLES, numberTriangles);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] texCoords;
  delete [] normals;
}

/////////  PYRAMID  /////////////////////////////////////////////////////

void arPyramidMesh::attachMesh(arGraphicsNode* parent, 
                               const string& name){
  const int numberPoints = 5;
  const int numberTriangles = 6;
  float pointPositions[3*numberPoints] = {
    0,0,0.5, -0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,0.5,-0.5, -0.5,0.5,-0.5};
  int triangleVertices[3*numberTriangles] 
    = {1,3,2, 1,4,3, 1,2,0, 2,3,0, 3,4,0, 4,1,0};
  arVector3 faceNormals[4]; // normals for the triangular sides of the pyramid
  faceNormals[0] = arVector3(0.5,-0.5,-1)*arVector3(-0.5,-0.5,-1);
  faceNormals[0].normalize();
  faceNormals[1] = arVector3(0.5,0.5,-1)*arVector3(0.5,-0.5,-1);
  faceNormals[1].normalize();
  faceNormals[2] = arVector3(-0.5,0.5,-1)*arVector3(0.5,0.5,-1);
  faceNormals[2].normalize();
  faceNormals[3] = arVector3(-0.5,-0.5,-1)*arVector3(-0.5,0.5,-1);
  faceNormals[3].normalize();
  float normals[9*numberTriangles] 
    = {0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
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
  float texCoords[6*numberTriangles] = {0,0,1,1,1,0, 0,0,0,1,1,1,
                         0,0,1,0,0.5,0.5, 1,0,1,1,0.5,0.5,
                         1,1,0,1,0.5,0.5, 0,1,0,0,0.5,0.5};

  
  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  ar_attachMesh(parent, name,
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

// Sometimes it is convenient to be able to only use some of the vertical
// bands on the sphere (which allows for a way to peek inside)
void arSphereMesh::setSectionSkip(int sectionSkip){
  _sectionSkip = sectionSkip;
}

void arSphereMesh::attachMesh(arGraphicsNode* parent, const string& name){
  // we may change the number of triangles later in the function because
  // of decimation
  int numberTriangles = 2*_numberDivisions*_numberDivisions;
  const int numberPoints = (_numberDivisions+1)*_numberDivisions;

  float* pointPositions = new float[3*numberPoints];
  int* triangleVertices = new int[3*numberTriangles];
  float* texCoords = new float[3*texcoordsPerTri*numberTriangles];
  float* normals = new float[3*normalsPerTri*numberTriangles];

  // populate the points array
  int i=0, j=0;
  float z=0., radius=0.;
  int iPoint = 0;
  for (j=0; j<_numberDivisions+1; j++){
    for (i=0; i<_numberDivisions; i++){
      z = sin(3.141592/2 - (3.141592*j)/_numberDivisions);
      radius = (z*z>=1) ? 0. : sqrt(1-z*z);
      arVector3 location(
        radius*cos( (2*3.141592*i)/_numberDivisions),
        radius*sin( (2*3.141592*i)/_numberDivisions),
        z);
      pointPositions[iPoint++] = location[0];
      pointPositions[iPoint++] = location[1];
      pointPositions[iPoint++] = location[2];
    }
  }

  // populate the triangles arrays
  int iTriangle = 0;
  for (j=0; j<_numberDivisions; j++){
    for (i=0; i<_numberDivisions; i++, iTriangle++){
      const int k = (i+1) % _numberDivisions;
      triangleVertices[6*iTriangle  ] = j    *_numberDivisions + i;
      triangleVertices[6*iTriangle+1] = (j+1)*_numberDivisions + i;
      triangleVertices[6*iTriangle+2] = (j+1)*_numberDivisions + k;
      triangleVertices[6*iTriangle+3] = j    *_numberDivisions + i;
      triangleVertices[6*iTriangle+4] = (j+1)*_numberDivisions + k;
      triangleVertices[6*iTriangle+5] = j    *_numberDivisions + k;

      z = sin(3.141592/2 - (3.141592*j)/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle   ] = radius*cos((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+1 ] = radius*sin((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+2 ] = z;
      z = sin(3.141592/2 - (3.141592*(j+1))/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle+3 ] = radius*cos((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+4 ] = radius*sin((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+5 ] = z;
      z = sin(3.141592/2 - (3.141592*(j+1))/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle+6 ] = radius*cos((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+7 ] = radius*sin((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+8 ] = z;
      z = sin(3.141592/2 - (3.141592*j)/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle+9 ] = radius*cos((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+10] = radius*sin((2*3.141592*i)/_numberDivisions);
      normals[18*iTriangle+11] = z;
      z = sin(3.141592/2 - (3.141592*(j+1))/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle+12] = radius*cos((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+13] = radius*sin((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+14] = z;
      z = sin(3.141592/2 - (3.141592*j)/_numberDivisions);
      radius = sqrt(1-z*z);
      normals[18*iTriangle+15] = radius*cos((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+16] = radius*sin((2*3.141592*k)/_numberDivisions);
      normals[18*iTriangle+17] = z;

      texCoords[12*iTriangle   ] = float(i)  /_numberDivisions;
      texCoords[12*iTriangle+1 ] = float(j)  /_numberDivisions;
      texCoords[12*iTriangle+2 ] = float(i)  /_numberDivisions;
      texCoords[12*iTriangle+3 ] = float(j+1)/_numberDivisions;
      texCoords[12*iTriangle+4 ] = float(i+1)/_numberDivisions;
      texCoords[12*iTriangle+5 ] = float(j+1)/_numberDivisions;
      texCoords[12*iTriangle+6 ] = float(i)  /_numberDivisions;
      texCoords[12*iTriangle+7 ] = float(j)  /_numberDivisions;
      texCoords[12*iTriangle+8 ] = float(i+1)/_numberDivisions;
      texCoords[12*iTriangle+9 ] = float(j+1)/_numberDivisions;
      texCoords[12*iTriangle+10] = float(i+1)/_numberDivisions;
      texCoords[12*iTriangle+11] = float(j)  /_numberDivisions;
    }
  }

  // Go ahead and decimate the sphere triangles, if requested.
  int nearVertex = 0;
  int nearNormal = 0;
  int nearCoord = 0;
  for (j=0; j<_numberDivisions; j++){
    for (i=0; i<_numberDivisions; i+=_sectionSkip){
      const int whichTriangle = 2*(j*_numberDivisions + i);
      int farVertex = 3*whichTriangle;
      int farNormal = 9*whichTriangle;
      int farCoord = 6*whichTriangle;
      int k = 0;
      for (k=0; k<6; k++)
        triangleVertices[nearVertex++] = triangleVertices[farVertex++];
      for (k=0; k<18; k++)
        normals[nearNormal++] = normals[farNormal++];
      for (k=0; k<12; k++)
        texCoords[nearCoord++] = texCoords[farCoord++];
    }
  }

  numberTriangles = 2*_numberDivisions*(_numberDivisions/_sectionSkip);

  // Apply matrix to generated points and normals.
  ar_adjustPoints(_matrix, numberPoints, pointPositions);
  ar_adjustNormals(_matrix, numberTriangles*normalsPerTri, normals);

  ar_attachMesh(parent, name,
                numberPoints, pointPositions,
                numberTriangles*indicesPerTri, triangleVertices,
		numberTriangles*normalsPerTri, normals,
		numberTriangles*texcoordsPerTri, texCoords,
		DG_TRIANGLES, numberTriangles);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] texCoords;
  delete [] normals;
}


/* DEPRECATED! Will return this functionality in another way!
arVector3 arSphereMesh::_spherePoint(int i, int j){
  const float z = sin(3.141592/2 - (3.141592*j)/_numberDivisions);
  const float radius = (z*z>=1.) ? 0. : sqrt(1.-z*z);
  return arVector3(_matrix * arVector3(
                   radius*cos( (2*3.141592*i)/_numberDivisions),
                   radius*sin( (2*3.141592*i)/_numberDivisions),
                   z));
}

void arSphereMesh::draw(){
  glPushMatrix();
  glMultMatrixf(_matrix.v);
  int iTriangle = 0;
  glBegin(GL_TRIANGLES);
  for (int j=0; j<_numberDivisions; j++){
    for (int i=0; i<_numberDivisions; i++, iTriangle++){
      const int k = (i+1) % _numberDivisions;
      // SGI compiler crashes if we don't use temp in the following way
      // previously, I was doing spherePoint(i,j).v instead of temp.v
      arVector3 temp;
      glTexCoord2d(float(i)/_numberDivisions,float(j)/_numberDivisions);
      temp = _spherePoint(i,j);
      glVertex3fv(temp.v);
      glTexCoord2d(float(i)/_numberDivisions,float(j+1)/_numberDivisions);
      temp = _spherePoint(i,j+1);
      glVertex3fv(temp.v);
      glTexCoord2d(float(i+1)/_numberDivisions,float(j+1)/_numberDivisions);
      temp = _spherePoint(k,j+1);
      glVertex3fv(temp.v);
      glTexCoord2d(float(i)/_numberDivisions,float(j)/_numberDivisions);
      temp = _spherePoint(i,j);
      glVertex3fv(temp.v);
      glTexCoord2d(float(i+1)/_numberDivisions,float(j+1)/_numberDivisions);
      temp = _spherePoint(k,j+1);
      glVertex3fv(temp.v);
      glTexCoord2d(float(i+1)/_numberDivisions,float(j)/_numberDivisions);
      temp = _spherePoint(k,j);
      glVertex3fv(temp.v);
    }
  }
  glEnd();
  glPopMatrix();
}
*/

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

inline int arTorusMesh::_modAdd(int base, int x, int y){
  if (x+y<0)
    return x+y+base;
  if (x+y>=base)
    return x+y-base;
  return x+y;
}

void arTorusMesh::attachMesh(arGraphicsNode* parent, const string& name){

  ar_adjustPoints(_matrix, _numberPoints, _pointPositions);
  ar_adjustNormals(_matrix, _numberTriangles*normalsPerTri, _surfaceNormals);

  ar_attachMesh(parent, name,
                _numberPoints, _pointPositions,
                _numberTriangles*indicesPerTri, _triangleVertices,
		_numberTriangles*normalsPerTri, _surfaceNormals,
		_numberTriangles*texcoordsPerTri, _textureCoordinates,
		DG_TRIANGLES, _numberTriangles);
}

void arTorusMesh::_reset(int numberBigAroundQuads, int numberSmallAroundQuads,
                         float bigRadius, float smallRadius){
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

  // should build tables of cos(6.283*k/numberXXX) and sin(ditto)

  for (int i=0; i<numberBigAroundQuads;i++){
    for (int j=0; j<numberSmallAroundQuads; j++){
      // big around angle
      const float theta = (6.283 * i)/numberBigAroundQuads;
      // small around angle
      const float phi = (6.283 * j)/numberSmallAroundQuads;
      const int whichPoint = i*numberSmallAroundQuads+j;
      _pointPositions[3*whichPoint] =
        bigRadius*cos(theta) + smallRadius*cos(theta)*cos(phi);
      _pointPositions[3*whichPoint+1] =
        bigRadius*sin(theta) + smallRadius*sin(theta)*cos(phi);
      _pointPositions[3*whichPoint+2] = smallRadius*sin(phi);

      _triangleVertices[6*whichPoint] = i*numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+1] = _modAdd(numberBigAroundQuads,i,1)
                                   *numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+2] = i*numberSmallAroundQuads
                                   +_modAdd(numberSmallAroundQuads,j,1);
      _triangleVertices[6*whichPoint+3] = _modAdd(numberBigAroundQuads,i,1)
                                   *numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+4] = _modAdd(numberBigAroundQuads,i,1)
                                   *numberSmallAroundQuads
                                   +_modAdd(numberSmallAroundQuads,j,1);
      _triangleVertices[6*whichPoint+5]=i*numberSmallAroundQuads
                                        +_modAdd(numberSmallAroundQuads,j,1);

      // each triangle vertex has a defined normal
      _surfaceNormals[18*whichPoint]
        = cos((6.283*i)/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+1]
	= sin((6.283*i)/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+2]
	= sin((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+3]
	= cos((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+4]
        = sin((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+5]
	= sin((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+6]
	= cos((6.283*i)/numberBigAroundQuads)
        * cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+7]
        = sin((6.283*i)/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+8]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j,1))
              /numberSmallAroundQuads);

      _surfaceNormals[18*whichPoint+9]
        = cos((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+10]
	= sin((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+11]
	= sin((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+12]
	= cos((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+13]
        = sin((6.283*_modAdd(numberBigAroundQuads,i,1))/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+14]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j,1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+15]
	= cos((6.283*i)/numberBigAroundQuads)
        * cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+16]
        = sin((6.283*i)/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j,1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+17]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j,1))
              /numberSmallAroundQuads);

      _textureCoordinates[12*whichPoint   ]=float(i)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+1 ]=float(j)/numberSmallAroundQuads;
      _textureCoordinates[12*whichPoint+2 ]=float(i+1)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+3 ]=float(j)/numberSmallAroundQuads;
      _textureCoordinates[12*whichPoint+4 ]=float(i)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+5 ]=float(j+1)/numberSmallAroundQuads;
      _textureCoordinates[12*whichPoint+6 ]=float(i+1)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+7 ]=float(j)/numberSmallAroundQuads;
      _textureCoordinates[12*whichPoint+8 ]=float(i+1)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+9 ]=float(j+1)/numberSmallAroundQuads;
      _textureCoordinates[12*whichPoint+10]= float(i)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+11]=float(j+1)/numberSmallAroundQuads;
    }
  }
}
