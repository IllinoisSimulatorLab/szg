//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_COLOR4_NODE_H
#define AR_COLOR4_NODE_H

#include "arGraphicsArrayNode.h"

class arColor4Node:public arGraphicsArrayNode{
 public:
  arColor4Node(arGraphicsDatabase*);
  ~arColor4Node(){}

  float* getColor4(int& number);
  void   setColor4(int number, float* color4, int* IDs = NULL);
};

#endif
