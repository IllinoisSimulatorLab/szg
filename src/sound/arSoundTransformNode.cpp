//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundTransformNode.h"
#include "arMath.h"
#include "arSoundDatabase.h" // to get ar_transformStack

arSoundTransformNode::arSoundTransformNode(){
  _typeCode = AR_S_TRANSFORM_NODE;
  _typeString = "transform";
}

bool arSoundTransformNode::render(){
  ar_transformStack.top() = ar_transformStack.top() *
    arMatrix4(_commandBuffer.v); // "glMultMatrixf"
  return true;
}

const int len = sizeof(arMatrix4) / sizeof(float);

arStructuredData* arSoundTransformNode::dumpData(){
  arStructuredData* result = _l.makeDataRecord(_l.AR_TRANSFORM);
  _dumpGenericNode(result, _l.AR_TRANSFORM_ID);
  if (!result->dataIn(_l.AR_TRANSFORM_MATRIX, _commandBuffer.v, AR_FLOAT, len)) {
    delete result;
    return NULL;
  }
  return result;
}

bool arSoundTransformNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _l.AR_TRANSFORM){
    cout << "arTransformNode error: expected "
         << _l.AR_TRANSFORM
         << " (" << _l._stringFromID(_l.AR_TRANSFORM) << "), not "
         << inData->getID()
         << " (" << _l._stringFromID(inData->getID()) << ")\n";
    return false;
  }

  _commandBuffer.grow(len);
  inData->dataOut(_l.AR_TRANSFORM_MATRIX, _commandBuffer.v, AR_FLOAT, len);
  return true;
}
