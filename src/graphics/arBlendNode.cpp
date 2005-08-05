//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arBlendNode::arBlendNode(){
  _typeCode = AR_G_BLEND_NODE;
  _typeString = "blend"; 
  _blendFactor = 1.0; 
}

arStructuredData* arBlendNode::dumpData(){
  return _dumpData(_blendFactor); 
}

bool arBlendNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_BLEND){
    cerr << "arBlendNode error: expected "
         << _g->AR_BLEND
         << " (" << _g->_stringFromID(_g->AR_BLEND) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  _commandBuffer.grow(2);
  _commandBuffer.v[0] = inData->getDataFloat(_g->AR_BLEND_FACTOR);
  // AARGH! annoying duplication of resources with the blend factor.
  _blendFactor = _commandBuffer.v[0];
  // DEFUNCT
  //_blend = &_commandBuffer;
  return true;
}

float arBlendNode::getBlend(){
  return _blendFactor;
}

arStructuredData* arBlendNode::_dumpData(float blendFactor){
  arStructuredData* result = _g->makeDataRecord(_g->AR_BLEND);
  _dumpGenericNode(result,_g->AR_BLEND_ID);
  result->dataIn(_g->AR_BLEND_FACTOR, &blendFactor, AR_FLOAT, 1);
  return result;
}
