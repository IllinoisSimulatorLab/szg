//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INDEX_NODE_H
#define AR_INDEX_NODE_H

#include "arGraphicsArrayNode.h"

class SZG_CALL arIndexNode:public arGraphicsArrayNode{
 public:
  arIndexNode(arGraphicsDatabase*);
  ~arIndexNode(){}

  int* getIndices(int& number);
  void setIndices(int number, int* indices, int* IDs = NULL);
};

#endif
