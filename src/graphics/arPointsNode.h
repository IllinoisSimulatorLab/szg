//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_POINTS_NODE_H
#define AR_POINTS_NODE_H

#include "arGraphicsArrayNode.h"

class SZG_CALL arPointsNode:public arGraphicsArrayNode{
/// Set of (OpenGL) points.
 public:
  arPointsNode(arGraphicsDatabase*);
  ~arPointsNode(){}

  float* getPoints(int& number);
  void   setPoints(int number, float* points, int* IDs = NULL);
};

#endif
