//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h" // too bad this must be included

arTransformNode::arTransformNode() {
  _name = "transform_node";
  _typeCode = AR_G_TRANSFORM_NODE;
  _typeString = "transform";
}

void arTransformNode::draw(arGraphicsContext*) {
  arGuard dummy(_nodeLock);
  glMultMatrixf(_transform.v);
}

bool arTransformNode::receiveData(arStructuredData* inData) {
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(inData))
    return true;
  if (!_g->checkNodeID(_g->AR_TRANSFORM, inData->getID(), "arTransformNode"))
    return false;

  arGuard dummy(_nodeLock);
  inData->dataOut(_g->AR_TRANSFORM_MATRIX, _transform.v, AR_FLOAT, 16);
  return true;
}

arMatrix4 arTransformNode::getTransform() {
  arGuard dummy(_nodeLock);
  return _transform;
}

void arTransformNode::setTransform(const arMatrix4& transform) {
  if (active()) {
    _nodeLock.lock();
      arStructuredData* r = _dumpData(transform, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _transform = transform;
  }
}

arStructuredData* arTransformNode::dumpData() {
  arGuard dummy(_nodeLock);
  return _dumpData(_transform, false);
}

arStructuredData* arTransformNode::_dumpData(const arMatrix4& transform, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_TRANSFORM);
  _dumpGenericNode(r, _g->AR_TRANSFORM_ID);
  if (!r->dataIn(_g->AR_TRANSFORM_MATRIX, transform.v, AR_FLOAT, 16)) {
    delete r;
    return NULL;
  }
  return r;
}
