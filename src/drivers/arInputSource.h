//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_SOURCE_H
#define AR_INPUT_SOURCE_H

#include "arSZGClient.h"
// NOTE: These items can be shared objects
#include "arCallingConventions.h"
#include "arInputSink.h"
#include "arIOFilter.h"
#include "arInputLanguage.h"
#include "arInputEvent.h"

class arInputNode;

/// Something which emits input-device messages.

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

#ifdef UNUSED
  void setFilter(arIOFilter*);
#endif

  int getNumberButtons() const;
  int getNumberAxes() const;
  int getNumberMatrices() const;

  /// \todo how do you do groups in doxygen???
  /// \name Send a single item.
  //@(
  void sendButton(int index, int value);
  void sendAxis(int index, float value);
  void sendMatrix(const arMatrix4& value)
    { sendMatrix(0, value); }
  void sendMatrix(int index, const arMatrix4& value);
  //@)

  /// \name Send several items as a single packet.
  //@(
  void sendButtonsAxesMatrices(
    int numButtons, const int* rgiButtons, const int* rgvalueButtons,
    int numAxes, const int* rgiAxes, const float* rgvalueAxes,
    int numMatrices, const int* rgiMatrices, const arMatrix4* rgvalueMatrices);
  //@)

  /// \name Accumulate several values and then send them as a single packet.
  //@(
  void queueButton(int index, int value);
  void queueAxis(int index, float value);
  void queueMatrix(const arMatrix4& value)
    { queueMatrix(0, value); }
  void queueMatrix(int index, const arMatrix4& value);
  void sendQueue();
  //@)

 protected:
  arInputLanguage _inp;
  int _inputChannelID;
  arInputSink* _inputSink;
  arIOFilter* _filter;

  arStructuredData* _data;

  int _numberButtons;
  int _numberAxes;
  int _numberMatrices;

  // buffers for accumulating queueXXX() data
  enum { _maxSize = 256 }; // oh no! fixed size!!!
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
  // a utility function
  bool _fillCommonData(arStructuredData*);
  // the filtration function
//  arStructuredData* _filterData(arStructuredData*);
  // send the data to the input sink
  void _sendData(arStructuredData* theData = NULL);
  // send a reconfigure message to the input sink
  bool _reconfig();
  // need to be able to register input sink objects... plus be able to send to
  // them on a particular channel (which is useful for aggregating multiple
  // input devices into a single virtual input device
  void _setInputSink(int, arInputSink*);
};

#endif
