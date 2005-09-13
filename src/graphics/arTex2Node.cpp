//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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

float* arTex2Node::getTex2(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arTex2Node::setTex2(int number, float* tex2, int* IDs){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(number, tex2, IDs);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _mergeElements(number, tex2, IDs);
  }
}
