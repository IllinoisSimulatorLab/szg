//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INDEX_NODE_H
#define AR_INDEX_NODE_H

#include <vector>
#include "arGraphicsArrayNode.h"
#include "arGraphicsCalling.h"

class SZG_CALL arIndexNode:public arGraphicsArrayNode{
 public:
  arIndexNode();
  virtual ~arIndexNode() {}

  virtual void initialize(arDatabase* database);

  const int* getIndices(int& number);
  void setIndices(int number, int* indices, int* IDs = NULL);
  vector<int> getIndices();
  void setIndices(vector<int>& indices);
  void setIndices(vector<int>& indices,
                  vector<int>& IDs);
};

#endif
