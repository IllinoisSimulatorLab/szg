//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arVisibilityNode::arVisibilityNode():
  _visibility(false){
  _name = "visibility_node";
  // doesn't compile on RedHat 8 if these are not in the constructor body
  _typeCode = AR_G_VISIBILITY_NODE;
  _typeString = "visibility";
}

arStructuredData* arVisibilityNode::dumpData(){
  // Responsibility of the caller to delete this message.
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_visibility, false);
  _nodeLock.unlock();
  return r;
}

bool arVisibilityNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_VISIBILITY){
    cerr << "arVisibilityNode error: expected "
         << _g->AR_VISIBILITY
         << " (" << _g->_stringFromID(_g->AR_VISIBILITY) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  int vis = inData->getDataInt(_g->AR_VISIBILITY_VISIBILITY);
  _nodeLock.lock();
  _visibility = vis ? true : false;
  _nodeLock.unlock();
  return true;
}

bool arVisibilityNode::getVisibility(){
  _nodeLock.lock();
  bool r= _visibility;
  _nodeLock.unlock();
  return r;
}

void arVisibilityNode::setVisibility(bool visibility){
  if (active()){
    _nodeLock.lock();
    arStructuredData* r = _dumpData(visibility, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
    _visibility = visibility;
    _nodeLock.unlock();
  }
}

// NOT thread-safe.
arStructuredData* arVisibilityNode::_dumpData(bool visibility,
                                              bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result = getOwner()->getDataParser()->getStorage(_g->AR_VISIBILITY);
  }
  else{
    result = _g->makeDataRecord(_g->AR_VISIBILITY);
  }
  _dumpGenericNode(result,_g->AR_VISIBILITY_ID);
  int vis = visibility ? 1 : 0;
  if (!result->dataIn(_g->AR_VISIBILITY_VISIBILITY, &vis, AR_INT, 1)) {
    delete result;
    return NULL;
  }

  return result;
}


