//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "ColIcosahedronMesh.h"

#ifndef lSETi
#define lSETi(u, i, x,y,z) {u[i+0]=x;u[i+1]=y;u[i+2]=z;}
#endif
#ifndef lCPYi
#define lCPYi(u,i,v) {u[i+0]=v[0];u[i+1]=v[1];u[i+2]=v[2];}
#endif
#ifndef lCROSSi
#define lCROSSi(u,a,v,b,t,c) {u[a+0]=v[b+1]*t[c+2]-v[b+2]*t[c+1];\
                              u[a+1]=v[b+2]*t[c+0]-v[b+0]*t[c+2];\
                              u[a+2]=v[b+0]*t[c+1]-v[b+1]*t[c+0];}
#endif

#ifndef lDOTi
#define lDOTi(u,a,v,b) (u[a+0]*v[b+0]+u[a+1]*v[b+1]+u[a+2]*v[a+2])
#endif

#ifndef lSUBi
#define lSUBi(u,a,v,b,t,c){u[a+0]=v[b+0]-t[c+0];\
                           u[a+1]=v[b+1]-t[c+1];\
                           u[a+2]=v[b+2]-t[c+2];}
#endif

#ifndef lLENGTHi
#define lLENGTHi(u,i) (sqrt(lDOTi(u,i,u,i)))
#endif

ColIcosahedronMesh::ColIcosahedronMesh(){
  pointsID=trianglesID=normalsID=colorsID=-1;
  changedColor=0;
  name = string("");

  for (int i=0; i<60; i++){
    colors[4*i] = 1;
    colors[4*i+1] = 1;
    colors[4*i+2] = 1;
    colors[4*i+3] = 1;
  }
}

ColIcosahedronMesh::ColIcosahedronMesh(ARfloat color[3])
{
  pointsID=trianglesID=normalsID=colorsID=-1;
  changedColor=0;
  name = string("");
  for (int i=0;i<60;i++){
    colors[4*i] = color[0];
    colors[4*i+1] = color[1];
    colors[4*i+2] = color[2];
    colors[4*i+3] = 1;
  }
}

ColIcosahedronMesh::~ColIcosahedronMesh(){
  dgErase(name+" points");
}

void ColIcosahedronMesh::attachMesh(const string& icName,
  const string& parentName)
{
  name = icName;
  //numbers we calculated ahead of time
  #define AA 0.5
  #define BB 0.30901699
  ARfloat pointPositions[36] = {  0, BB,-AA,
                                 BB, AA,  0,
                                -BB, AA,  0,
                                  0, BB, AA,
                                  0,-BB, AA,
                                -AA,  0, BB,
                                 AA,  0, BB,
                                  0,-BB,-AA,
                                 AA,  0,-BB,
                                -AA,  0,-BB,
                                 BB,-AA,  0,
                                -BB,-AA,  0};
  ARint triangleVertices[60];
  lSETi(triangleVertices, 0, 0,1,2);
  lSETi(triangleVertices, 3, 3,2,1);
  lSETi(triangleVertices, 6, 3,4,5);
  lSETi(triangleVertices, 9, 3,6,4);
  lSETi(triangleVertices,12, 0,7,8);
  lSETi(triangleVertices,15, 0,9,7);
  lSETi(triangleVertices,18, 4,10,11);
  lSETi(triangleVertices,21, 7,11,10);
  lSETi(triangleVertices,24, 2,5,9);
  lSETi(triangleVertices,27, 11,9,5);
  lSETi(triangleVertices,30, 1,8,6);
  lSETi(triangleVertices,33, 10,6,8);
  lSETi(triangleVertices,36, 3,5,2);
  lSETi(triangleVertices,39, 3,1,6);
  lSETi(triangleVertices,42, 0,2,9);
  lSETi(triangleVertices,45, 0,8,1);
  lSETi(triangleVertices,48, 7,9,11);
  lSETi(triangleVertices,51, 7,10,8);
  lSETi(triangleVertices,54, 4,11,5);
  lSETi(triangleVertices,57, 4,6,10);

  ARfloat triNormals[180];
  makeNormals(pointPositions,36,triangleVertices,60,triNormals);

  pointsID = dgPoints(icName+" points", parentName, 12, pointPositions);
  dgIndex(icName+" index", icName+" points", 60, triangleVertices);
  dgNormal3(icName+" normals", icName+" index", 60, triNormals);
  colorsID = dgColor4(icName+" colors", icName+" normals", 60, colors);
  dgDrawable(icName+" triangles", icName+" colors", DG_TRIANGLES, 20);

  /*if(pointsID == -1)
  {
    pointsID = dgPoints(icName+" points", parentName,
                        12, pointIDs, pointPositions);
  }
  else
  {
    (void)dgPoints(pointsID, 12, pointIDs, pointPositions);
  }

  if(trianglesID == -1)
  {
    trianglesID = dgTriangles(icName+ " triangles", icName+" points",
                              20, triangleIDs, triangleVertices);
  }
  else
  {
    (void)dgTriangles(trianglesID, 20, triangleIDs, triangleVertices);
  }

  if(normalsID == -1)
  {
    normalsID=dgNormals(icName+" normals", icName+" triangles",
                        20, triangleIDs, triNormals);
  }
  else
  {
    (void)dgNormals(normalsID, 20, triangleIDs, triNormals);
  }

  if(colorsID == -1)
  {
    colorsID=dgColTriangles(icName+" colors", icName+" normals",
                      20, triangleIDs, colors);
  }
  else
  {
    (void)dgColTriangles(colorsID, 20, triangleIDs, colors);
    }*/
}

