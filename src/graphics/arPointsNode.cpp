//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPointsNode.h"
#include "arGraphicsDatabase.h"

arPointsNode::arPointsNode(){
  _name = "points_node";
  _typeCode = AR_G_POINTS_NODE;
  _typeString = "points";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 3;
  _commandBuffer.grow(1);
}

void arPointsNode::initialize(arDatabase* database){
  // _g (the graphics language) is set in this call.
  arGraphicsNode::initialize(database);
  _recordType = _g->AR_POINTS;
  _IDField = _g->AR_POINTS_ID;
  _indexField = _g->AR_POINTS_POINT_IDS;
  _dataField = _g->AR_POINTS_POSITIONS;
  _commandBuffer.setType(_g->AR_POINTS);
}

// Fast access to points node contents.
// Cannot *change* the point values using this pointer (that would break
// sharing). Also, since the pointer can get invalidated if the array
// gets expanded, this must be called within node->lock()/node->unlock().
const float* arPointsNode::getPoints(int& number){
  // DO NOT lock the node here. We are assuming this method is called from
  // within a locked node lock.
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

// Fast way to get data into the node. 
void arPointsNode::setPoints(int number, float* points, int* IDs){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(number, points, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _mergeElements(number, points, IDs);
    _nodeLock.unlock();
  }
}

// Slower way to get the points data (there are several extra copy 
// operations). However, this is more convenient for calling from Python.
// Thread-safe.
vector<arVector3> arPointsNode::getPoints(){
  vector<arVector3> result;
  arGuard dummy(_nodeLock);
  const unsigned num = _commandBuffer.size()/_arrayStride;
  result.resize(num);
  for (unsigned i = 0; i < num; i++){
    result[i][0] = _commandBuffer.v[3*i];
    result[i][1] = _commandBuffer.v[3*i+1];
    result[i][2] = _commandBuffer.v[3*i+2];
  }
  return result;
}

// Adds an additional copy plus dynamic memory allocation. 
// But this is more convenient for calling from Python.
// Thread-safe.
void arPointsNode::setPoints(vector<arVector3>& points){
  float* ptr = new float[points.size()*3];
  for (unsigned int i = 0; i < points.size(); i++){
    ptr[3*i] = points[i][0];
    ptr[3*i+1] = points[i][1];
    ptr[3*i+2] = points[i][2];
  }
  setPoints(points.size(), ptr, NULL);
  delete [] ptr;
}

// Adds an additional copy plus dynamic memory allocation.
// But this is more convenient for calling from Python.
// Thread-safe.
void arPointsNode::setPoints(vector<arVector3>& points,
			     vector<int>& IDs){
  const unsigned num = min(IDs.size(), points.size());
  float* fPtr = new float[3*num];
  int* iPtr = new int[num];
  for (unsigned int i = 0; i < num; i++){
    fPtr[3*i] = points[i][0];
    fPtr[3*i+1] = points[i][1];
    fPtr[3*i+2] = points[i][2];
    iPtr[i] = IDs[i];
  }
  setPoints(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
