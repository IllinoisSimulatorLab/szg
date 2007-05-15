//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h" // too bad this must be included

arTransformNode::arTransformNode(){
  _name = "transform_node";
  _typeCode = AR_G_TRANSFORM_NODE;
  _typeString = "transform";
}

void arTransformNode::draw(arGraphicsContext*){ 
  _nodeLock.lock();
  glMultMatrixf(_transform.v);
  _nodeLock.unlock();
}

arStructuredData* arTransformNode::dumpData(){
  // It is the responsibility of the caller to delete this record.
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_transform, false);
  _nodeLock.unlock();
  return r;
}

bool arTransformNode::receiveData(arStructuredData* inData){
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(inData))
    return true;
  if (!_g->checkNodeID(_g->AR_TRANSFORM, inData->getID(), "arTransformNode"))
    return false;

  _nodeLock.lock();
    inData->dataOut(_g->AR_TRANSFORM_MATRIX, _transform.v, AR_FLOAT, 16);
  _nodeLock.unlock();
  return true;
}

arMatrix4 arTransformNode::getTransform(){
  _nodeLock.lock();
    const arMatrix4 result(_transform);
  _nodeLock.unlock();
  return result;
}

void arTransformNode::setTransform(const arMatrix4& transform){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(transform, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _transform = transform;
    _nodeLock.unlock();
  }
}

// NOT thread-safe. Call from within a locked section.
arStructuredData* arTransformNode::_dumpData(const arMatrix4& transform,
                                             bool owned){
  arStructuredData* result = owned ?
    _owningDatabase->getDataParser()->getStorage(_g->AR_TRANSFORM) :
    _g->makeDataRecord(_g->AR_TRANSFORM);
  _dumpGenericNode(result,_g->AR_TRANSFORM_ID);
  result->dataIn(_g->AR_TRANSFORM_MATRIX,transform.v,AR_FLOAT,16);
  return result;
}
