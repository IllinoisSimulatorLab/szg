//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_COLOR4_NODE_H
#define AR_COLOR4_NODE_H

#include "arGraphicsArrayNode.h"
#include "arGraphicsCalling.h"

#include <vector>

class SZG_CALL arColor4Node:public arGraphicsArrayNode{
 public:
  arColor4Node();
  ~arColor4Node() {}

  virtual void initialize(arDatabase* database);

  const float* getColor4(int& number);
  void setColor4(int number, float* color4, int* IDs = NULL);
  vector<arVector4> getColor4();
  void setColor4(vector<arVector4>& color4);
  void setColor4(vector<arVector4>& color4,
                 vector<int>& IDs);
};

#endif
