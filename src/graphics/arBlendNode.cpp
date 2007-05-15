//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arBlendNode::arBlendNode(){
  // todo: initializers
  _name = "blend_node"; // default
  _typeCode = AR_G_BLEND_NODE;
  _typeString = "blend"; 
  _blendFactor = 1.0; 
}

arStructuredData* arBlendNode::dumpData(){
  // Caller is responsible for deleting.
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_blendFactor, false); 
  _nodeLock.unlock();
  return r;
}

bool arBlendNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_BLEND, inData->getID(), "arBlendNode"))
    return false;

  _nodeLock.lock();
  _blendFactor = inData->getDataFloat(_g->AR_BLEND_FACTOR);
  _nodeLock.unlock();
  return true;
}

float arBlendNode::getBlend(){
  _nodeLock.lock();
    const float r = _blendFactor;
  _nodeLock.unlock();
  return r;
}

void arBlendNode::setBlend(float blendFactor){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(blendFactor, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _blendFactor = blendFactor;
    _nodeLock.unlock();
  }
}

// NOT thread-safe.
arStructuredData* arBlendNode::_dumpData(float blendFactor, bool owned){
  arStructuredData* result = owned ?
    getOwner()->getDataParser()->getStorage(_g->AR_BLEND) :
    _g->makeDataRecord(_g->AR_BLEND);
  _dumpGenericNode(result,_g->AR_BLEND_ID);
  result->dataIn(_g->AR_BLEND_FACTOR, &blendFactor, AR_FLOAT, 1);
  return result;
}
