//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arIndexNode.h"
#include "arGraphicsDatabase.h"

arIndexNode::arIndexNode() : arGraphicsArrayNode(AR_INT, 1) {
  _name = "index_node";
  _typeCode = AR_G_INDEX_NODE;
  _typeString = "index";
  // By default, there is a single index entry with index 0.
  _commandBuffer.grow(1);
  _commandBuffer.v[0] = 0;
}

void arIndexNode::initialize(arDatabase* database) {
  arGraphicsNode::initialize(database);
  arGraphicsArrayNode::initialize(
    _g->AR_INDEX,
    _g->AR_INDEX_ID,
    _g->AR_INDEX_INDEX_IDS,
    _g->AR_INDEX_INDICES,
    _g->AR_INDEX);
}

// Speedy accessor.  Not thread-safe, so call while _nodeLock'd.
const int* arIndexNode::getIndices(int& number) {
  number = _numElements();
  return (int*)_commandBuffer.v;
}

// Slow, thread-safe, Python-compatible accessor.
vector<int> arIndexNode::getIndices() {
  arGuard _(_nodeLock, "arIndexNode::getIndices");
  const unsigned num = _numElements();
  vector<int> r(num);
  // Bug: assumes that sizeof(int) == sizeof(float).
  int* p = (int*) _commandBuffer.v;
  for (unsigned int i = 0; i < num; i++) {
    r[i] = p[i];
  }
  return r;
}

void arIndexNode::setIndices(int number, int* indices, int* IDs) {
  if (active()) {
    _nodeLock.lock("arIndexNode::setIndices active");
      arStructuredData* r = _dumpData(number, indices, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arIndexNode::setIndices inactive");
    _mergeElements(number, indices, IDs);
  }
}

// Slow, Python-compatible.
void arIndexNode::setIndices(vector<int>& indices) {
  const unsigned num = indices.size();
  int* ptr = new int[num];
  for (unsigned i = 0; i < num; ++i) {
    ptr[i] = indices[i];
  }
  setIndices(num, ptr, NULL);
  delete [] ptr;
}

// Slow, Python-compatible.
void arIndexNode::setIndices(vector<int>& indices, vector<int>& IDs) {
  const unsigned num = min(IDs.size(), indices.size());
  int* ptr = new int[num*2];
  for (unsigned int i = 0; i < num; ++i) {
    ptr[i] = indices[i];
    ptr[i+num] = IDs[i];
  }
  setIndices(num, ptr, ptr+num);
  delete [] ptr;
}
