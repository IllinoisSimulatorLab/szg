//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

#ifndef BLOBBY_NODE_H
#define BLOBBY_NODE_H

#include "arGraphicsAPI.h"
#include "arMath.h"
#include "ColIcosahedronMesh.h"
#include <string>
using namespace std;

class BlobbyNode{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  BlobbyNode();
  BlobbyNode(const arMatrix4& blobRotation, const arMatrix4& blobTranslation, 
             const arMatrix4& blobScale, ARfloat blobColor[3]);
  ~BlobbyNode();

  void attachMesh(const string&, const string&);
  void attachMesh(const string&, BlobbyNode*);

  void scaleBlob(float, float, float);

  void changeColor(ARfloat color[3]);
  void rotateBlob(char, float);
  void translateBlob(ARfloat, ARfloat, ARfloat);
  void update();

  void resetNodeRotation();
  void resetNodeTranslation();

 protected:

  ColIcosahedronMesh *blob;
  arMatrix4 blobRotation; 
  arMatrix4 blobTranslation, blobScale;
  ARfloat blobColor[3];
  arStructuredData *transformData;

  arMatrix4 originalBlobRotation, originalBlobTranslation;

  int changedJointScale, changedBlobScale;
  int changedBlobRotation, changedBlobTranslation, changedJointColor;
  int changedBlobColor;

  int blobScaleID, blobRotateID, blobTranslateID;

  string nodeName;
};

#endif
