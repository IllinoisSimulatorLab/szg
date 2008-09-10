//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "HypDodecahedron.h"

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

HypDodecahedron::HypDodecahedron() :
  name(string("")),
  changedColor(0),
  changedPoints(0),
  whichDod(0),
  dodScale(1),
  pointNodeID(-1),
  lineNodeID(-1)
{
  lSETi(color, 0, 0.1,0.9,0.1);
  for (int ii=0;ii<4;ii++)
    for(int jj=0;jj<4;jj++)
      transformation[ii+4*jj] = (ii == jj) ? 1 : 0;
}

HypDodecahedron::HypDodecahedron(ARfloat color[3], int whichOne, double scale) :
  name(string("")),
  changedColor(0),
  changedPoints(0),
  whichDod(whichOne),
  dodScale(scale),
  pointNodeID(-1),
  lineNodeID(-1)
{
  lCPYi(this->color, 0, color);
  for(int ii=0;ii<4;ii++)
    for(int jj=0;jj<4;jj++)
      transformation[ii+4*jj] = (ii == jj) ? 1 : 0;
}
  
HypDodecahedron::~HypDodecahedron(){
  dgErase(name + " points");
}

void HypDodecahedron::attachMesh(const string& hypDodName,
  const string& parentName)
{
  int ii;
  name=hypDodName;

  //  project to 3d points
  float points3D[N_VERTS*3];
  
  makeDodecahedron(points3D, lineEndPoints);
  
  //  attach 3d points
  if(pointNodeID == -1){
    pointNodeID=
      dgPoints(name+" points", parentName, N_VERTS, points3D);
  }
  else{
    dgPoints(pointNodeID, N_VERTS, points3D);
  }

  //  attach colored lines
  float lineColors[N_EDGES*8];

  for(ii=0; ii<N_EDGES; ii++) {
    lineColors[8*ii] = color[0];
    lineColors[8*ii+1] = color[1];
    lineColors[8*ii+2] = color[2];
    lineColors[8*ii+3] = 1;
    lineColors[8*ii+4] = color[0];
    lineColors[8*ii+5] = color[1];
    lineColors[8*ii+6] = color[2];
    lineColors[8*ii+7] = 1;
  }

  if(lineNodeID==-1){
    dgColor4(name+" colors", name+" points", 2*N_EDGES, lineColors);
    dgIndex(name+" index", name+" colors", 2*N_EDGES, lineEndPoints);
    lineNodeID = dgDrawable(name+" lines", name+" index", DG_LINES, N_EDGES);
  }
  else{
    dgDrawable(lineNodeID, DG_LINES, N_EDGES);
  }
}

void HypDodecahedron::makeDodecahedron(float *destPoints, int *destLines)
{

//set up 4d points
  int ii,jj;
  double transform[16];
  //copy transformation matrix for this dodecahedron
  for(ii=0;ii<4;ii++) { transform[ 0+ii]=hgrp[whichDod][ii][0]; }
  for(ii=0;ii<4;ii++) { transform[ 4+ii]=hgrp[whichDod][ii][1]; }
  for(ii=0;ii<4;ii++) { transform[ 8+ii]=hgrp[whichDod][ii][2]; }
  for(ii=0;ii<4;ii++) { transform[12+ii]=hgrp[whichDod][ii][3]; }

  double dodVertex[4];
  for(ii=0;ii<N_VERTS;ii++)
  {
    for(jj=0;jj<3;jj++) dodVertex[jj]=dodScale*ddvert[ii][jj];
    dodVertex[3]=1;
    multiplyVectorByMatrix(dodVertex, transform, dodPoints[ii]);
  }

//project to 3d points
  project4DTo3D(destPoints);
  
//set up endpoints
  ii=jj=0;
  for(jj=0;jj<N_EDGES;jj++,ii++)
  {
    if(ddwire[ii+1]==STOPSTRIP)
    {
      ii+=2;
    }
    else if(ddwire[ii+1]==ENDARRAY)
    {
      break;
    }
    destLines[2*jj+0]=ddwire[ii+0];
    destLines[2*jj+1]=ddwire[ii+1];
  }
}

void HypDodecahedron::changeColor(ARfloat newColor[3])
{
  changedColor=1;

  for(int ii=0; ii<3; ii++)
  {
    color[ii]=newColor[ii];
  }
}


//precondition - attachMesh already called
void HypDodecahedron::updateAttached()
{
  if(changedPoints)
  {
    changedPoints=0;

    float pointPositions[N_VERTS*3];
    project4DTo3D(pointPositions);

    dgPoints(pointNodeID, N_VERTS, pointPositions);
  }
}

void HypDodecahedron::updateUnattached(float destPoints[N_VERTS*3])
{
//cout<<"updateUnattached\n";
    project4DTo3D(destPoints);
}

void HypDodecahedron::updateUnattached(float destPoints[N_VERTS*3],
                                       double* theMatrix){
//cout<<"updateUnattached\n";
    project4DTo3D(destPoints,theMatrix);
}
  

