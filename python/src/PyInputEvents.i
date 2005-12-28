//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

%{
#include "arInputEvent.h"
#include "arInputState.h"
#include "arInputEventQueue.h"

PyObject* ar_inputEventToDict( arInputEvent& event ) {
  PyObject* dict = PyDict_New();
  if (!dict) {
    PyErr_SetString( PyExc_TypeError, "arInputEvent error: failed to allocate dictionary." );
    return NULL;
  }
  int theType = event.getType();
  std::string typeString;
  PyObject* value;
  switch (theType) {
    case AR_EVENT_MATRIX:
      typeString = "matrix";
      value = ar_matrix4ToTuple( event.getMatrix() );
      break;
    case AR_EVENT_AXIS:
      typeString = "axis";
      value = PyFloat_FromDouble( (double)event.getAxis() );
      break;
    case AR_EVENT_BUTTON:
      typeString = "button";
      value = PyInt_FromLong( (long)event.getButton() );
      break;
    default:
      return dict;
   }
  PyObject* pIndex = PyInt_FromLong( (long)event.getIndex() );
  // Amazingly, some versions of Python *do not* have const char* as the
  // second parameter below & barf when passed typeString.c_str(). Consequently, the copy.
  char* tmp = new char[typeString.length()+1];
  if (!tmp) {
    PyErr_SetString( PyExc_TypeError, "arInputEvent error: failed to allocate temp string buffer." );
    return NULL;
  }
  strcpy(tmp, typeString.c_str());
  if ((PyDict_SetItemString( dict, tmp, value ) == -1) ||
       (PyDict_SetItemString( dict, "index", pIndex ) == -1)) {
    PyErr_SetString( PyExc_TypeError, "arInputEvent error: failed to populate dictionary." );
    // Must delete the temporary string buffer.
    delete [] tmp;
    return NULL;
  }
  // Must delete the temporary string buffer.
  delete [] tmp;
  Py_XDECREF(value);
  Py_XDECREF(pIndex);
  return dict;
}
%}

enum arInputEventType {AR_EVENT_GARBAGE=-1, AR_EVENT_BUTTON=0, 
                       AR_EVENT_AXIS=1, AR_EVENT_MATRIX=2};

class arInputEvent {
  public:
    arInputEvent();
    arInputEvent( const arInputEventType type, const unsigned int index );
    virtual ~arInputEvent();
    operator bool();
    
    arInputEventType getType() const;
    unsigned int getIndex() const;
    int getButton() const;
    float getAxis() const;
    arMatrix4 getMatrix() const;
    
    void setIndex( const unsigned int i );
    bool setButton( const unsigned int b );
    bool setAxis( const float a );
    // SWIG cant handle having both of these setMatrix() signatures.
    // bool setMatrix( const float* v );
    bool setMatrix( const arMatrix4& m );
    void trash();
    void zero();

%extend{
string __str__(void) {
  ostringstream s(ostringstream::out);
  switch (self->getType()) {
    case AR_EVENT_BUTTON:
      s << "BUTTON[" << self->getIndex() << "]: " << self->getButton();
      break;
    case AR_EVENT_AXIS:
      s << "AXIS[" << self->getIndex() << "]: " << self->getAxis();
        break;
    case AR_EVENT_MATRIX:
      s << "MATRIX[" << self->getIndex() << "]:\n" << self->getMatrix();
      break;
    case AR_EVENT_GARBAGE:
      s << "GARBAGE[" << self->getIndex() << "]";
      break;
    default:
      s << "EVENT_ERROR[" << self->getIndex() << "]";
  }
  return s.str();
}

PyObject* toDict() {
  return ar_inputEventToDict(*self);
}

}

};

class arInputState {
  public:
    arInputState();
    arInputState( const arInputState& x );
    arInputState& operator=( const arInputState& x );
    ~arInputState();

