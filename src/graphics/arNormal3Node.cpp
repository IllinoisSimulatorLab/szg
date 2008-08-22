//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNormal3Node.h"
#include "arGraphicsDatabase.h"

arNormal3Node::arNormal3Node() : arGraphicsArrayNode(AR_FLOAT, 3) {
  _name = "normal3_node";
  _typeCode = AR_G_NORMAL3_NODE;
  _typeString = "normal3";
  _commandBuffer.grow(1);
}

void arNormal3Node::initialize(arDatabase* database) {
  arGraphicsNode::initialize(database);
  arGraphicsArrayNode::initialize(
    _g->AR_NORMAL3,
    _g->AR_NORMAL3_ID,
    _g->AR_NORMAL3_NORMAL_IDS,
    _g->AR_NORMAL3_NORMALS,
    _g->AR_NORMAL3);
}

// Speedy accessor.  Not thread-safe, so call while _nodeLock'd.
const float* arNormal3Node::getNormal3(int& number) {
  number = _numElements();
  return _commandBuffer.v;
}

// Slow, thread-safe, Python-compatible accessor.
vector<arVector3> arNormal3Node::getNormal3() {
  arGuard _(_nodeLock, "arNormal3Node::getNormal3");
  const unsigned num = _numElements();
  vector<arVector3> r(num);
  for (unsigned i = 0; i < num; ++i) {
    r[i].set(_commandBuffer.v + _arrayStride*i);
  }
  return r;
}

void arNormal3Node::setNormal3(int number, float* normal3, int* IDs) {
  if (active()) {
    _nodeLock.lock("arNormal3Node::setNormal3 active");
      arStructuredData* r = _dumpData(number, normal3, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arNormal3Node::setNormal3 inactive");
    _mergeElements(number, normal3, IDs);
  }
}

// Slow, Python-compatible.
void arNormal3Node::setNormal3(vector<arVector3>& normal3) {
  const unsigned num = normal3.size();
  float* fPtr = new float[num*_arrayStride];
  for (unsigned i = 0; i < num; ++i) {
    normal3[i].get(fPtr + _arrayStride*i);
  }
  setNormal3(num, fPtr, NULL);
  delete [] fPtr;
}

// Slow, Python-compatible.
void arNormal3Node::setNormal3(vector<arVector3>& normal3, vector<int>& IDs) {
  const unsigned num = min(IDs.size(), normal3.size());
  float* fPtr = new float[_arrayStride*num];
  int* iPtr = new int[num];
  for (unsigned i = 0; i < num; ++i) {
    normal3[i].get(fPtr + _arrayStride*i);
    iPtr[i] = IDs[i];
  }
  setNormal3(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
