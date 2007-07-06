//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
 
#include "arPrecompiled.h"
#include "arGraphicsArrayNode.h"
#include "arGraphicsDatabase.h"

bool arGraphicsArrayNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_recordType, inData->getID(), "arGraphicsArrayNode"))
    return false;

  ARint* theIDs = (ARint*)inData->getDataPtr(_indexField, AR_INT);
  void* theData = inData->getDataPtr(_dataField, _nodeDataType);
  const ARint len = inData->getDataDimension(_dataField) / _arrayStride;
  const ARint numberIDs = inData->getDataDimension(_indexField);
  
  if (numberIDs <= 0){
    ar_log_warning() << "arGraphicsArrayNode " << _typeString << ": no IDs.\n";
    // Nothing to do.
    return true;
  }

  arGuard dummy(_nodeLock);
  if (theIDs[0] == -1){
    // Pack array elements in order.
    _mergeElements(len, theData);
  }
  else{
    _mergeElements(len, theData, theIDs);
  }
  
  // Bookkeeping.
  _commandBuffer.setType(_recordType);
  return true;
}

// NOT thread-safe. Call while _nodeLock'd.
// Methods like arPointsNode::setPoints and arGraphicsArrayNode::receiveData
// call this while _nodeLock'd, so we can't lock in here lest deadlocks ensue.
void arGraphicsArrayNode::_mergeElements(int number, void* elements, int* IDs){
  const int numbytes = arDataTypeSize(_nodeDataType);
  if (!IDs){
    // Coordinate vectors are packed in ID order.
    _commandBuffer.grow(_arrayStride*number);
    memcpy(_commandBuffer.v, elements, _arrayStride * number * numbytes);
  }
  else{
    // Coordinate vectors are packed in arbitrary order.
    // Find the maximum ID, and allocate more space if needed.
    int i = 0;
    int max=-1;
    for (i=0; i<number; i++){
      if (IDs[i] > max){
	max = IDs[i];
      }
    }
    _commandBuffer.grow(_arrayStride * (max+1));
    for (i=0; i<number; i++)
      memcpy(&_commandBuffer.v[_arrayStride*IDs[i]], 
	     (const char*)elements + numbytes * (_arrayStride*i),
	     _arrayStride * numbytes);
  }
}

arStructuredData* arGraphicsArrayNode::dumpData(){
  // Guard because _commandBuffer.v may change if _commandBuffer resizes.
  arGuard dummy(_nodeLock);
  return _dumpData(_numElements(), _commandBuffer.v, NULL, false);
}

arStructuredData* arGraphicsArrayNode::_dumpData(
  int number, void* elements, int* IDs, bool owned){
  arStructuredData* r = owned ?
    _owningDatabase->getDataParser()->getStorage(_recordType) :
    _g->makeDataRecord(_recordType);
  if (!r){
    ar_log_warning() << "arGraphicsArrayNode failed to make record of type " <<
      _g->numstringFromID(_recordType) << ".\n";
    return NULL;
  }

  _dumpGenericNode(r, _IDField);
  r->setDataDimension(_indexField, number);
  r->setDataDimension(_dataField, number*_arrayStride);
  if (IDs){
    r->dataIn(_indexField, IDs, AR_INT, number);
  }
  else{
    // Index field gets the identity map.
    ARint* dataIDs = (ARint*)r->getDataPtr(_indexField, AR_INT);
    for (int i=0; i<number; ++i)
      dataIDs[i] = i;
  }
  // Fill in the data field. BUG: THIS RELIES ON THE FACT THAT ALL OUR DATA
  // TYPE HAVE THE SAME SIZE IN MEMORY (i.e. FLOAT AND INT)
  memcpy(r->getDataPtr(_dataField,_nodeDataType),
         elements, _arrayStride * number * arDataTypeSize(_nodeDataType));
  return r;
}
