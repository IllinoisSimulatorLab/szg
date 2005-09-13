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
  ar_mutex_init(&_accessLock);
}

void arTransformNode::draw(arGraphicsContext*){ 
  ar_mutex_lock(&_accessLock);
  glMultMatrixf(_transform.v);
  ar_mutex_unlock(&_accessLock);
}

arStructuredData* arTransformNode::dumpData(){
  return _dumpData(_transform);
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
  ar_mutex_lock(&_accessLock);
  inData->dataOut(_g->AR_TRANSFORM_MATRIX, _transform.v, AR_FLOAT, 16);
  ar_mutex_unlock(&_accessLock);
  return true;
}

arMatrix4 arTransformNode::getTransform(){
  ar_mutex_lock(&_accessLock);
  arMatrix4 result = _transform;
  ar_mutex_unlock(&_accessLock);
  return result;
}

void arTransformNode::setTransform(const arMatrix4& transform){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(transform);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    ar_mutex_lock(&_accessLock);
    _transform = transform;
    ar_mutex_unlock(&_accessLock);
  }
}

arStructuredData* arTransformNode::_dumpData(const arMatrix4& transform){
  arStructuredData* result = _g->makeDataRecord(_g->AR_TRANSFORM);
  _dumpGenericNode(result,_g->AR_TRANSFORM_ID);
  // HMMM.. NOT REALLY SURE IF LOCKING IS NEEDED HERE...
  ar_mutex_lock(&_accessLock);
  result->dataIn(_g->AR_TRANSFORM_MATRIX,transform.v,AR_FLOAT,16);
  ar_mutex_unlock(&_accessLock);
  return result;
}
