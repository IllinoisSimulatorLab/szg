//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arVisibilityNode::arVisibilityNode():
  _visibility(false){
  // A sensible default node.
  _name = "visibility_node";
  // doesn't compile on RedHat 8 if these are not in the constructor body
  _typeCode = AR_G_VISIBILITY_NODE;
  _typeString = "visibility";
}

arStructuredData* arVisibilityNode::dumpData(){
  return _dumpData(_visibility);
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
  _visibility = vis ? true : false;
  return true;
}

bool arVisibilityNode::getVisibility(){
  return _visibility;
}

void arVisibilityNode::setVisibility(bool visibility){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(visibility);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _visibility = visibility;
  }
}

arStructuredData* arVisibilityNode::_dumpData(bool visibility){
  arStructuredData* result = _g->makeDataRecord(_g->AR_VISIBILITY);
  _dumpGenericNode(result,_g->AR_VISIBILITY_ID);
  int vis = visibility ? 1 : 0;
  if (!result->dataIn(_g->AR_VISIBILITY_VISIBILITY, &vis, AR_INT, 1)) {
    delete result;
    return NULL;
  }

  return result;
}


