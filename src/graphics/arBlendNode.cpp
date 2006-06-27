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
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r = _dumpData(_blendFactor, false); 
  ar_mutex_unlock(&_nodeLock);
  return r;
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

  ar_mutex_lock(&_nodeLock);
  _blendFactor = inData->getDataFloat(_g->AR_BLEND_FACTOR);
  ar_mutex_unlock(&_nodeLock);
  return true;
}

float arBlendNode::getBlend(){
  ar_mutex_lock(&_nodeLock);
    const float r = _blendFactor;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

void arBlendNode::setBlend(float blendFactor){
  if (active()){
    ar_mutex_lock(&_nodeLock);
      arStructuredData* r = _dumpData(blendFactor, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
      _blendFactor = blendFactor;
    ar_mutex_unlock(&_nodeLock);
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
