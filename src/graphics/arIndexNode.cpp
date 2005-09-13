//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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
  _commandBuffer.grow(1);
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

// PROBLEM: MAYBE I SHOULD GO AHEAD AND MERGE ALL THE ARRAY NODES TOGETHER
// VIA SUBCLASSING? (AT LEAST WITH RESPECT TO FLOAT ARRAYs vs INT ARRAYs)
int* arIndexNode::getIndices(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return (int*)_commandBuffer.v;
}

void arIndexNode::setIndices(int number, int* indices, int* IDs){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(number, indices, IDs);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _mergeElements(number, indices, IDs);
  }
}

