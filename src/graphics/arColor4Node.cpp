//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arColor4Node.h"
#include "arGraphicsDatabase.h"

arColor4Node::arColor4Node() : arGraphicsArrayNode(AR_FLOAT, 4)
{
  _name = "color4_node";

  // todo: init these 3 in constructor of arDatabaseNode,
  // via constructor of arGraphicsNode ?
  _typeCode = AR_G_COLOR4_NODE;
  _typeString = "color4";
  _commandBuffer.grow(1);
}

void arColor4Node::initialize(arDatabase* database) {
  arGraphicsNode::initialize(database);
  arGraphicsArrayNode::initialize(
    _g->AR_COLOR4,
    _g->AR_COLOR4_ID,
    _g->AR_COLOR4_COLOR_IDS,
    _g->AR_COLOR4_COLORS,
    _g->AR_COLOR4);
}

// Speedy accessor.  Not thread-safe, so call while _nodeLock'd.
const float* arColor4Node::getColor4(int& number) {
  number = _numElements();
  return _commandBuffer.v;
}

// Slow, thread-safe, Python-compatible accessor.
vector<arVector4> arColor4Node::getColor4() {
  arGuard dummy(_nodeLock);
  const unsigned num = _numElements();
  vector<arVector4> r(num);
  for (unsigned i = 0; i < num; i++) {
    r[i].set(_commandBuffer.v + _arrayStride * i);
  }
  return r;
}

void arColor4Node::setColor4(int number, float* color4, int* IDs) {
  if (active()) {
    _nodeLock.lock();
      arStructuredData* r = _dumpData(number, color4, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _mergeElements(number, color4, IDs);
  }
}

// Slow, Python-compatible.
void arColor4Node::setColor4(vector<arVector4>& color4) {
  const unsigned num = color4.size();
  float* fPtr = new float[num*_arrayStride];
  for (unsigned i = 0; i < num; i++) {
    color4[i].get(fPtr + _arrayStride*i);
  }
  setColor4(num, fPtr, NULL);
  delete [] fPtr;
}

// Slow, Python-compatible.
void arColor4Node::setColor4(vector<arVector4>& color4, vector<int>& IDs) {
  const unsigned num = min(IDs.size(), color4.size());
  float* fPtr = new float[_arrayStride*num];
  int* iPtr = new int[num];
  for (unsigned i = 0; i < num; i++) {
    color4[i].get(fPtr + _arrayStride*i);
    iPtr[i] = IDs[i];
  }
  setColor4(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
