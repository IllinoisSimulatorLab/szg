//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arViewerNode::arViewerNode(){
  // A sensible default name.
  _name = "viewer_node";
  _typeCode = AR_G_VIEWER_NODE;
  _typeString = "viewer";
}

arStructuredData* arViewerNode::dumpData(){
  return _dumpData(_head);
}

// NOTE: arViewerNode is a friend class of arHead
bool arViewerNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_VIEWER){
    cerr << "arViewerNode error: expected "
         << _g->AR_VIEWER
         << " (" << _g->_stringFromID(_g->AR_VIEWER) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  inData->dataOut(_g->AR_VIEWER_MATRIX, _head._matrix.v, AR_FLOAT, 16);
  inData->dataOut(_g->AR_VIEWER_MID_EYE_OFFSET, 
                  _head._midEyeOffset.v, AR_FLOAT, 3);
  inData->dataOut(_g->AR_VIEWER_EYE_DIRECTION, 
                  _head._eyeDirection.v, AR_FLOAT, 3);
  _head._eyeSpacing = inData->getDataFloat(_g->AR_VIEWER_EYE_SPACING);
  _head._nearClip = inData->getDataFloat(_g->AR_VIEWER_NEAR_CLIP);
  _head._farClip = inData->getDataFloat(_g->AR_VIEWER_FAR_CLIP);
  _head._unitConversion = inData->getDataFloat(_g->AR_VIEWER_UNIT_CONVERSION);
  _head._fixedHeadMode = inData->getDataInt(_g->AR_VIEWER_FIXED_HEAD_MODE);
  return true;
}

void arViewerNode::setHead(const arHead& head){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(head);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _head = head;
  }
}

// NOTE: arViewerNode is a friend class of arHead
arStructuredData* arViewerNode::_dumpData(const arHead& head){
  arStructuredData* result = _g->makeDataRecord(_g->AR_VIEWER);
  _dumpGenericNode(result,_g->AR_VIEWER_ID);
  if (!result->dataIn(_g->AR_VIEWER_MATRIX, head._matrix.v, AR_FLOAT, 16) 
      || !result->dataIn(_g->AR_VIEWER_MID_EYE_OFFSET, 
                         head._midEyeOffset.v, AR_FLOAT, 3) 
      || !result->dataIn(_g->AR_VIEWER_EYE_DIRECTION,
                         head._eyeDirection.v, AR_FLOAT, 3) 
      || !result->dataIn(_g->AR_VIEWER_EYE_SPACING,
                         &head._eyeSpacing, AR_FLOAT, 1) 
      || !result->dataIn(_g->AR_VIEWER_NEAR_CLIP,
                         &head._nearClip, AR_FLOAT, 1) 
      || !result->dataIn(_g->AR_VIEWER_FAR_CLIP,
                         &head._farClip, AR_FLOAT, 1) 
      || !result->dataIn(_g->AR_VIEWER_FIXED_HEAD_MODE,
                         &head._fixedHeadMode, AR_INT, 1) 
      || !result->dataIn(_g->AR_VIEWER_UNIT_CONVERSION,
                         &head._unitConversion, AR_FLOAT, 1)) {
    delete result;
    return NULL;
  }
  return result;
}
