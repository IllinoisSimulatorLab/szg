//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEX2_NODE_H
#define AR_TEX2_NODE_H

#include "arGraphicsArrayNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arTex2Node:public arGraphicsArrayNode{
 public:
  arTex2Node();
  virtual ~arTex2Node(){}

  virtual void initialize(arDatabase* database);

  float* getTex2(int& number);
  void   setTex2(int number, float* tex2, int* IDs = NULL);
};

#endif
