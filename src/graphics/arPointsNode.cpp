//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPointsNode.h"
#include "arGraphicsDatabase.h"

arPointsNode::arPointsNode(arGraphicsDatabase* owner){
  _typeCode = AR_G_POINTS_NODE;
  _typeString = "points";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 3;
  _whichBufferToReplace = &_points;
  _g = &(owner->_gfx);
  _recordType = _g->AR_POINTS;
  _IDField = _g->AR_POINTS_ID;
  _indexField = _g->AR_POINTS_POINT_IDS;
  _dataField = _g->AR_POINTS_POSITIONS;
  _commandBuffer.grow(1);
  _commandBuffer.setType(_g->AR_POINTS);
}

// PROBLEM: MAYBE I SHOULD GO AHEAD AND MERGE ALL THE ARRAY NODES TOGETHER
// VIA SUBCLASSING? (AT LEAST WITH RESPECT TO FLOAT ARRAYs vs INT ARRAYs)
float* arPointsNode::getPoints(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arPointsNode::setPoints(int number, float* points, int* IDs){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(number, points, IDs);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _mergeElements(number, points, IDs);
  }
}
