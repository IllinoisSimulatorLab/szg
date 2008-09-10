//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_POINTS_NODE_H
#define AR_POINTS_NODE_H

#include "arGraphicsArrayNode.h"
#include "arGraphicsCalling.h"

#include <vector>

class SZG_CALL arPointsNode:public arGraphicsArrayNode{
// Set of OpenGL points.
 public:
  arPointsNode();
  virtual ~arPointsNode() {}

  virtual void initialize(arDatabase* database);

  const float* getPoints(int& number);
  void setPoints(int number, float* points, int* IDs = NULL);
  vector<arVector3> getPoints();
  void setPoints(vector<arVector3>& points);
  void setPoints(vector<arVector3>& points,
                 vector<int>& IDs);
};

#endif
