//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arNormal3Node.h"
#include "arGraphicsDatabase.h"

arNormal3Node::arNormal3Node(arGraphicsDatabase* owner){
  _typeCode = AR_G_NORMAL3_NODE;
  _typeString = "normal3";
  _nodeDataType = AR_FLOAT;
  _arrayStride = 3;
  // DEFUNCT
  //_whichBufferToReplace = &_normal3;
  _g = &(owner->_gfx);
  _recordType = _g->AR_NORMAL3;
  _IDField = _g->AR_NORMAL3_ID;
  _indexField = _g->AR_NORMAL3_NORMAL_IDS;
  _dataField = _g->AR_NORMAL3_NORMALS;
  _commandBuffer.grow(1);
  _commandBuffer.setType(_g->AR_NORMAL3);
}

float* arNormal3Node::getNormal3(int& number){
  number = _commandBuffer.size()/_arrayStride;
  return _commandBuffer.v;
}

void arNormal3Node::setNormal3(int number, float* normal3, int* IDs){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(number, normal3, IDs);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _mergeElements(number, normal3, IDs);
  }
}

