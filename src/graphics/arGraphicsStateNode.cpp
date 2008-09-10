//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arGraphicsStateNode::arGraphicsStateNode() {
  _name = "graphics_state";
  _typeCode = AR_G_GRAPHICS_STATE_NODE;
  _typeString = "graphics state";
  _stateName = "garbage_state";
  _stateID = AR_G_GARBAGE_STATE;
  _stateValueInt[0] = AR_G_FALSE;
  _stateValueInt[1] = AR_G_FALSE;
  _stateValueFloat = 0;
}

arGraphicsStateNode::~arGraphicsStateNode() {
}

void arGraphicsStateNode::draw(arGraphicsContext*) {
}

bool arGraphicsStateNode::receiveData(arStructuredData* data) {
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(data)) {
    return true;
  }
  if (!_g->checkNodeID(_g->AR_GRAPHICS_STATE, data->getID(), "arGraphicsStateNode")) {
    return false;
  }

  arGuard _(_nodeLock, "arGraphicsStateNode::receiveData");
  _stateName = data->getDataString(_g->AR_GRAPHICS_STATE_STRING);
  _stateID = _convertStringToStateID(_stateName);
  int d[2];
  data->dataOut(_g->AR_GRAPHICS_STATE_INT, d, AR_INT, 2);
  _stateValueInt[0] = (arGraphicsStateValue) d[0];
  _stateValueInt[1] = (arGraphicsStateValue) d[1];
  data->dataOut(_g->AR_GRAPHICS_STATE_FLOAT, &_stateValueFloat, AR_FLOAT, 1);
  return true;
}

string arGraphicsStateNode::getStateName() {
  arGuard _(_nodeLock, "arGraphicsStateNode::getStateName");
  return _stateName;
}

arGraphicsStateID arGraphicsStateNode::getStateID() {
  arGuard _(_nodeLock, "arGraphicsStateNode::getStateID");
  return _stateID;
}

bool arGraphicsStateNode::isFloatState() {
  arGuard _(_nodeLock, "arGraphicsStateNode::isFloatState");
  // _isFloatState uses the node's current state.
  return _isFloatState();
}

bool arGraphicsStateNode::isFloatState(const string& stateName) {
  return _checkFloatState(_convertStringToStateID(stateName));
}

bool arGraphicsStateNode::getStateValuesInt(arGraphicsStateValue& value1,
                                            arGraphicsStateValue& value2) {
  arGuard _(_nodeLock, "arGraphicsStateNode::getStateValuesInt");
  if (_isFloatState()) {
    return false;
  }
  value1 = _stateValueInt[0];
  value2 = _stateValueInt[1];
  return true;
}

arGraphicsStateValue arGraphicsStateNode::getStateValueInt(int i) {
  arGraphicsStateValue v1, v2;
  bool success = getStateValuesInt(v1, v2);
  if (success) {
    if (i == 0) {
      return v1;
    }
    if (i == 1) {
      return v2;
    }
    return AR_G_FALSE;
  }
  return AR_G_FALSE;
}

string arGraphicsStateNode::getStateValueString(int i) {
  arGraphicsStateValue v1, v2;
  return !getStateValuesInt(v1, v2) ? "false" :
    (i == 0) ? _convertStateValueToString(v1) :
    (i == 1) ? _convertStateValueToString(v2) : 
    "false";
}

bool arGraphicsStateNode::getStateValueFloat(float& value) {
  arGuard _(_nodeLock, "arGraphicsStateNode::getStateValueFloat");
  if (!_isFloatState()) {
    return false;
  }
  value = _stateValueFloat;
  return true;
}

float arGraphicsStateNode::getStateValueFloat() {
  float result = 0;
  (void) getStateValueFloat(result);
  return result;
}