//rotates about origin
void HypDodecahedron::hypRotate(char axis, double radians)
{
  changedPoints=1;

//  transformation=ar_rotationMatrix(axis, radians)*transformation;
  double rotationMatrix[16]=
    {1.0, 0.0, 0.0, 0.0,
     0.0, 1.0, 0.0, 0.0,
     0.0, 0.0, 1.0, 0.0,
     0.0, 0.0, 0.0, 1.0};

  switch (axis)
  {
  case 'x':
    rotationMatrix[5]=cos(radians);
    rotationMatrix[6]=sin(radians);
    rotationMatrix[9]=-sin(radians);
    rotationMatrix[10]=cos(radians);
    break;
  case 'y':
    rotationMatrix[0]=cos(radians);
    rotationMatrix[2]=sin(radians);
    rotationMatrix[8]=-sin(radians);
    rotationMatrix[10]=cos(radians);
    break;
  case 'z':
    rotationMatrix[0]=cos(radians);
    rotationMatrix[1]=sin(radians);
    rotationMatrix[4]=-sin(radians);
    rotationMatrix[5]=cos(radians);
    break;
  }
  
  multiplyMatrixByMatrix(transformation, rotationMatrix, transformation);

}

//only use z
void HypDodecahedron::hypTranslate(double speed)
{
  changedPoints=1;
  double translationMatrix[16]=
    {1.0, 0.0, 0.0, 0.0,
     0.0, 1.0, 0.0, 0.0,
     0.0, 0.0, 1.0, 0.0,
     0.0, 0.0, 0.0, 1.0};
  translationMatrix[10]=translationMatrix[15]=cosh(speed);
  translationMatrix[11]=translationMatrix[14]=sinh(speed);

  multiplyMatrixByMatrix(translationMatrix, transformation, transformation);
}

//multiply with the vector on the **LEFT**
void HypDodecahedron::multiplyVectorByMatrix
     (double vector[4], double matrix[16], double dest[4])
{
  int ii, jj;
  double tempVector[4];
  for(ii=0;ii<4;ii++) tempVector[ii]=vector[ii];
  for(ii=0;ii<4;ii++)
  {
    dest[ii]=0;
    for(jj=0;jj<4;jj++)
    {
      dest[ii]+=matrix[ii*4+jj]*tempVector[jj];
    }
  }
}

//multiply with the vector on the **RIGHT**
void HypDodecahedron::multiplyMatrixByVector
     (double matrix[16], double vector[4], double dest[4])
{
  //transpose matrix and then call the left multiplication
  double transposedMatrix[16];

  for(int ii=0;ii<4;ii++)
  {
    transposedMatrix[0+4*ii]=matrix[ii];
    transposedMatrix[1+4*ii]=matrix[ii+4];
    transposedMatrix[2+4*ii]=matrix[ii+8];
    transposedMatrix[3+4*ii]=matrix[ii+12];
  }

  multiplyVectorByMatrix(vector, transposedMatrix, dest);
}

void HypDodecahedron::multiplyMatrixByMatrix
                      (double left[16], double right[16], double product[16])
{
  int ii, jj;
  double templeft[16], tempright[16];
  for(ii=0;ii<16;ii++)
  {
    templeft[ii]=left[ii];
    tempright[ii]=right[ii];
  }
  for(ii=0;ii<4;ii++) for(jj=0;jj<4;jj++)
  {
    product[ii+4*jj]=templeft[ii]*tempright[4*jj] + 
                     templeft[ii+4]*tempright[4*jj+1] +
                     templeft[ii+8]*tempright[4*jj+2] +
                     templeft[ii+12]*tempright[4*jj+3];
  //product[ii*4 + jj] += templeft[ii +4*kk]*tempright[kk + 4*jj];
  }
}
  
void HypDodecahedron::project4DTo3D(float points3D[N_VERTS*3])
{
  double transformedPoint[4];
  for(int ii=0;ii<N_VERTS;ii++)
  {
    //transform
    multiplyMatrixByVector(transformation, dodPoints[ii], transformedPoint);

    //project
    for(int jj=0; jj<3; jj++)
    {
      if(transformedPoint[3]!=0) 
      {
        points3D[ii*3+jj]=transformedPoint[jj]/transformedPoint[3];
      }
      else
      {
        points3D[ii*3+jj]=0;
        cout<<"divide by zero"<<endl;
      }
    }
  }
}

void HypDodecahedron::project4DTo3D(float points3D[N_VERTS*3],
                                    double* theMatrix){

  double transformedPoint[4];
  for(int ii=0;ii<N_VERTS;ii++)
  {
    //transform
    multiplyMatrixByVector(theMatrix, dodPoints[ii], transformedPoint);

    //project
    for(int jj=0; jj<3; jj++)
    {
      if(transformedPoint[3]!=0) 
      {
        points3D[ii*3+jj]=transformedPoint[jj]/transformedPoint[3];
      }
      else
      {
        points3D[ii*3+jj]=0;
        cout<<"divide by zero"<<endl;
      }
    }
  }
}
