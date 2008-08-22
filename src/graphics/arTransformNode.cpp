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
  arGuard _(_nodeLock, "arTransformNode::draw");
  glMultMatrixf(_transform.v);
}

bool arTransformNode::receiveData(arStructuredData* inData) {
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(inData))
    return true;
  if (!_g->checkNodeID(_g->AR_TRANSFORM, inData->getID(), "arTransformNode"))
    return false;

  arGuard _(_nodeLock, "arTransformNode::receiveData");
  inData->dataOut(_g->AR_TRANSFORM_MATRIX, _transform.v, AR_FLOAT, 16);
  return true;
}

arMatrix4 arTransformNode::getTransform() {
  arGuard _(_nodeLock, "arTransformNode::getTransform");
  return _transform;
}

void arTransformNode::setTransform(const arMatrix4& transform) {
  if (active()) {
    _nodeLock.lock("arTransformNode::setTransform active");
      arStructuredData* r = _dumpData(transform, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arTransformNode::setTransform inactive");
    _transform = transform;
  }
}

arStructuredData* arTransformNode::dumpData() {
  arGuard _(_nodeLock, "arTransformNode::dumpData");
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
