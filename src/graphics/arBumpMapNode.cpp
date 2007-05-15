//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBumpMapNode.h"
#include "arGraphicsDatabase.h"

arBumpMapNode::arBumpMapNode(){
  _name = "bump_map_node";
  _typeCode = AR_G_BUMP_MAP_NODE;
  _typeString = "bump map";
}

arStructuredData* arBumpMapNode::dumpData(){
  int i, currPos = 0;	// this will keep track of where we are in commandBuffer
  arStructuredData* result = _g->makeDataRecord(_g->AR_BUMPMAP);
  _dumpGenericNode(result,_g->AR_BUMPMAP_ID);

  // filename
  int len = (ARint) _commandBuffer.v[currPos++]; // characters in filename
  result->setDataDimension(_g->AR_BUMPMAP_FILE, len);
  ARchar* text = (ARchar*) result->getDataPtr(_g->AR_BUMPMAP_FILE,AR_CHAR);
  for (i=0; i<len; ++i)
    text[i] = (ARint) _commandBuffer.v[currPos++];
  
  // height
  result->dataIn(_g->AR_BUMPMAP_HEIGHT, &_commandBuffer.v[currPos++], AR_FLOAT, 1);

  return result;
}

bool arBumpMapNode::receiveData(arStructuredData*){
  // Deprecated.
  return true;
}
