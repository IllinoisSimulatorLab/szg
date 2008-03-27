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
  for (int i=0; i<number; i++){
    (m * arVector3(data+i*3)).get(data+i*3);
  }
}

void ar_adjustNormals(const arMatrix4& m, int number, float* data){
  for (int i=0; i<number; i++){
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

  // Precompute table of sin and cos
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
  for (i=0; i<2*_numberDivisions; i++){
    pointIDs[i] = i;
    if (i<_numberDivisions){
      location.set(_bottomRadius*cn[i], _bottomRadius*sn[i], -0.5);
    }
    else{
      location.set(_topRadius*cn[i], pointPositions[3*i+1] = _topRadius*sn[i], 0.5);
    }
    //location = _matrix*location;
    location.get(pointPositions+3*i);
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

    // note i vs j!
    arVector3(cn[i], sn[i], 0).get(normals+18*i);
    arVector3(cn[i], sn[i], 0).get(normals+18*i+3);
    arVector3(cn[j], sn[j], 0).get(normals+18*i+6);
    arVector3(cn[i], sn[i], 0).get(normals+18*i+9);
    arVector3(cn[j], sn[j], 0).get(normals+18*i+12);
    arVector3(cn[j], sn[j], 0).get(normals+18*i+15);

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
    // construct polygons for the ends
    const int topPoint = 2*_numberDivisions;
    const int bottomPoint = topPoint + 1;
    pointIDs[topPoint] = topPoint;
    pointIDs[bottomPoint] = bottomPoint;
    arVector3(0, 0,  0.5).get(pointPositions + 3*topPoint);
    arVector3(0, 0, -0.5).get(pointPositions + 3*bottomPoint);
    for (i=0; i<_numberDivisions; i++){
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

      texCoords[6*topTriangle  ] = 0.5+0.5*cn[i];
      texCoords[6*topTriangle+1] = 0.5+0.5*sn[i];
      texCoords[6*topTriangle+2] = 0.5+0.5*cn[i+1];
      texCoords[6*topTriangle+3] = 0.5+0.5*sin((6.283*(i+1))/_numberDivisions);
      texCoords[6*topTriangle+4] = 0.5;
      texCoords[6*topTriangle+5] = 0.5;

      texCoords[6*bottomTriangle  ] = 0.5+0.5*cn[i];
      texCoords[6*bottomTriangle+1] = 0.5+0.5*sn[i];
      texCoords[6*bottomTriangle+2] = 0.5;
      texCoords[6*bottomTriangle+3] = 0.5;
      texCoords[6*bottomTriangle+4] = 0.5+0.5*cn[i+1];
      texCoords[6*bottomTriangle+5] = 0.5+0.5*sn[i+1];
      
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
    cn[i] = cos( (2*M_PI * i) / _numberDivisions);
    sn[i] = sin( (2*M_PI * i) / _numberDivisions);
    s2[i] = sin(M_PI/2 - (M_PI*i)/_numberDivisions);
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
  for (j=0; j<_numberDivisions; j++){
    for (i=0; i<_numberDivisions; i++, iTriangle++){
      const int k = (i+1) % _numberDivisions;
      triangleVertices[6*iTriangle  ] = j    *_numberDivisions + i;
      triangleVertices[6*iTriangle+1] = (j+1)*_numberDivisions + i;
      triangleVertices[6*iTriangle+2] = (j+1)*_numberDivisions + k;
      triangleVertices[6*iTriangle+3] = j    *_numberDivisions + i;
      triangleVertices[6*iTriangle+4] = (j+1)*_numberDivisions + k;
      triangleVertices[6*iTriangle+5] = j    *_numberDivisions + k;

      float z = s2[j];
      float radius = sqrt(1-z*z);
      normals[18*iTriangle   ] = radius*cn[i];
      normals[18*iTriangle+1 ] = radius*sn[i];
      normals[18*iTriangle+2 ] = z;

      z = s2[j+1];
      radius = sqrt(1-z*z);
      normals[18*iTriangle+3 ] = radius*cn[i];
      normals[18*iTriangle+4 ] = radius*sn[i];
      normals[18*iTriangle+5 ] = z;

      z = s2[j+1];
      radius = sqrt(1-z*z);
      normals[18*iTriangle+6 ] = radius*cn[k];
      normals[18*iTriangle+7 ] = radius*sn[k];
      normals[18*iTriangle+8 ] = z;

      z = s2[j];
      radius = sqrt(1-z*z);
      normals[18*iTriangle+9 ] = radius*cn[i];
      normals[18*iTriangle+10] = radius*sn[i];
      normals[18*iTriangle+11] = z;

      z = s2[j+1];
      radius = sqrt(1-z*z);
      normals[18*iTriangle+12] = radius*cn[k];
      normals[18*iTriangle+13] = radius*sn[k];
      normals[18*iTriangle+14] = z;

      z = s2[j];
      radius = sqrt(1-z*z);
      normals[18*iTriangle+15] = radius*cn[k];
      normals[18*iTriangle+16] = radius*sn[k];
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

  delete [] cn;
  delete [] sn;
  delete [] s2;

  // Decimate the sphere triangles, if requested.
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

inline int arTorusMesh::_modAdd(const int base, const int x){
  return (x < 0) ? x+base : (x >= base) ? x-base : x;
}

bool arTorusMesh::attachMesh(arGraphicsNode* parent, const string& name){
  ar_adjustPoints(_matrix, _numberPoints, _pointPositions);
  ar_adjustNormals(_matrix, _numberTriangles*normalsPerTri, _surfaceNormals);
  return ar_attachMesh(parent, name,
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

  // todo: build tables of cos(6.283*k/numberXXX) and sin(ditto)

  for (int i=0; i<numberBigAroundQuads;i++){
    for (int j=0; j<numberSmallAroundQuads; j++){
      // big around angle
      const float theta = (6.283 * i)/numberBigAroundQuads;
      // small around angle
      const float phi = (6.283 * j)/numberSmallAroundQuads;
      const int whichPoint = i*numberSmallAroundQuads+j;
      _pointPositions[3*whichPoint  ] =
        bigRadius*cos(theta) + smallRadius*cos(theta)*cos(phi);
      _pointPositions[3*whichPoint+1] =
        bigRadius*sin(theta) + smallRadius*sin(theta)*cos(phi);
      _pointPositions[3*whichPoint+2] = smallRadius*sin(phi);

      _triangleVertices[6*whichPoint  ] = i*numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+1] = _modAdd(numberBigAroundQuads,i+1)
                                   *numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+2] = i*numberSmallAroundQuads
                                   +_modAdd(numberSmallAroundQuads,j+1);
      _triangleVertices[6*whichPoint+3] = _modAdd(numberBigAroundQuads,i+1)
                                   *numberSmallAroundQuads+j;
      _triangleVertices[6*whichPoint+4] = _modAdd(numberBigAroundQuads,i+1)
                                   *numberSmallAroundQuads
                                   +_modAdd(numberSmallAroundQuads,j+1);
      _triangleVertices[6*whichPoint+5]=i*numberSmallAroundQuads
                                        +_modAdd(numberSmallAroundQuads,j+1);

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
	= cos((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+4]
        = sin((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+5]
	= sin((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+6]
	= cos((6.283*i)/numberBigAroundQuads)
        * cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+7]
        = sin((6.283*i)/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+8]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j+1))
              /numberSmallAroundQuads);

      _surfaceNormals[18*whichPoint+9]
        = cos((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+10]
	= sin((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+11]
	= sin((6.283*j)/numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+12]
	= cos((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+13]
        = sin((6.283*_modAdd(numberBigAroundQuads,i+1))/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+14]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j+1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+15]
	= cos((6.283*i)/numberBigAroundQuads)
        * cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
              /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+16]
        = sin((6.283*i)/numberBigAroundQuads)
	*cos((6.283*_modAdd(numberSmallAroundQuads,j+1))
             /numberSmallAroundQuads);
      _surfaceNormals[18*whichPoint+17]
	= sin((6.283*_modAdd(numberSmallAroundQuads,j+1))
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
      _textureCoordinates[12*whichPoint+10]=float(i)/numberBigAroundQuads;
      _textureCoordinates[12*whichPoint+11]=float(j+1)/numberSmallAroundQuads;
    }
  }
}
