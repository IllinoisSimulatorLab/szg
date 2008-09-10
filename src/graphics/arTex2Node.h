//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEX2_NODE_H
#define AR_TEX2_NODE_H

#include "arGraphicsArrayNode.h"
#include "arGraphicsCalling.h"

class SZG_CALL arTex2Node:public arGraphicsArrayNode{
 public:
  arTex2Node();
  virtual ~arTex2Node() {}

  virtual void initialize(arDatabase* database);

  const float* getTex2(int& number);
  void setTex2(int number, float* tex2, int* IDs = NULL);
  vector<arVector2> getTex2();
  void setTex2(vector<arVector2>& tex2);
  void setTex2(vector<arVector2>& tex2,
               vector<int>& IDs);
};

#endif
