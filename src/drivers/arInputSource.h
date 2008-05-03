//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_SOURCE_H
#define AR_INPUT_SOURCE_H

#include "arInputSink.h"
#include "arInputLanguage.h"
#include "arInputEvent.h"
#include "arDriversCalling.h"

class arInputNode;

// Produce input-device messages.

class SZG_CALL arInputSource{
  // Needs assignment operator and copy constructor, for pointer members.
  friend class arInputNode;
 public:
  arInputSource();
  virtual ~arInputSource();

  void setInputNode(arInputSink*);

  virtual bool init(arSZGClient&)
    { return true; }
  virtual bool start()
    { return true; }
  virtual bool stop()
    { return true; }
  virtual bool restart()
    { return stop() && start(); }

  int getNumberButtons() const
    { return _numberButtons; }
  int getNumberAxes() const
    { return _numberAxes; }
  int getNumberMatrices() const
    { return _numberMatrices; }

  // Send one item.
  void sendButton(int index, int value);
  void sendAxis(int index, float value);
  void sendMatrix(int index, const arMatrix4& value);
  void sendMatrix(const arMatrix4& value) { sendMatrix(0, value); }

  // Send several items in one packet.
  void sendButtonsAxesMatrices(
    int numButtons, const int* rgiButtons, const int* rgvalueButtons,
    int numAxes, const int* rgiAxes, const float* rgvalueAxes,
    int numMatrices, const int* rgiMatrices, const arMatrix4* rgvalueMatrices);

  // Accumulate items.
  void queueButton(int index, int value);
  void queueAxis(int index, float value);
  void queueMatrix(int index, const arMatrix4& value);
  void queueMatrix(const arMatrix4& value) { queueMatrix(0, value); }
  // Send accumulated items in one packet.
  void sendQueue();

  virtual void handleMessage( const string& /*messageType*/, const string& /*messageBody*/ ) {}

 protected:
  arInputLanguage _inp;
  int _inputChannelID;
  arInputSink* _inputSink;

  arStructuredData* _data;

  int _numberButtons;
  int _numberAxes;
  int _numberMatrices;

  // buffers for accumulating queueXXX() data
  enum { _maxSize = 256 }; // bug: several fixed length buffers
  ARint _types[3*_maxSize];
  ARint _indices[3*_maxSize];
  ARint _buttons[_maxSize];
  ARfloat _axes[_maxSize];
  arMatrix4 _matrices[_maxSize];
  int _iAll, _iButton, _iAxis, _iMatrix;
  ARint* _theIndices;
  ARint* _theTypes;
  int _numThingsMax;

  // init() calls this to set the device signature.
  void _setDeviceElements(int buttons, int axes, int matrices);
  void _setDeviceElements(const ARint* nums)
    { _setDeviceElements(nums[0], nums[1], nums[2]); }

  bool _fillCommonData(arStructuredData*);

  // Send data to the input sink.
  void _sendData(arStructuredData* theData = NULL);

  // Send a reconfigure message to the input sink.
  bool _reconfig();

  // To register input sinks, and send them on a particular channel
  // (to aggregate multiple input devices).
  void _setInputSink(int, arInputSink*);

 private:
  bool _fComplained[3][_maxSize];
};

#endif
