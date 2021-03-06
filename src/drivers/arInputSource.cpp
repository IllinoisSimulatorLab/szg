//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputSource.h"

arInputSource::arInputSource() :
  _inputChannelID(0),
  _inputSink(NULL),
  _numberButtons(0),
  _numberAxes(0),
  _numberMatrices(0),
  _iAll(0),
  _iButton(0),
  _iAxis(0),
  _iMatrix(0)
{
  memset(_fComplained, 0, sizeof(_fComplained));
  _data = new arStructuredData(_inp.find("input"));
}

arInputSource::~arInputSource() {
  if (_data)
    delete _data;
}

void arInputSource::setInputNode(arInputSink* inputSink) {
  _inputSink = inputSink;
}

void arInputSource::sendButton(int i, int value) {
  if (!_data)
    return;
  if (i<0 || i>=_numberButtons) {
    if (!_fComplained[0][i]) {
      _fComplained[0][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range sendButton(" << i << ").\n";
    }
    return;
  }

  ARint theIndex = i;
  const ARint theType = AR_EVENT_BUTTON;
  if (!_fillCommonData(_data) ||
      !_data->dataIn("types", &theType, AR_INT, 1) || // only one item
      !_data->dataIn("indices", &theIndex, AR_INT, 1) ||
      !_data->dataIn("buttons", &value, AR_INT, 1) || // one button
      !_data->dataIn("axes", 0) || // no axes
      !_data->dataIn("matrices", 0)) { // no matrices
    ar_log_error() << "arInputSource: problem in sendButton.\n";
  }
  _sendData();
}

void arInputSource::sendAxis(int i, float value) {
  if (!_data)
    return;
  if (i<0 || i>=_numberAxes) {
    if (!_fComplained[1][i]) {
      _fComplained[1][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range sendAxis(" << i << ").\n";
    }
    return;
  }

  const ARint theIndex = i;
  const ARint theType = AR_EVENT_AXIS;
  const ARfloat theValue = ARfloat(value);
  if (!_fillCommonData(_data) ||
      !_data->dataIn("types", &theType, AR_INT, 1) ||
      !_data->dataIn("indices", &theIndex, AR_INT, 1) ||
      !_data->dataIn("buttons", 0) ||
      !_data->dataIn("axes", &theValue, AR_FLOAT, 1) ||
      !_data->dataIn("matrices", 0)) {
    ar_log_error() << "arInputSource: problem in sendAxis.\n";
  }
  _sendData();
}

void arInputSource::sendMatrix(int i, const arMatrix4& value) {
  if (!_data)
    return;
  if (i<0 || i>=_numberMatrices) {
    if (!_fComplained[2][i]) {
      _fComplained[2][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range sendMatrix(" << i << ").\n";
    }
    return;
  }

  ARint theIndex = i;
  const ARint theType = AR_EVENT_MATRIX;
  if (!_fillCommonData(_data) ||
      !_data->dataIn("types", &theType, AR_INT, 1) ||
      !_data->dataIn("indices", &theIndex, AR_INT, 1) ||
      !_data->dataIn("buttons", 0) ||
      !_data->dataIn("axes", 0) ||
      !_data->dataIn("matrices", value.v, AR_FLOAT, 16)) {
    ar_log_error() << "arInputSource: problem in sendMatrix.\n";
  }
  _sendData();
}

// todo: actually call this function from somewhere.

void arInputSource::sendButtonsAxesMatrices(
  int numButtons, const int* rgiButtons, const int* rgvalueButtons,
  int numAxes, const int* rgiAxes, const float* rgvalueAxes,
  int numMatrices, const int* rgiMatrices, const arMatrix4* rgvalueMatrices) {

  if (!_data)
    return;
  const int numThings = numButtons + numAxes + numMatrices;
  if (numThings > _numThingsMax) {
    _numThingsMax = numThings;
    if (_theIndices)
      delete [] _theIndices;
    if (_theTypes)
      delete [] _theTypes;
    _theIndices = new ARint[numThings];
    _theTypes = new ARint[numThings];
  }

  // Pack the arrays with first buttons, then axes, then matrices.
  int iAll = 0;
  int i =0 ;
  for (i=0; i<numButtons; i++) {
    _theTypes[iAll] = AR_EVENT_BUTTON;
    _theIndices[iAll++] = rgiButtons ? rgiButtons[i] : i;
    }
  for (i=0; i<numAxes; i++) {
    _theTypes[iAll] = AR_EVENT_AXIS;
    _theIndices[iAll++] = rgiAxes ? rgiAxes[i] : i;
    }
  for (i=0; i<numMatrices; i++) {
    _theTypes[iAll] = AR_EVENT_MATRIX;
    _theIndices[iAll++] = rgiMatrices ? rgiMatrices[i] : i;
    }

  if (iAll != numThings)
    ar_log_error() << "arInputSource: numThings miscounted.\n";

  if (!_fillCommonData(_data) ||
      !_data->dataIn("types", _theTypes, AR_INT, numThings) ||
      !_data->dataIn("indices", _theIndices, AR_INT, numThings) ||
      !_data->dataIn("buttons", rgvalueButtons, AR_INT, numButtons) ||
      !_data->dataIn("axes", rgvalueAxes, AR_FLOAT, numAxes) ||
      !_data->dataIn("matrices", rgvalueMatrices, AR_FLOAT, 16*numMatrices)) {
    ar_log_error() << "arInputSource: dataIn failed in sendButtonsAxesMatrices.\n";
  }
  _sendData();
}

void arInputSource::queueButton(int i, int value) {
  if (i<0 || i>=_numberButtons) {
    if (!_fComplained[0][i]) {
      _fComplained[0][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range queueButton(" << i << ").\n";
    }
    return;
  }

  _types[_iAll] = AR_EVENT_BUTTON;
  _indices[_iAll++] = i;
  _buttons[_iButton++] = value;
}

void arInputSource::queueAxis(int i, float value) {
  if (i<0 || i>=_numberAxes) {
    if (!_fComplained[1][i]) {
      _fComplained[1][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range queueAxis(" << i << ").\n";
    }
    return;
  }

  _types[_iAll] = AR_EVENT_AXIS;
  _indices[_iAll++] = i;
  _axes[_iAxis++] = ARfloat(value);
}

void arInputSource::queueMatrix(int i, const arMatrix4& value) {
  if (i<0 || i>=_numberMatrices) {
    if (!_fComplained[2][i]) {
      _fComplained[2][i] = true;
      ar_log_error() << "arInputSource ignoring out-of-range queueMatrix(" << i << ").\n";
    }
    return;
  }

  _types[_iAll] = AR_EVENT_MATRIX;
  _indices[_iAll++] = i;
  _matrices[_iMatrix++] = value;
}

void arInputSource::sendQueue() {
  if (!_data || _iAll <= 0)
    return;

  if (!_fillCommonData(_data) ||
      !_data->dataIn("types", _types, AR_INT, _iAll) ||
      !_data->dataIn("indices", _indices, AR_INT, _iAll) ||
      !_data->dataIn("buttons", _buttons, AR_INT, _iButton) ||
      !_data->dataIn("axes", _axes, AR_FLOAT, _iAxis) ||
      !_data->dataIn("matrices", _matrices, AR_FLOAT, 16*_iMatrix)) {
    ar_log_error() << "arInputSource: problem in sendQueue.\n";
  }
  _sendData();
  _iAll = _iButton = _iAxis = _iMatrix = 0;
}

void arInputSource::_setDeviceElements(int buttons, int axes, int matrices) {
  if (buttons<0) {
    ar_log_error() << "arInputSource zeroing negative number of buttons.\n";
    buttons = 0;
  }
  if (axes<0) {
    ar_log_error() << "arInputSource zeroing negative number of axes.\n";
    axes = 0;
  }
  if (matrices<0) {
    ar_log_error() << "arInputSource zeroing negative number of matrices.\n";
    matrices = 0;
  }

  if (buttons>_maxSize-2) {
    ar_log_error() << "arInputSource truncating excessive number of buttons to " <<
      _maxSize-2 << ".\n";
    buttons = _maxSize-2;
  }
  if (axes>_maxSize-2) {
    ar_log_error() << "arInputSource truncating excessive number of axes to " <<
      _maxSize-2 << ".\n";
    axes = _maxSize-2;
  }
  if (matrices>_maxSize-2) {
    ar_log_error() << "arInputSource truncating excessive number of matrices to " <<
      _maxSize-2 << ".\n";
    matrices = _maxSize-2;
  }

  _numberButtons = buttons;
  _numberAxes = axes;
  _numberMatrices = matrices;
}

bool arInputSource::_fillCommonData(arStructuredData* d) {
  const ARint signature[3] = { _numberButtons, _numberAxes, _numberMatrices };
  const ar_timeval t(ar_time());
  const ARint theTime[2] = { t.sec, t.usec };
  return d->dataIn("signature", signature, AR_INT, 3) &&
         d->dataIn("timestamp", theTime, AR_INT, 2);
}

void arInputSource::_sendData(arStructuredData* theData) {
  if (!_inputSink) {
    ar_log_error() << "arInputSource: undefined input sink.\n";
    return;
  }

  if (theData) {
    // send the data we've been given instead of the internal data
    _inputSink->receiveData(_inputChannelID, theData);
  }
  else if (_data) {
    _inputSink->receiveData(_inputChannelID, _data);
  }
}

bool arInputSource::_reconfig() {
  if (!_inputSink) {
    ar_log_error() << "arInputSource: undefined input sink.\n";
    return false;
  }
  return _inputSink->sourceReconfig(_inputChannelID);
}

void arInputSource::_setInputSink(int id, arInputSink* sink) {
  if (sink)
    _inputSink = sink;
  else
    ar_log_error() << "arInputSource ignoring NULL input sink.\n";

  if (id >= 0)
    _inputChannelID = id;
  else
    ar_log_error() << "arInputSource ignoring negative input channel id.\n";
}
