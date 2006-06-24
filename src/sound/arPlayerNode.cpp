//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPlayerNode.h"
#include "arSoundDatabase.h"

arPlayerNode::arPlayerNode(){
  // RedHat 8.0 will fail to compile this if the following are outside the
  // constructor body
  _typeCode = AR_S_PLAYER_NODE;
  _typeString = "player";
}

arStructuredData* arPlayerNode::dumpData(){
  arStructuredData* result = _l.makeDataRecord(_l.AR_PLAYER);
  _dumpGenericNode(result,_l.AR_PLAYER_ID);
  if (!result->dataIn(_l.AR_PLAYER_MATRIX,_commandBuffer.v,AR_FLOAT,16) ||
      !result->dataIn(_l.AR_PLAYER_MID_EYE_OFFSET,_commandBuffer.v+16,AR_FLOAT,3) ||
      !result->dataIn(_l.AR_PLAYER_UNIT_CONVERSION,_commandBuffer.v+19,AR_FLOAT,1)) {
    delete result;
    return NULL;
  }
  return result;
}

bool arPlayerNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _l.AR_PLAYER){
    cout << "arPlayerNode error: expected "
         << _l.AR_PLAYER
         << " (" << _l._stringFromID(_l.AR_PLAYER) << "), not "
         << inData->getID()
         << " (" << _l._stringFromID(inData->getID()) << ")\n";
    return false;
  }
  _commandBuffer.grow(20);
  inData->dataOut(_l.AR_PLAYER_MATRIX, _commandBuffer.v, AR_FLOAT,16);
  inData->dataOut(_l.AR_PLAYER_MID_EYE_OFFSET,_commandBuffer.v+16,AR_FLOAT,3);
  inData->dataOut(_l.AR_PLAYER_UNIT_CONVERSION,_commandBuffer.v+19,AR_FLOAT,1);
  return true;
}
