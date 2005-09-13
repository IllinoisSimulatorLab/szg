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

// PROBLEM: MAYBE I SHOULD GO AHEAD AND MERGE ALL THE ARRAY NODES TOGETHER
// VIA SUBCLASSING? (AT LEAST WITH RESPECT TO FLOAT ARRAYs vs INT ARRAYs)
float* arColor4Node::getColor4(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arColor4Node::setColor4(int number, float* color4, int* IDs){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(number, color4, IDs);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _mergeElements(number, color4, IDs);
  }
}
