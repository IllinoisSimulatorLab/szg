//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arGraphicsStateNode::arGraphicsStateNode(){
  // A sensible default name.
  _name = "graphics_state";
  _typeCode = AR_G_GRAPHICS_STATE_NODE;
  _typeString = "graphics state";
  _stateName = "garbage_state";
  _stateID = AR_G_GARBAGE_STATE;
  _stateValueInt[0] = AR_G_FALSE;
  _stateValueInt[1] = AR_G_FALSE;
  _stateValueFloat = 0;
}

arGraphicsStateNode::~arGraphicsStateNode(){
}

void arGraphicsStateNode::draw(arGraphicsContext*){
}

arStructuredData* arGraphicsStateNode::dumpData(){
  return _dumpData(_stateName, _stateValueInt, _stateValueFloat);
}

bool arGraphicsStateNode::receiveData(arStructuredData* data){
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(data)){
    return true;
  }
  if (data->getID() != _g->AR_GRAPHICS_STATE){
    cerr << "arGraphicsStateNode error: expected "
         << _g->AR_GRAPHICS_STATE
         << " (" << _g->_stringFromID(_g->AR_GRAPHICS_STATE) << "), not "
         << data->getID()
         << " (" << _g->_stringFromID(data->getID()) << ")\n";
    return false;
  }
  _stateName = data->getDataString(_g->AR_GRAPHICS_STATE_STRING);
  _stateID = _convertStringToStateID(_stateName);
  int d[2]; 
  data->dataOut(_g->AR_GRAPHICS_STATE_INT, d, AR_INT, 2);
  _stateValueInt[0] = (arGraphicsStateValue) d[0];
  _stateValueInt[1] = (arGraphicsStateValue) d[1];
  data->dataOut(_g->AR_GRAPHICS_STATE_FLOAT, &_stateValueFloat, AR_FLOAT, 1);
  return true;
}

string arGraphicsStateNode::getStateName(){
  return _stateName;
}

arGraphicsStateID arGraphicsStateNode::getStateID(){
  return _stateID;
}

bool arGraphicsStateNode::getStateValuesInt(arGraphicsStateValue& value1,
		                            arGraphicsStateValue& value2){
  if (_isFloatState()){
    return false;
  }
  value1 = _stateValueInt[0];
  value2 = _stateValueInt[1];
  return true;
}

bool arGraphicsStateNode::getStateValueFloat(float& value){
  if (!_isFloatState()){
    return false;
  }
  value = _stateValueFloat;
  return true;
}

bool arGraphicsStateNode::setGraphicsState(const string& stateName,
			                   arGraphicsStateValue* stateValueInt,
			                   float stateValueFloat){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(stateName, stateValueInt, stateValueFloat);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _stateName = stateName;
    _stateID = _convertStringToStateID(_stateName);
    if (stateValueInt){
      _stateValueInt[0] = stateValueInt[0];
      _stateValueInt[1] = stateValueInt[1];
    }
    else{
      // Sensible defaults.
      _stateValueInt[0] = AR_G_FALSE;
      _stateValueInt[1] = AR_G_FALSE;
    }
    _stateValueFloat = stateValueFloat;
  }
  return true;
}

// It doesn't matter that this is inefficient since it will likely be
// executed quite infrequently.
arGraphicsStateID arGraphicsStateNode::_convertStringToStateID(
						      const string& stateName){
  if (stateName == "garbage_state"){
    return AR_G_GARBAGE_STATE;
  }
  else if (stateName == "point_size"){
    return AR_G_POINT_SIZE;
  }
  else if (stateName == "line_width"){
    return AR_G_LINE_WIDTH;
  }
  else if (stateName == "shade_model"){
    return AR_G_SHADE_MODEL;
  }
  else if (stateName == "lighting"){
    return AR_G_LIGHTING;
  }
  else if (stateName == "blend"){
    return AR_G_BLEND;
  }
  else if (stateName == "depth_test"){
    return AR_G_DEPTH_TEST;
  }
  else if (stateName == "blend_func"){
    return AR_G_BLEND_FUNC;
  }
  return AR_G_GARBAGE_STATE;
}

bool arGraphicsStateNode::_isFloatState(){
  switch(_stateID){
  case AR_G_POINT_SIZE:
    return true;
  case AR_G_LINE_WIDTH:
    return true;
  default:
    return false;
  }
}

arStructuredData* arGraphicsStateNode::_dumpData(const string& stateName,
                                           arGraphicsStateValue* stateValueInt,
					   float stateValueFloat){
  arStructuredData* result = _g->makeDataRecord(_g->AR_GRAPHICS_STATE);
  _dumpGenericNode(result, _g->AR_GRAPHICS_STATE_ID);
  // Don't use the member variable. Instead, use the function parameter.
  result->dataInString(_g->AR_GRAPHICS_STATE_STRING, stateName);
  int data[2];
  // Don't use the member variable. Instead, use the function parameter.
  if (stateValueInt){
    data[0] = stateValueInt[0];
    data[1] = stateValueInt[1];
  }
  else{
    // Sensible defaults.
    data[0] = AR_G_FALSE;
    data[1] = AR_G_FALSE;
  }
  result->dataIn(_g->AR_GRAPHICS_STATE_INT, data, AR_INT, 2);
  // Don't use the member variable. Instead, use the function parameter.
  result->dataIn(_g->AR_GRAPHICS_STATE_FLOAT, &stateValueFloat, AR_FLOAT, 1);
  return result;
}
