//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arTex2Node.h"
#include "arGraphicsDatabase.h"

arTex2Node::arTex2Node(){
  // A sensible default name.
  _name = "tex2_node";
  _typeCode = AR_G_TEX2_NODE;
  _typeString = "tex2";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 2;
  _commandBuffer.grow(1);
}

void arTex2Node::initialize(arDatabase* database){
  // _g (the graphics language) is set in this call.
  arGraphicsNode::initialize(database);
  _recordType = _g->AR_TEX2;
  _IDField = _g->AR_TEX2_ID;
  _indexField = _g->AR_TEX2_TEX_IDS;
  _dataField = _g->AR_TEX2_COORDS;
  _commandBuffer.setType(_g->AR_TEX2);
}

/// Included for speedy access. This is NOT thread-safe and instead must
/// be called from within a locked section. 
const float* arTex2Node::getTex2(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arTex2Node::setTex2(int number, float* tex2, int* IDs){
  if (active()){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(number, tex2, IDs, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _mergeElements(number, tex2, IDs);
    ar_mutex_unlock(&_nodeLock);
  }
}


/// Slower way to get the tex coord data (there are several extra copy 
/// operations). However, this is more convenient for calling from Python.
/// Thread-safe.
vector<arVector2> arTex2Node::getTex2(){
  vector<arVector2> result;
  // Must be thread-safe.
  ar_mutex_lock(&_nodeLock);
  unsigned int num = _commandBuffer.size()/_arrayStride;
  result.resize(num);
  for (unsigned int i = 0; i < num; i++){
    result[i][0] = _commandBuffer.v[2*i];
    result[i][1] = _commandBuffer.v[2*i+1];
  }
  ar_mutex_unlock(&_nodeLock);
  return result;
}

/// Adds an additional copy plus dynamic memory allocation. 
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arTex2Node::setTex2(vector<arVector2>& tex2){
  float* ptr = new float[tex2.size()*2];
  for (unsigned int i = 0; i < tex2.size(); i++){
    ptr[2*i] = tex2[i][0];
    ptr[2*i+1] = tex2[i][1];
  }
  setTex2(tex2.size(), ptr, NULL);
  delete [] ptr;
}

/// Adds an additional copy plus dynamic memory allocation.
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arTex2Node::setTex2(vector<arVector2>& tex2,
			 vector<int>& IDs){
  unsigned int num = IDs.size();
  // If there are two many IDs for points, cut down on our dimension.
  if (num > tex2.size()){
    num = tex2.size();
  }
  float* fPtr = new float[2*num];
  int* iPtr = new int[num];
  for (unsigned int i = 0; i < num; i++){
    fPtr[2*i] = tex2[i][0];
    fPtr[2*i+1] = tex2[i][1];
    iPtr[i] = IDs[i];
  }
  setTex2(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