    // the "get" functions cannot be const since they involve 
    // a mutex lock/unlock 
    int getButton(       const unsigned int buttonNumber );
    float getAxis(       const unsigned int axisNumber );
    arMatrix4 getMatrix( const unsigned int matrixNumber );
    
    bool getOnButton(  const unsigned int buttonNumber );
    bool getOffButton( const unsigned int buttonNumber );
  
    /// \todo some classes use getNumberButtons, others getNumButtons (etc).  Be consistent.
    // Note that for the arInputState the number of buttons and the button
    // signature are the same.
    unsigned int getNumberButtons()  const { return _buttons.size(); }
    unsigned int getNumberAxes()     const { return _axes.size(); }
    unsigned int getNumberMatrices() const { return _matrices.size(); }
    
    bool setButton( const unsigned int buttonNumber, const int value );
    bool setAxis(   const unsigned int axisNumber, const float value );
    bool setMatrix( const unsigned int matrixNumber, const arMatrix4& value );
  
    bool update( const arInputEvent& event );
};

class arInputEventQueue {
  public:
    arInputEventQueue() :
      _numButtons(0),
      _numAxes(0),
      _numMatrices(0),
      _buttonSignature(0),
      _axisSignature(0),
      _matrixSignature(0) {
      }
    ~arInputEventQueue();
    void appendEvent( const arInputEvent& event );
    void appendQueue( const arInputEventQueue& queue );
    bool empty() const { return _queue.empty(); }
    bool size() const { return _queue.size(); }
    arInputEvent popNextEvent();
    
    unsigned int getNumberButtons() const { return _numButtons; }
    unsigned int getNumberAxes() const { return _numAxes; }
    unsigned int getNumberMatrices() const { return _numMatrices; }

    unsigned int getButtonSignature() const { return _buttonSignature; }
    unsigned int getAxisSignature() const { return _axisSignature; }
    unsigned int getMatrixSignature() const { return _matrixSignature; }
                        
    void clear();

%extend{
PyObject* toList() {
  PyObject *lst=PyList_New(0);
  if (!lst) {
    PyErr_SetString(PyExc_ValueError, "arInputEventQueue.toList() error: PyList_New() failed");
    return NULL;
  }
  while (true) {
    arInputEvent event = self->popNextEvent();
    if (event.getType() == AR_EVENT_GARBAGE) {
      break;
    } 
    PyObject* dict = ar_inputEventToDict( event );
    if (!dict) {
      // already set error string in ar_inputEventToDict(), should not
      // set it again here.
      Py_DECREF(lst);
      return NULL;
    }
    PyList_Append( lst, dict );
    Py_DECREF(dict);
  }
  return lst;
}

}

};

class arInputSource{
public:
  arInputSource();
  virtual ~arInputSource();
  
  void setInputNode(arInputSink*);
  
  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
  
  void sendButton(int index, int value);
  void sendAxis(int index, float value);
  void sendMatrix(int index, const arMatrix4& value);
  void queueButton(int index, int value);
  void queueAxis(int index, float value);
  void queueMatrix(int index, const arMatrix4& value);
  void sendQueue();
};

class arGenericDriver: public arInputSource{
public:
  arGenericDriver();
  ~arGenericDriver();
  
  void setSignature(int,int,int);
};

class arInputSink{
public:
  arInputSink();
  virtual ~arInputSink();
  
  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
};

class arNetInputSource: public arInputSource{
public:
  arNetInputSource();
  ~arNetInputSource();
  
  void setSlot(int slot);
  
  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
};

class arInputNode: public arInputSink {
  // Needs assignment operator and copy constructor, for pointer members.
public:
  arInputNode( bool bufferEvents = false );
  // if anyone ever derives from this class, make the following virtual:
  // destructor init start stop restart receiveData sourceReconfig.
  ~arInputNode();
  
  bool init(arSZGClient&);
  bool start();
  bool stop();
  bool restart();
  
  void addInputSource( arInputSource* theSource, bool iOwnIt );
  
  int getButton(int);
  float getAxis(int);
  arMatrix4 getMatrix(int);
};

