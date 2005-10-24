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
  // Caller is responsible for deleting.
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r 
    = _dumpData(_stateName, _stateValueInt, _stateValueFloat, false);
  ar_mutex_unlock(&_nodeLock);
  return r;
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
  ar_mutex_lock(&_nodeLock);
  _stateName = data->getDataString(_g->AR_GRAPHICS_STATE_STRING);
  _stateID = _convertStringToStateID(_stateName);
  int d[2]; 
  data->dataOut(_g->AR_GRAPHICS_STATE_INT, d, AR_INT, 2);
  _stateValueInt[0] = (arGraphicsStateValue) d[0];
  _stateValueInt[1] = (arGraphicsStateValue) d[1];
  data->dataOut(_g->AR_GRAPHICS_STATE_FLOAT, &_stateValueFloat, AR_FLOAT, 1);
  ar_mutex_unlock(&_nodeLock);
  return true;
}

string arGraphicsStateNode::getStateName(){
  ar_mutex_lock(&_nodeLock);
  string r = _stateName;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

arGraphicsStateID arGraphicsStateNode::getStateID(){
  ar_mutex_lock(&_nodeLock);
  arGraphicsStateID r = _stateID;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

bool arGraphicsStateNode::isFloatState(){
  return _isFloatState();
}

bool arGraphicsStateNode::isFloatState(const string& stateName){
  return _checkFloatState(_convertStringToStateID(stateName));
}

bool arGraphicsStateNode::getStateValuesInt(arGraphicsStateValue& value1,
		                            arGraphicsStateValue& value2){
  ar_mutex_lock(&_nodeLock);
  if (_isFloatState()){
    ar_mutex_unlock(&_nodeLock);
    return false;
  }
  value1 = _stateValueInt[0];
  value2 = _stateValueInt[1];
  ar_mutex_unlock(&_nodeLock);
  return true;
}

arGraphicsStateValue arGraphicsStateNode::getStateValueInt(int i){
  arGraphicsStateValue v1, v2;
  bool success = getStateValuesInt(v1, v2);
  if (success){
    if (i == 0){
      return v1;
    }
    if (i == 1){
      return v2;
    }
    return AR_G_FALSE;
  }
  return AR_G_FALSE;
}

string arGraphicsStateNode::getStateValueString(int i){
  arGraphicsStateValue v1, v2;
  bool success = getStateValuesInt(v1, v2);
  if (success){
    if (i == 0){
      return _convertStateValueToString(v1);
    }
    if (i == 1){
      return _convertStateValueToString(v2);
    }
    return "false";
  }
  return "false";
}

bool arGraphicsStateNode::getStateValueFloat(float& value){
  ar_mutex_lock(&_nodeLock);
  if (!_isFloatState()){
    ar_mutex_unlock(&_nodeLock);
    return false;
  }
  value = _stateValueFloat;
  ar_mutex_unlock(&_nodeLock);
  return true;
}

float arGraphicsStateNode::getStateValueFloat(){
  float result = 0;
  (void) getStateValueFloat(result);
  return result;
}

bool arGraphicsStateNode::setGraphicsStateInt( const string& stateName,
                                               arGraphicsStateValue value1, 
                                               arGraphicsStateValue value2) {
  arGraphicsStateID id = _convertStringToStateID(stateName);
  if (_checkFloatState(id)) {
    cerr << "arGraphicsStateNode error: attempt to setGraphicsStateInt() for float state node type.\n";
    return false;
  }
  if (_owningDatabase) {
    arGraphicsStateValue stateValueInt[2];
    stateValueInt[0] = value1;
    stateValueInt[1] = value2;
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(stateName, stateValueInt, _stateValueFloat, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else {
    ar_mutex_lock(&_nodeLock);
    _stateValueFloat = -1.;
    _stateName = stateName;
    _stateID = id;
    _stateValueInt[0] = value1;
    _stateValueInt[1] = value2;
    ar_mutex_unlock(&_nodeLock);
  }
  return true;
}

bool arGraphicsStateNode::setGraphicsStateString( const string& stateName,
						  const string& value1,
						  const string& value2){
  arGraphicsStateValue v1 = _convertStringToStateValue(value1);
  arGraphicsStateValue v2 = _convertStringToStateValue(value2);
  return setGraphicsStateInt( stateName, v1, v2 );
}

bool arGraphicsStateNode::setGraphicsStateFloat(const string& stateName,
                                                float stateValueFloat) {
  arGraphicsStateID id = _convertStringToStateID(stateName);
  if (!_checkFloatState(id)) {
    cerr << "arGraphicsStateNode error: attempt to setGraphicsStateFloat() for int state node type.\n";
    return false;
  }
  if (_owningDatabase) {
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(stateName, NULL, _stateValueFloat, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    delete r;
  }
  else {
    ar_mutex_lock(&_nodeLock);
    _stateName = stateName;
    _stateID = id;
    // Sensible defaults.
    _stateValueInt[0] = AR_G_FALSE;
    _stateValueInt[1] = AR_G_FALSE;
    _stateValueFloat = stateValueFloat;
    ar_mutex_unlock(&_nodeLock);
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

string arGraphicsStateNode::_convertStateIDToString(arGraphicsStateID id){
  switch (id){
  case AR_G_GARBAGE_STATE:
    return "garbage_state";
  case AR_G_POINT_SIZE:
    return "point_size";
  case AR_G_LINE_WIDTH:
    return "line_width";
  case AR_G_SHADE_MODEL:
    return "shade_model";
  case AR_G_LIGHTING:
    return "lighting";
  case AR_G_BLEND:
    return "blend";
  case AR_G_DEPTH_TEST:
    return "depth_test";
  case AR_G_BLEND_FUNC:
    return "blend_func";
  default:
    return "garbage_state";
  }
}

arGraphicsStateValue arGraphicsStateNode::_convertStringToStateValue(
						     const string& stateValue){
  if (stateValue == "false"){
    return AR_G_FALSE;
  }
  else if (stateValue == "true"){
    return AR_G_TRUE;
  }
  else if (stateValue == "smooth"){
    return AR_G_SMOOTH;
  }
  else if (stateValue == "flat"){
    return AR_G_FLAT;
  }
  else if (stateValue == "zero"){
    return AR_G_ZERO;
  }
  else if (stateValue == "one"){
    return AR_G_ONE;
  }
  else if (stateValue == "dst_color"){
    return AR_G_DST_COLOR;
  }
  else if (stateValue == "src_color"){
    return AR_G_SRC_COLOR;
  }
  else if (stateValue == "one_minus_dst_color"){
    return AR_G_ONE_MINUS_DST_COLOR;
  }
  else if (stateValue == "one_minus_src_color"){
    return AR_G_ONE_MINUS_SRC_COLOR;
  }
  else if (stateValue == "src_alpha"){
    return AR_G_SRC_ALPHA;
  }
  else if (stateValue == "one_minus_src_alpha"){
    return AR_G_ONE_MINUS_SRC_ALPHA;
  }
  else if (stateValue == "dst_alpha"){
    return AR_G_DST_ALPHA;
  }
  else if (stateValue == "one_minus_dst_alpha"){
    return AR_G_ONE_MINUS_DST_ALPHA;
  }
  else if (stateValue == "src_alpha_saturate"){
    return AR_G_SRC_ALPHA_SATURATE;
  }
  return AR_G_FALSE;
}

string arGraphicsStateNode::_convertStateValueToString(arGraphicsStateValue v){
  switch (v){
  case AR_G_FALSE:
    return "false";
  case AR_G_TRUE:
    return "true";
  case AR_G_SMOOTH:
    return "smooth";
  case AR_G_FLAT:
    return "flat";
  case AR_G_ZERO:
    return "zero";
  case AR_G_ONE:
    return "one";
  case AR_G_DST_COLOR:
    return "dst_color";
  case AR_G_SRC_COLOR:
    return "src_color";
  case AR_G_ONE_MINUS_DST_COLOR:
    return "one_minus_dst_color";
  case AR_G_ONE_MINUS_SRC_COLOR:
    return "one_minus_src_color";
  case AR_G_SRC_ALPHA:
    return "src_alpha";
  case AR_G_ONE_MINUS_SRC_ALPHA:
    return "one_minus_src_alpha";
  case AR_G_DST_ALPHA:
    return "dst_alpha";
  case AR_G_ONE_MINUS_DST_ALPHA:
    return "one_minus_dst_alpha";
  case AR_G_SRC_ALPHA_SATURATE:
    return "src_alpha_saturate";
  default:
    return "false";
  }
}

bool arGraphicsStateNode::_checkFloatState( arGraphicsStateID id ) {
  switch (id) {
  case AR_G_POINT_SIZE:
    return true;
  case AR_G_LINE_WIDTH:
    return true;
  default:
    return false;
  }
}

/// NOT thread-safe.
arStructuredData* arGraphicsStateNode::_dumpData(const string& stateName,
                                         arGraphicsStateValue* stateValueInt,
                                         float stateValueFloat,
                                         bool owned ) {
  arStructuredData* result = NULL;
  if (owned){
    result = getOwner()->getDataParser()->getStorage(_g->AR_GRAPHICS_STATE);
  }
  else{
    result = _g->makeDataRecord(_g->AR_GRAPHICS_STATE);
  }
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