bool arGraphicsStateNode::setGraphicsStateInt( const string& stateName,
                                               arGraphicsStateValue v1,
                                               arGraphicsStateValue v2) {
  arGraphicsStateID id = _convertStringToStateID(stateName);
  if (_checkFloatState(id)) {
    ar_log_error() << "arGraphicsStateNode ignoring setGraphicsStateInt() for float type.\n";
    return false;
  }
  if (_owningDatabase) {
    arGraphicsStateValue stateValueInt[2] = { v1, v2 };
    _nodeLock.lock("arGraphicsStateNode::setGraphicsStateInt owned");
    arStructuredData* r = _dumpData(stateName, stateValueInt, _stateValueFloat, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else {
    arGuard _(_nodeLock, "arGraphicsStateNode::setGraphicsStateInt unowned");
    _stateValueFloat = -1.;
    _stateName = stateName;
    _stateID = id;
    _stateValueInt[0] = v1;
    _stateValueInt[1] = v2;
  }
  return true;
}

bool arGraphicsStateNode::setGraphicsStateString( const string& stateName,
                                                  const string& value1,
                                                  const string& value2) {
  arGraphicsStateValue v1 = _convertStringToStateValue(value1);
  arGraphicsStateValue v2 = _convertStringToStateValue(value2);
  return setGraphicsStateInt( stateName, v1, v2 );
}

bool arGraphicsStateNode::setGraphicsStateFloat(const string& stateName,
                                                float stateValueFloat) {
  arGraphicsStateID id = _convertStringToStateID(stateName);
  if (!_checkFloatState(id)) {
    ar_log_error() << "arGraphicsStateNode ignoring setGraphicsStateFloat() for int type.\n";
    return false;
  }
  if (_owningDatabase) {
    _nodeLock.lock("arGraphicsStateNode::setGraphicsStateFloat owned");
    arStructuredData* r = _dumpData(stateName, NULL, stateValueFloat, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    delete r;
  }
  else {
    arGuard _(_nodeLock, "arGraphicsStateNode::setGraphicsStateFloat unowned");
    _stateName = stateName;
    _stateID = id;
    // Defaults.
    _stateValueInt[0] = AR_G_FALSE;
    _stateValueInt[1] = AR_G_FALSE;
    _stateValueFloat = stateValueFloat;
  }
  return true;
}

// Inefficient, but rarely called.
arGraphicsStateID arGraphicsStateNode::_convertStringToStateID(
  const string& stateName) {
  if (stateName == "garbage_state") {
    return AR_G_GARBAGE_STATE;
  }
  else if (stateName == "point_size") {
    return AR_G_POINT_SIZE;
  }
  else if (stateName == "line_width") {
    return AR_G_LINE_WIDTH;
  }
  else if (stateName == "shade_model") {
    return AR_G_SHADE_MODEL;
  }
  else if (stateName == "lighting") {
    return AR_G_LIGHTING;
  }
  else if (stateName == "blend") {
    return AR_G_BLEND;
  }
  else if (stateName == "depth_test") {
    return AR_G_DEPTH_TEST;
  }
  else if (stateName == "blend_func") {
    return AR_G_BLEND_FUNC;
  }
  return AR_G_GARBAGE_STATE;
}

string arGraphicsStateNode::_convertStateIDToString(arGraphicsStateID id) {
  switch (id) {
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
                                                     const string& stateValue) {
  if (stateValue == "false") {
    return AR_G_FALSE;
  }
  else if (stateValue == "true") {
    return AR_G_TRUE;
  }
  else if (stateValue == "smooth") {
    return AR_G_SMOOTH;
  }
  else if (stateValue == "flat") {
    return AR_G_FLAT;
  }
  else if (stateValue == "zero") {
    return AR_G_ZERO;
  }
  else if (stateValue == "one") {
    return AR_G_ONE;
  }
  else if (stateValue == "dst_color") {
    return AR_G_DST_COLOR;
  }
  else if (stateValue == "src_color") {
    return AR_G_SRC_COLOR;
  }
  else if (stateValue == "one_minus_dst_color") {
    return AR_G_ONE_MINUS_DST_COLOR;
  }
  else if (stateValue == "one_minus_src_color") {
    return AR_G_ONE_MINUS_SRC_COLOR;
  }
  else if (stateValue == "src_alpha") {
    return AR_G_SRC_ALPHA;
  }
  else if (stateValue == "one_minus_src_alpha") {
    return AR_G_ONE_MINUS_SRC_ALPHA;
  }
  else if (stateValue == "dst_alpha") {
    return AR_G_DST_ALPHA;
  }
  else if (stateValue == "one_minus_dst_alpha") {
    return AR_G_ONE_MINUS_DST_ALPHA;
  }
  else if (stateValue == "src_alpha_saturate") {
    return AR_G_SRC_ALPHA_SATURATE;
  }
  return AR_G_FALSE;
}

string arGraphicsStateNode::_convertStateValueToString(arGraphicsStateValue v) {
  switch (v) {
  default:
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
  }
}

// Thread-safe. DO NOT use the _nodeLock in here (can be called from
// inside that lock).
bool arGraphicsStateNode::_checkFloatState( arGraphicsStateID id ) const {
  switch (id) {
  case AR_G_POINT_SIZE:
  case AR_G_LINE_WIDTH:
    return true;
  default:
    return false;
  }
}

arStructuredData* arGraphicsStateNode::dumpData() {
  arGuard _(_nodeLock, "arGraphicsStateNode::dumpData");
  return _dumpData(_stateName, _stateValueInt, _stateValueFloat, false);
}

arStructuredData* arGraphicsStateNode::_dumpData(
    const string& stateName,
    arGraphicsStateValue* stateValueInt,
    float stateValueFloat,
    bool owned ) {
  arStructuredData* r = _getRecord(owned, _g->AR_GRAPHICS_STATE);
  _dumpGenericNode(r, _g->AR_GRAPHICS_STATE_ID);

  // Use the function arg, not the member variable.
  r->dataInString(_g->AR_GRAPHICS_STATE_STRING, stateName);

  // Use the function arg, not the member variable.
  int data[2];
  if (stateValueInt) {
    data[0] = stateValueInt[0];
    data[1] = stateValueInt[1];
  }
  else{
    // Defaults.
    data[0] = AR_G_FALSE;
    data[1] = AR_G_FALSE;
  }
  // Use the function arg, not the member variable.
  if (!r->dataIn(_g->AR_GRAPHICS_STATE_INT, data, AR_INT, 2) ||
      !r->dataIn(_g->AR_GRAPHICS_STATE_FLOAT, &stateValueFloat, AR_FLOAT, 1)) {
    delete r;
    return NULL;
  }

  return r;
}
