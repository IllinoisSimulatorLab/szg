//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arBlendNode::arBlendNode() {
  // todo: initializers
  _name = "blend_node"; // default
  _typeCode = AR_G_BLEND_NODE;
  _typeString = "blend";
  _blendFactor = 1.0;
}

bool arBlendNode::receiveData(arStructuredData* inData) {
  if (!_g->checkNodeID(_g->AR_BLEND, inData->getID(), "arBlendNode"))
    return false;

  arGuard dummy(_nodeLock);
  _blendFactor = inData->getDataFloat(_g->AR_BLEND_FACTOR);
  return true;
}

float arBlendNode::getBlend() {
  arGuard dummy(_nodeLock);
  return _blendFactor;
}

void arBlendNode::setBlend(float blendFactor) {
  if (active()) {
    _nodeLock.lock();
      arStructuredData* r = _dumpData(blendFactor, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _blendFactor = blendFactor;
  }
}

arStructuredData* arBlendNode::dumpData() {
  arGuard dummy(_nodeLock);
  return _dumpData(_blendFactor, false);
}

arStructuredData* arBlendNode::_dumpData(float blendFactor, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_BLEND);
  _dumpGenericNode(r, _g->AR_BLEND_ID);
  if (!r->dataIn(_g->AR_BLEND_FACTOR, &blendFactor, AR_FLOAT, 1)) {
    delete r;
    return NULL;
  }
  return r;
}
