//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// It is really OBNOXIOUS that we have to define the graphics database here.
#include "arGraphicsDatabase.h"

arTransformNode::arTransformNode(){
  // A sensible default name.
  _name = "transform_node";
  _typeCode = AR_G_TRANSFORM_NODE;
  _typeString = "transform";
}

void arTransformNode::draw(arGraphicsContext*){ 
  ar_mutex_lock(&_nodeLock);
  glMultMatrixf(_transform.v);
  ar_mutex_unlock(&_nodeLock);
}

arStructuredData* arTransformNode::dumpData(){
  // It is the responsibility of the caller to delete this record.
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r = _dumpData(_transform, false);
  ar_mutex_unlock(&_nodeLock);
  return r;
}

bool arTransformNode::receiveData(arStructuredData* inData){
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(inData)){
    return true;
  }
  if (inData->getID() != _g->AR_TRANSFORM){
    cerr << "arTransformNode error: expected "
         << _g->AR_TRANSFORM
         << " (" << _g->_stringFromID(_g->AR_TRANSFORM) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }
  ar_mutex_lock(&_nodeLock);
  inData->dataOut(_g->AR_TRANSFORM_MATRIX, _transform.v, AR_FLOAT, 16);
  ar_mutex_unlock(&_nodeLock);
  return true;
}

arMatrix4 arTransformNode::getTransform(){
  ar_mutex_lock(&_nodeLock);
  arMatrix4 result = _transform;
  ar_mutex_unlock(&_nodeLock);
  return result;
}

void arTransformNode::setTransform(const arMatrix4& transform){
  if (_owningDatabase){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(transform, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _transform = transform;
    ar_mutex_unlock(&_nodeLock);
  }
}

/// This method is NOT thread-safe. Instead, it is the caller's responsbility
/// to call it from within a locked section.
arStructuredData* arTransformNode::_dumpData(const arMatrix4& transform,
                                             bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result = _owningDatabase->getDataParser()->getStorage(_g->AR_TRANSFORM);
  }
  else{
    result = _g->makeDataRecord(_g->AR_TRANSFORM);
  }
  _dumpGenericNode(result,_g->AR_TRANSFORM_ID);
  result->dataIn(_g->AR_TRANSFORM_MATRIX,transform.v,AR_FLOAT,16);
  return result;
}
