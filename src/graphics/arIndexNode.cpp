//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arIndexNode.h"
#include "arGraphicsDatabase.h"

arIndexNode::arIndexNode(){
  // A sensible default name.
  _name = "index_node";
  _typeCode = AR_G_INDEX_NODE;
  _typeString = "index";
  _nodeDataType = AR_INT;
  _arrayStride = 1;
  // By default, there is a single index entry with index 0.
  _commandBuffer.grow(1);
  _commandBuffer.v[0] = 0;
}

void arIndexNode::initialize(arDatabase* database){
  // _g (the graphics language) is set in this call.
  arGraphicsNode::initialize(database);
  _recordType = _g->AR_INDEX;
  _IDField = _g->AR_INDEX_ID;
  _indexField = _g->AR_INDEX_INDEX_IDS;
  _dataField = _g->AR_INDEX_INDICES;
  _commandBuffer.setType(_g->AR_INDEX);
}

/// Included for speedy access. This is NOT thread-safe and instead must
/// be called from within a locked section. 
const int* arIndexNode::getIndices(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return (int*)_commandBuffer.v;
}

void arIndexNode::setIndices(int number, int* indices, int* IDs){
  if (active()){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(number, indices, IDs, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _mergeElements(number, indices, IDs);
    ar_mutex_unlock(&_nodeLock);
  }
}

/// Slower way to get the index data (there are several extra copy 
/// operations). However, this is more convenient for calling from Python.
/// Thread-safe.
vector<int> arIndexNode::getIndices(){
  vector<int> result;
  // Must be thread-safe.
  ar_mutex_lock(&_nodeLock);
  unsigned int num = _commandBuffer.size()/_arrayStride;
  // BUG BUG BUG. Relying on the fact that sizeof(int) = sizeof(float)
  int* ptr = (int*) _commandBuffer.v;
  result.resize(num);
  for (unsigned int i = 0; i < num; i++){
    result[i] = ptr[i];
  }
  ar_mutex_unlock(&_nodeLock);
  return result;
}

/// Adds an additional copy plus dynamic memory allocation. 
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arIndexNode::setIndices(vector<int>& indices){
  int* ptr = new int[indices.size()];
  for (unsigned int i = 0; i < indices.size(); i++){
    ptr[i] = indices[i];
  }
  setIndices(indices.size(), ptr, NULL);
  delete [] ptr;
}

/// Adds an additional copy plus dynamic memory allocation.
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arIndexNode::setIndices(vector<int>& indices,
			     vector<int>& IDs){
  unsigned int num = IDs.size();
  // If there are two many IDs for points, cut down on our dimension.
  if (num > indices.size()){
    num = indices.size();
  }
  int* ptr = new int[num];
  int* iPtr = new int[num];
  for (unsigned int i = 0; i < num; i++){
    ptr[i] = indices[i];
    iPtr[i] = IDs[i];
  }
  setIndices(num, ptr, iPtr);
  delete [] ptr;
  delete [] iPtr;
}
