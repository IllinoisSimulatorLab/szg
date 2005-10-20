//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arColor4Node.h"
#include "arGraphicsDatabase.h"

arColor4Node::arColor4Node(){
  _name = "color4_node";
  _typeCode = AR_G_COLOR4_NODE;
  _typeString = "color4";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 4;
  _commandBuffer.grow(1);
}

void arColor4Node::initialize(arDatabase* database){
  arGraphicsArrayNode::initialize(database);
  arGraphicsDatabase* d = (arGraphicsDatabase*) database;
  _g = &(d->_gfx);
  _recordType = _g->AR_COLOR4;
  _IDField = _g->AR_COLOR4_ID;
  _indexField = _g->AR_COLOR4_COLOR_IDS;
  _dataField = _g->AR_COLOR4_COLORS;
  _commandBuffer.setType(_g->AR_COLOR4);
}

/// Included for speedy access. This is NOT thread-safe and instead must
/// be called from within a locked section. 
const float* arColor4Node::getColor4(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arColor4Node::setColor4(int number, float* color4, int* IDs){
  if (_owningDatabase){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(number, color4, IDs, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _mergeElements(number, color4, IDs);
    ar_mutex_unlock(&_nodeLock);
  }
}

/// Slower way to get the color data (there are several extra copy 
/// operations). However, this is more convenient for calling from Python.
/// Thread-safe.
vector<arVector4> arColor4Node::getColor4(){
  vector<arVector4> result;
  // Must be thread-safe.
  ar_mutex_lock(&_nodeLock);
  unsigned int num = _commandBuffer.size()/_arrayStride;
  result.resize(num);
  for (unsigned int i = 0; i < num; i++){
    result[i][0] = _commandBuffer.v[4*i];
    result[i][1] = _commandBuffer.v[4*i+1];
    result[i][2] = _commandBuffer.v[4*i+2];
    result[i][3] = _commandBuffer.v[4*i+3];
  }
  ar_mutex_unlock(&_nodeLock);
  return result;
}

/// Adds an additional copy plus dynamic memory allocation. 
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arColor4Node::setColor4(vector<arVector4>& color4){
  float* ptr = new float[color4.size()*4];
  for (unsigned int i = 0; i < color4.size(); i++){
    ptr[4*i] = color4[i][0];
    ptr[4*i+1] = color4[i][1];
    ptr[4*i+2] = color4[i][2];
    ptr[4*i+3] = color4[i][3];
  }
  setColor4(color4.size(), ptr, NULL);
  delete [] ptr;
}

/// Adds an additional copy plus dynamic memory allocation.
/// But this is more convenient for calling from Python.
/// Thread-safe.
void arColor4Node::setColor4(vector<arVector4>& color4,
			     vector<int>& IDs){
  unsigned int num = IDs.size();
  // If there are two many IDs for points, cut down on our dimension.
  if (num > color4.size()){
    num = color4.size();
  }
  float* fPtr = new float[4*num];
  int* iPtr = new int[num];
  for (unsigned int i = 0; i < num; i++){
    fPtr[4*i] = color4[i][0];
    fPtr[4*i+1] = color4[i][1];
    fPtr[4*i+2] = color4[i][2];
    fPtr[4*i+3] = color4[i][3];
    iPtr[i] = IDs[i];
  }
  setColor4(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
