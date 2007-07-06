//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTex2Node.h"
#include "arGraphicsDatabase.h"

arTex2Node::arTex2Node() : arGraphicsArrayNode(AR_FLOAT, 2) {
  _name = "tex2_node";
  _typeCode = AR_G_TEX2_NODE;
  _typeString = "tex2";
  _commandBuffer.grow(1);
}

void arTex2Node::initialize(arDatabase* database){
  // _g (the graphics language) is set in this call.
  arGraphicsNode::initialize(database);
  arGraphicsArrayNode::initialize(
    _g->AR_TEX2,
    _g->AR_TEX2_ID,
    _g->AR_TEX2_TEX_IDS,
    _g->AR_TEX2_COORDS,
    _g->AR_TEX2);
}

// Speedy accessor.  Not thread-safe, so call while _nodeLock'd.
const float* arTex2Node::getTex2(int& number){
  number = _numElements();
  return _commandBuffer.v;
}

// Slow, thread-safe, Python-compatible accessor.
vector<arVector2> arTex2Node::getTex2(){
  arGuard dummy(_nodeLock);
  const unsigned num = _numElements();
  vector<arVector2> r(num);
  for (unsigned i = 0; i < num; ++i){
    r[i].set(_commandBuffer.v + _arrayStride*i);
  }
  return r;
}

void arTex2Node::setTex2(int number, float* tex2, int* IDs){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(number, tex2, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r); // why not getOwner() ?
  }
  else{
    arGuard dummy(_nodeLock);
    _mergeElements(number, tex2, IDs);
  }
}

// Slow, Python-compatible.
void arTex2Node::setTex2(vector<arVector2>& tex2){
  const unsigned num = tex2.size();
  float* ptr = new float[num*_arrayStride];
  for (unsigned i = 0; i < num; ++i){
    tex2[i].get(ptr + _arrayStride*i);
  }
  setTex2(num, ptr, NULL);
  delete [] ptr;
}

// Slow, Python-compatible.
void arTex2Node::setTex2(vector<arVector2>& tex2, vector<int>& IDs){
  const unsigned num = min(IDs.size(), tex2.size());
  float* fPtr = new float[_arrayStride*num];
  int* iPtr = new int[num];
  for (unsigned i = 0; i < num; ++i){
    tex2[i].get(fPtr + _arrayStride*i);
    iPtr[i] = IDs[i];
  }
  setTex2(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
