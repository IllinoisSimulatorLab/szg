//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "BlobbyMan.h"

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

BlobbyMan::BlobbyMan()
{
  float shirtColor[3]={0.7,0.2,0.2};
  initialize(shirtColor);
}

BlobbyMan::BlobbyMan(ARfloat shirtColor[3])
{
  initialize(shirtColor);
}

void BlobbyMan::initialize(ARfloat shirtColor[3])
{
  ARfloat blueColor[3]={0.0,0.2,0.5}, skinColor[3]={0.8,0.3,0.2};
  ARfloat dkBrownColor[3]={0.7,0.2,0.1}, greenColor[3]={0.0,0.5,0.0};   
  arMatrix4 identity;

  blobs[LEFT_FOOT] =  new BlobbyNode(identity,
                                ar_translationMatrix(0,-0.15,-0.05), 
                                     ar_scaleMatrix(0.16,0.38,0.10), 
                                     dkBrownColor);
  blobs[LEFT_ANKLE] =  new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
				     ar_scaleMatrix(0.08),
                                     greenColor);
  blobs[LEFT_TIBIA] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                     ar_scaleMatrix(0.2,0.2,0.85),
                                     blueColor);
  blobs[LEFT_KNEE] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                     ar_scaleMatrix(0.1),
                                     greenColor);
  blobs[LEFT_FEMUR] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                    ar_scaleMatrix(0.282,0.282,0.85),
                                     blueColor);
  blobs[LEFT_HIP] = new BlobbyNode(identity,
                              ar_translationMatrix(+0.178,0.0,-0.08),
                                     ar_scaleMatrix(0.11),
                                     greenColor);
                                     
  blobs[RIGHT_FOOT] =  new BlobbyNode(identity,
                                ar_translationMatrix(0,-0.15,-0.05), 
                                     ar_scaleMatrix(0.16,0.38,0.10), 
                                     dkBrownColor);
  blobs[RIGHT_ANKLE] =  new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
				     ar_scaleMatrix(0.08),
                                     greenColor);
  blobs[RIGHT_TIBIA] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                     ar_scaleMatrix(0.2,0.2,0.85),
                                     blueColor);
  blobs[RIGHT_KNEE] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                     ar_scaleMatrix(0.1),
                                     greenColor);
  blobs[RIGHT_FEMUR] = new BlobbyNode(identity,
                                ar_translationMatrix(0.0,0.0,-0.425),
                                     ar_scaleMatrix(0.282,0.282,0.85),
                                     blueColor);
  blobs[RIGHT_HIP] = new BlobbyNode(identity,
                              ar_translationMatrix(-0.178,0.0,-0.08),
                                     ar_scaleMatrix(0.11),
                                     greenColor);
  blobs[PELVIS] = new BlobbyNode(identity,
                                 ar_translationMatrix(0.0,0.0,-0.08),
                                    ar_scaleMatrix(0.55,0.304,0.306),
                                    blueColor);
  blobs[LUMBAR] = new BlobbyNode(identity,
                                 identity,
                                    ar_scaleMatrix(0.5,0.28,0.3),
                                    greenColor);


  blobs[CHEST] = new BlobbyNode(identity,
                             ar_translationMatrix(0.0,0.0,0.49),
                                    ar_scaleMatrix(0.612,0.42,1.0),
                                    shirtColor);
  blobs[WAIST] = new BlobbyNode(    identity,
                                    identity,
                                    ar_scaleMatrix(0.045,0.025,0.025),
                                    greenColor);

  blobs[SCAPULAE] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,0.44),
                                   ar_scaleMatrix(0.9,0.306,0.28),
                                   shirtColor);
  blobs[NECK] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,0.16),
                                   ar_scaleMatrix(0.13,0.13,0.28),
                                   greenColor);
  blobs[HEAD] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,0.33),
                                   ar_scaleMatrix(0.4,0.46,0.6),
                                   skinColor);

  blobs[RIGHT_SHOULDER] = new BlobbyNode(identity,
                            ar_translationMatrix(-0.45,0.0,0.0),
                                   ar_scaleMatrix(0.15),
                                   greenColor);
  blobs[RIGHT_HUMERUS] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,-0.275),
                                   ar_scaleMatrix(0.18,0.18,0.55),
                                   shirtColor);
  blobs[RIGHT_ELBOW] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,-0.275),
                                   ar_scaleMatrix(0.08),
                                   greenColor);
  blobs[RIGHT_ULNA] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,-0.25),
                                   ar_scaleMatrix(0.16,0.16,0.5),
                                   skinColor);
  blobs[RIGHT_WRIST] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,-0.27),
                                   ar_scaleMatrix(0.06),
                                   greenColor);
  blobs[RIGHT_HAND] = new BlobbyNode(identity,
                            ar_translationMatrix(0.0,0.0,-0.136),
                                   ar_scaleMatrix(0.05,0.12,0.279),
                                   skinColor);

  blobs[LEFT_SHOULDER] = new BlobbyNode(identity,
                                   ar_translationMatrix(+0.45,0.0,0.0),
                                   ar_scaleMatrix(0.15),
                                   greenColor);
  blobs[LEFT_HUMERUS] = new BlobbyNode(identity,
                                   ar_translationMatrix(0.0,0.0,-0.275),
                                   ar_scaleMatrix(0.18,0.18,0.55),
                                   shirtColor);
  blobs[LEFT_ELBOW] = new BlobbyNode(identity,
                                   ar_translationMatrix(0.0,0.0,-0.275),
                                   ar_scaleMatrix(0.08),
                                   greenColor);
  blobs[LEFT_ULNA] = new BlobbyNode(identity,
                                   ar_translationMatrix(0.0,0.0,-0.25),
                                   ar_scaleMatrix(0.16,0.16,0.5),
                                   skinColor);
  blobs[LEFT_WRIST] = new BlobbyNode(identity,
                                   ar_translationMatrix(0.0,0.0,-0.27),
                                   ar_scaleMatrix(0.06),
                                   greenColor);
  blobs[LEFT_HAND] = new BlobbyNode(identity,
                                   ar_translationMatrix(0.0,0.0,-0.136),
                                   ar_scaleMatrix(0.05,0.12,0.279),
                                   skinColor);
}

