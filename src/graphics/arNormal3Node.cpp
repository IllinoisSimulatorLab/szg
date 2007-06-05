//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNormal3Node.h"
#include "arGraphicsDatabase.h"

arNormal3Node::arNormal3Node(){
  _name = "normal3_node";
  _typeCode = AR_G_NORMAL3_NODE;
  _typeString = "normal3";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 3;
  _commandBuffer.grow(1);
}

void arNormal3Node::initialize(arDatabase* database){
  // _g (the graphics language) is set in this call.
  arGraphicsNode::initialize(database);
  _recordType = _g->AR_NORMAL3;
  _IDField = _g->AR_NORMAL3_ID;
  _indexField = _g->AR_NORMAL3_NORMAL_IDS;
  _dataField = _g->AR_NORMAL3_NORMALS;
  _commandBuffer.setType(_g->AR_NORMAL3);
}

// Included for speedy access. This is NOT thread-safe and instead must
// be called from within a locked section. 
const float* arNormal3Node::getNormal3(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arNormal3Node::setNormal3(int number, float* normal3, int* IDs){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(number, normal3, IDs, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _mergeElements(number, normal3, IDs);
    _nodeLock.unlock();
  }
}

// Slower way to get the normals data (there are several extra copy 
// operations). However, this is more convenient for calling from Python.
// Thread-safe.
vector<arVector3> arNormal3Node::getNormal3(){
  vector<arVector3> result;
  arGuard dummy(_nodeLock);
  unsigned int num = _commandBuffer.size()/_arrayStride;
  result.resize(num);
  for (unsigned i = 0; i < num; i++){
    result[i] = arVector3(_commandBuffer.v + 3*i);
  }
  return result;
}

// Adds an additional copy plus dynamic memory allocation. 
// But this is more convenient for calling from Python.
// Thread-safe.
void arNormal3Node::setNormal3(vector<arVector3>& normal3){
  float* fPtr = new float[normal3.size()*3];
  for (unsigned int i = 0; i < normal3.size(); i++){
    fPtr[3*i  ] = normal3[i][0];
    fPtr[3*i+1] = normal3[i][1];
    fPtr[3*i+2] = normal3[i][2];
  }
  setNormal3(normal3.size(), fPtr, NULL);
  delete [] fPtr;
}

// Adds an additional copy plus dynamic memory allocation.
// But this is more convenient for calling from Python.
// Thread-safe.
void arNormal3Node::setNormal3(vector<arVector3>& normal3,
			       vector<int>& IDs){
  const unsigned num = min(IDs.size(), normal3.size());
  float* fPtr = new float[3*num];
  int* iPtr = new int[num];
  for (unsigned int i = 0; i < num; i++){
    fPtr[3*i  ] = normal3[i][0];
    fPtr[3*i+1] = normal3[i][1];
    fPtr[3*i+2] = normal3[i][2];
    iPtr[i] = IDs[i];
  }
  setNormal3(num, fPtr, iPtr);
  delete [] fPtr;
  delete [] iPtr;
}
