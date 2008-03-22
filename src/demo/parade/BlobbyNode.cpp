//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "BlobbyNode.h"

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

BlobbyNode::BlobbyNode(){
  blob = NULL;
  transformData=NULL;
  blobTranslateID=blobRotateID=blobScaleID=-1;
}

BlobbyNode::BlobbyNode(const arMatrix4& blobRotation,
           const arMatrix4& blobTranslation,
           const arMatrix4& blobScale, ARfloat blobColor[3])
{
  originalBlobRotation=blobRotation;
  originalBlobTranslation=blobTranslation;
  changedBlobScale=0;
  changedBlobRotation=0;
  changedBlobTranslation=0;
  changedBlobColor=0;
  this->blobScale=blobScale;
  this->blobRotation=blobRotation;
  this->blobTranslation=blobTranslation;

  lCPYi(this->blobColor,0,blobColor);
  blob = NULL;
  transformData=NULL;
  blobTranslateID=blobRotateID=blobScaleID=-1;
}

BlobbyNode::~BlobbyNode(){
  if (blob)
    delete blob;
  if (transformData)
    delete transformData;
}

void BlobbyNode::scaleBlob(float xscale, float yscale, float zscale)
{
  blobScale = ar_scaleMatrix(xscale,yscale,zscale) * blobScale;
  changedBlobScale=1;
}

void BlobbyNode::changeColor(ARfloat color[3])
{
  if(blob)
  {
    blob->changeColor(color);
    changedBlobColor=1;
  }
}

void BlobbyNode::translateBlob(ARfloat xx, ARfloat yy, ARfloat zz)
{
  blobTranslation=ar_translationMatrix(xx,yy,zz)*blobTranslation;
  changedBlobTranslation=1;
}

void BlobbyNode::rotateBlob(char axis, float angle)
{
  blobRotation=ar_rotationMatrix(axis, angle)*blobRotation;
  changedBlobRotation=1;
}

void BlobbyNode::update()
{
  if(changedBlobScale)
  {
    changedBlobScale=0;
    dgTransform(blobScaleID, blobScale);
  }
  if(changedBlobRotation)
  {
    changedBlobRotation=0;
    dgTransform(blobRotateID, blobRotation);
  }
  if(changedBlobTranslation)
  {
    changedBlobTranslation=0;
    dgTransform(blobTranslateID, blobTranslation);
  }

  if(changedBlobColor)
  {
    changedBlobColor=0;
    blob->update();
  }
}

void BlobbyNode::attachMesh(const string& name, BlobbyNode *parent)
{
  attachMesh(name, parent->nodeName + " blob rotation");
}

void BlobbyNode::attachMesh(const string& name, const string& parentName)
{
  nodeName=name;

  if(blobTranslateID<0)
  {
    blobTranslateID =
      dgTransform(name+ " blob translation", parentName, blobTranslation);
  }
  else
  {
      (void)dgTransform(blobTranslateID, blobTranslation);
  }

  if(blobRotateID<0)
  {
    blobRotateID =
      dgTransform(name+" blob rotation", name+" blob translation", blobRotation);
  }
  else
  {
    (void)dgTransform(blobRotateID, blobRotation);
  }

  if(blobScaleID<0)
  {
    blobScaleID =
      dgTransform(name+" blob scale", name+" blob rotation", blobScale);
  }
  else
  {
    (void)dgTransform(blobScaleID, blobScale);
  }

  if (!blob) {
    blob = new ColIcosahedronMesh(blobColor);
  }
  blob->attachMesh(name+" blob", name+" blob scale");
}

void BlobbyNode::resetNodeRotation()
{
  blobRotation = originalBlobRotation;
}

void BlobbyNode::resetNodeTranslation()
{
  blobTranslation = originalBlobTranslation;
}