BlobbyMan::~BlobbyMan(){
  for (int ii=0; ii<NUMBER_OF_PARTS; ii++)
    if (blobs[ii])
      delete blobs[ii];
}

void BlobbyMan::attachMesh(const string& name, const string& parentName) 
{
  manTransformID=dgTransform(name+" man transform", parentName, manTransform);

  blobs[LUMBAR]->attachMesh(name+" lumbar", name+" man transform");
  blobs[PELVIS]->attachMesh(name+" pelvis", blobs[LUMBAR]);

  blobs[LEFT_HIP]->attachMesh(name+" left hip", blobs[PELVIS]);
  blobs[LEFT_FEMUR]->attachMesh(name+" left femur", blobs[LEFT_HIP]);
  blobs[LEFT_KNEE]->attachMesh(name+ " left knee", blobs[LEFT_FEMUR]);
  blobs[LEFT_TIBIA]->attachMesh(name+" left tibia", blobs[LEFT_KNEE]);
  blobs[LEFT_ANKLE]->attachMesh(name+" left ankle", blobs[LEFT_TIBIA]);
  blobs[LEFT_FOOT]->attachMesh(name+" left foot", blobs[LEFT_ANKLE]);

  blobs[RIGHT_HIP]->attachMesh(name+" right hip", blobs[PELVIS]);
  blobs[RIGHT_FEMUR]->attachMesh(name+" right femur", blobs[RIGHT_HIP]);
  blobs[RIGHT_KNEE]->attachMesh(name+ " right knee", blobs[RIGHT_FEMUR]);
  blobs[RIGHT_TIBIA]->attachMesh(name+" right tibia", blobs[RIGHT_KNEE]);
  blobs[RIGHT_ANKLE]->attachMesh(name+" right ankle", blobs[RIGHT_TIBIA]);
  blobs[RIGHT_FOOT]->attachMesh(name+" right foot", blobs[RIGHT_ANKLE]);

  blobs[WAIST]->attachMesh(name+" waist", name+" man transform");
  blobs[CHEST]->attachMesh(name+" chest", blobs[WAIST]);
  blobs[SCAPULAE]->attachMesh(name+" scapulae", blobs[CHEST]);
  blobs[NECK]->attachMesh(name+" neck", blobs[SCAPULAE]);
  blobs[HEAD]->attachMesh(name+" head", blobs[NECK]);

  blobs[RIGHT_SHOULDER]->attachMesh(name+" right shoulder", blobs[SCAPULAE]);
  blobs[RIGHT_HUMERUS]->attachMesh(name+" right humerus",blobs[RIGHT_SHOULDER]);
  blobs[RIGHT_ELBOW]->attachMesh(name+" right elbow", blobs[RIGHT_HUMERUS]);
  blobs[RIGHT_ULNA]->attachMesh(name+" right ulna", blobs[RIGHT_ELBOW]);
  blobs[RIGHT_WRIST]->attachMesh(name+" right wrist", blobs[RIGHT_ULNA]);
  blobs[RIGHT_HAND]->attachMesh(name+" right hand", blobs[RIGHT_WRIST]);

  blobs[LEFT_SHOULDER]->attachMesh(name+" left shoulder", blobs[SCAPULAE]);
  blobs[LEFT_HUMERUS]->attachMesh(name+" left humerus", blobs[LEFT_SHOULDER]);
  blobs[LEFT_ELBOW]->attachMesh(name+" left elbow", blobs[LEFT_HUMERUS]);
  blobs[LEFT_ULNA]->attachMesh(name+" left ulna", blobs[LEFT_ELBOW]);
  blobs[LEFT_WRIST]->attachMesh(name+" left wrist", blobs[LEFT_ULNA]);
  blobs[LEFT_HAND]->attachMesh(name+" left hand", blobs[LEFT_WRIST]);
}
 
void BlobbyMan::rotatePart(int partID, char axis, float degrees)
{
  blobs[partID]->rotateBlob(axis, degrees);
  changeFlags[partID]=1;
}

void BlobbyMan::scalePart(int partID, float xx, float yy, float zz)
{
  blobs[partID]->scaleBlob(xx,yy,zz);
  changeFlags[partID]=1;
}

void BlobbyMan::changeColor(int partID, ARfloat color[3])
{
  blobs[partID]->changeColor(color);
  changeFlags[partID]=1;
}

void BlobbyMan::update()
{
  for(int ii=0;ii<NUMBER_OF_PARTS;ii++)
  {
    if(changeFlags[ii])
    {
      blobs[ii]->update();
      changeFlags[ii]=0;
    }
  }
  if(changeFlags[NUMBER_OF_PARTS])
  {// if the man transform has changed
    dgTransform(manTransformID, manTransform);
  }
}

void BlobbyMan::resetMan()
{
  for(int ii=0; ii<NUMBER_OF_PARTS; ii++)
  {
    blobs[ii]->resetNodeRotation();
    blobs[ii]->resetNodeTranslation();
    changeFlags[ii] = 1;
  }
}
