//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

#ifndef BLOBBY_MAN_H
#define BLOBBY_MAN_H

#include "arGraphicsAPI.h"
#include "arMath.h"
#include "BlobbyNode.h"
#include <string>
using namespace std;

enum {
  PELVIS = 0,
  CHEST,
  LEFT_FEMUR,
  LEFT_TIBIA,
  LEFT_FOOT,
  RIGHT_FEMUR,
  RIGHT_TIBIA,
  RIGHT_FOOT,
  LEFT_HUMERUS,
  LEFT_ULNA,
  LEFT_HAND,
  RIGHT_HUMERUS,
  RIGHT_ULNA,
  RIGHT_HAND,
  HEAD,
  LUMBAR,
  WAIST,
  LEFT_HIP,
  LEFT_KNEE,
  LEFT_ANKLE,
  RIGHT_HIP,
  RIGHT_KNEE,
  RIGHT_ANKLE,
  LEFT_SHOULDER,
  LEFT_ELBOW,
  LEFT_WRIST,
  RIGHT_SHOULDER,
  RIGHT_ELBOW,
  RIGHT_WRIST,
  NECK,
  SCAPULAE,
  NUMBER_OF_PARTS
};


class BlobbyMan{
 public:
  BlobbyMan();
  BlobbyMan(ARfloat shirtColor[3]);
  ~BlobbyMan();

  void attachMesh(const string&, const string&);

  void rotatePart(int partID, char axis, float degree);
  void scalePart(int partID, float xx, float yy, float zz);
  void changeColor(int partID, ARfloat color[3]);

  void resetMan();
  void update();

protected:
  BlobbyNode* blobs[NUMBER_OF_PARTS];
  arMatrix4 manTransform;

  int manTransformID;
  int changeFlags[NUMBER_OF_PARTS+1];
  ARfloat colors[NUMBER_OF_PARTS * 3];  
  void initialize(ARfloat shirtColor[3]);

  string manName;
};

#endif