void ColIcosahedronMesh::changeColor(ARfloat newColor[3])
{
  for (int i=0; i<60; i++){
    colors[4*i] = newColor[0];
    colors[4*i+1] = newColor[1];
    colors[4*i+2] = newColor[2];
    colors[4*i+3] = 1;
  }
  changedColor = 1;
}

//precondition - attachMesh already called
void ColIcosahedronMesh::update()
{
  if (!changedColor){
    return;
  }
  changedColor = 0;
  dgColor4(colorsID, 60, colors);
}

//works for any origin-centered convex polyhedron
void ColIcosahedronMesh::makeNormals(ARfloat* points, int /*numPoints*/,
                                       ARint* triangleVertices,
                                       int numTriangleVertices,
                                       ARfloat* returnedNormals)
{
  ARfloat midpoint[3], side0[3],side1[3],norm[3], normlength;

  for (int ii=0; ii<numTriangleVertices; ii+=3)
  {
    midpoint[0]=(points[triangleVertices[ii+0]*3+0]+
                 points[triangleVertices[ii+1]*3+0]+
                 points[triangleVertices[ii+2]*3+0])/3;
    midpoint[1]=(points[triangleVertices[ii+0]*3+1]+
                 points[triangleVertices[ii+1]*3+1]+
                 points[triangleVertices[ii+2]*3+1])/3;
    midpoint[2]=(points[triangleVertices[ii+0]*3+2]+
                 points[triangleVertices[ii+1]*3+2]+
                 points[triangleVertices[ii+2]*3+2])/3;
    lSUBi(side0,0,points,3*triangleVertices[ii+1],
          points,3*triangleVertices[ii+0]);
    lSUBi(side1,0,points,3*triangleVertices[ii+2],
          points,3*triangleVertices[ii+1]);

    lCROSSi(norm,0,side0,0,side1,0);
    normlength=lLENGTHi(norm,0);

    if (lDOTi(norm, 0, midpoint, 0) < 0)
    {
      // Wrong face out.  Switch vertices to make correct face out.
      int tmp=triangleVertices[ii+1];
      triangleVertices[ii+1]=triangleVertices[ii+2];
      triangleVertices[ii+2]=tmp;

      // Reverse normal.
      norm[0] = -norm[0];
      norm[1] = -norm[1];
      norm[2] = -norm[2];
    }
    returnedNormals[ii*3+0] = returnedNormals[ii*3+3] =
      returnedNormals[ii*3+6] = norm[0] / normlength;
    returnedNormals[ii*3+1] = returnedNormals[ii*3+4] =
      returnedNormals[ii*3+7] = norm[1] / normlength;
    returnedNormals[ii*3+2] = returnedNormals[ii*3+5] =
      returnedNormals[ii*3+8] = norm[2] / normlength;
  }
}
