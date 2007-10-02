//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arVisibilityNode::arVisibilityNode():
  _visibility(false){
  _name = "visibility_node";
  // RedHat 8 fails to compile if these are initializers.
  _typeCode = AR_G_VISIBILITY_NODE;
  _typeString = "visibility";
}

bool arVisibilityNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_VISIBILITY, inData->getID(), "arVisibilityNode"))
    return false;

  const bool vis = inData->getDataInt(_g->AR_VISIBILITY_VISIBILITY) ? true : false;
  arGuard dummy(_nodeLock);
  _visibility = vis;
  return true;
}

bool arVisibilityNode::getVisibility(){
  arGuard dummy(_nodeLock);
  return _visibility;
}

void arVisibilityNode::setVisibility(bool visibility){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(visibility, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _visibility = visibility;
  }
}

arStructuredData* arVisibilityNode::dumpData(){
  arGuard dummy(_nodeLock);
  return _dumpData(_visibility, false);
}

arStructuredData* arVisibilityNode::_dumpData(bool visibility, bool owned){
  arStructuredData* r = _getRecord(owned, _g->AR_VISIBILITY);
  _dumpGenericNode(r, _g->AR_VISIBILITY_ID);
  const int vis = visibility ? 1 : 0;
  if (!r->dataIn(_g->AR_VISIBILITY_VISIBILITY, &vis, AR_INT, 1)) {
    delete r;
    return NULL;
  }

  return r;
}
