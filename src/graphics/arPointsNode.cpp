//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPointsNode.h"
#include "arGraphicsDatabase.h"

arPointsNode::arPointsNode() : arGraphicsArrayNode(AR_FLOAT, 3) {
  _name = "points_node";
  _typeCode = AR_G_POINTS_NODE;
  _typeString = "points";
  _commandBuffer.grow(1);
}

void arPointsNode::initialize(arDatabase* database){
  arGraphicsNode::initialize(database);
  arGraphicsArrayNode::initialize(
    _g->AR_POINTS,
    _g->AR_POINTS_ID,
    _g->AR_POINTS_POINT_IDS,
    _g->AR_POINTS_POSITIONS,
    _g->AR_POINTS);
}

// Speedy accessor.  Not thread-safe, so call while _nodeLock'd.
const float* arPointsNode::getPoints(int& number){
  number = _numElements();
  return _commandBuffer.v;
}

// Slow, thread-safe, Python-compatible accessor.
vector<arVector3> arPointsNode::getPoints(){
  arGuard dummy(_nodeLock);
  const unsigned num = _numElements();
  vector<arVector3> r(num);
  for (unsigned i = 0; i < num; ++i){
    r[i].set(_commandBuffer.v + _arrayStride * i);
  }
  return r;
}

void arPointsNode::setPoints(int number, float* points, int* IDs){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(number, points, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r); // why not getOwner() ?
  }
  else{
    arGuard dummy(_nodeLock);
    _mergeElements(number, points, IDs);
  }
}

// Slow, Python-compatible.
void arPointsNode::setPoints(vector<arVector3>& points){
  const unsigned num = points.size();
  float* fPtr = new float[num*_arrayStride];
  for (unsigned i = 0; i < num; ++i){
    points[i].get(fPtr + _arrayStride*i);
  }
  setPoints(num, fPtr, NULL);
  delete [] fPtr;
}

// Slow, Python-compatible.
void arPointsNode::setPoints(vector<arVector3>& points, vector<int>& IDs){
  const unsigned num = min(IDs.size(), points.size());
  float* fPtr = new float[_arrayStride*num];
  int* iPtr = new int[num];
  for (unsigned i = 0; i < num; ++i){
    points[i].get(fPtr + _arrayStride*i);
    iPtr[i] = IDs[i];
  }
  setPoints(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
