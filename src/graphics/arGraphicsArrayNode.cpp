//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
 
#include "arPrecompiled.h"
#include "arGraphicsArrayNode.h"
#include "arGraphicsDatabase.h"

arStructuredData* arGraphicsArrayNode::dumpData(){
  // The call to _dumpData must take place within a locked section.
  // _commandBuffer.v can change as the result of a buffer resize.
  _nodeLock.lock();
  // The caller is responsible for deleting this record.
  arStructuredData* result = _dumpData(_commandBuffer.size()/_arrayStride,
                                       _commandBuffer.v, NULL, false);
  _nodeLock.unlock();
  return result;
}

bool arGraphicsArrayNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _recordType){
    ar_log_warning() << "arGraphicsArrayNode " << _typeString << " expected "
	 << _recordType << " (" << _g->_stringFromID(_recordType) << "), not "
	 << inData->getID() << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  ARint* theIDs = (ARint*)inData->getDataPtr(_indexField, AR_INT);
  void* theData = inData->getDataPtr(_dataField, _nodeDataType);
  const ARint len = inData->getDataDimension(_dataField) / _arrayStride;
  const ARint numberIDs = inData->getDataDimension(_indexField);
  
  if (numberIDs <= 0){
    // Must be at least one ID.
    ar_log_warning() << "arGraphicsArrayNode " << _typeString << ": no IDs.\n";
    // This isn't really an error... we were able to process the info.
    return true;
  }

  // Must lock before data modification.
  _nodeLock.lock();
  if (theIDs[0] == -1){
    // Array elements are packed in order.
    _mergeElements(len, theData);
  }
  else{
    _mergeElements(len, theData, theIDs);
  }
  
  // Bookkeeping.
  _commandBuffer.setType(_recordType);
  _nodeLock.unlock();
  return true;
}

// NOT thread-safe. Caller must lock node's lock.
// Methods like arPointsNode::setPoints and arGraphicsArrayNode::receiveData
// call this from a locked section, so we can't lock in here lest deadlocks ensue.
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

// This method is NOT thread-safe. It is the responsbility of the caller
// to lock the node's lock before calling. Already, various methods
// (like arGraphicsArrayNode::dumpData) call this from a locked section.
arStructuredData* arGraphicsArrayNode::_dumpData(int number,
                                                 void* elements,
						 int* IDs,
                                                 bool owned){
  arStructuredData* result =  owned ?
    _owningDatabase->getDataParser()->getStorage(_recordType) :
    _g->makeDataRecord(_recordType);
  if (!result){
    ar_log_warning() << "arGraphicsArrayNode failed to make record of type " <<
      _recordType << " (" << _g->_stringFromID(_recordType) << ").\n";
    return NULL;
  }

  _dumpGenericNode(result, _IDField);
  result->setDataDimension(_indexField, number);
  result->setDataDimension(_dataField, number*_arrayStride);
  if (IDs){
    result->dataIn(_indexField, IDs, AR_INT, number);
  }
  else{
    // Index field gets the identity map.
    ARint* dataIDs = (ARint*)result->getDataPtr(_indexField, AR_INT);
    for (int i=0; i<number; i++)
      dataIDs[i] = i;
  }
  // Fill in the data field. BUG: THIS RELIES ON THE FACT THAT ALL OUR DATA
  // TYPE HAVE THE SAME SIZE IN MEMORY (i.e. FLOAT AND INT)
  memcpy(result->getDataPtr(_dataField,_nodeDataType),
         elements, _arrayStride * number * arDataTypeSize(_nodeDataType));
  return result;
}
