//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// **************** based on arSZGAppFramework.h *******************

%{
#include "arSZGAppFramework.h"
%}

enum arHeadWandSimState{
  AR_SIM_HEAD_TRANSLATE = 0,
  AR_SIM_HEAD_ROTATE,
  AR_SIM_WAND_TRANSLATE,
  AR_SIM_WAND_TRANS_BUTTONS,
  AR_SIM_WAND_ROTATE_BUTTONS,
  AR_SIM_USE_JOYSTICK,
  AR_SIM_SIMULATOR_ROTATE
};

class arInputSimulator: public arFrameworkObject{
 public:
  arInputSimulator();
  virtual ~arInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );
  void registerInputNode(arInputNode* node);

  virtual void draw() const;
  virtual void drawWithComposition();
  virtual void advance();
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( std::vector<unsigned int>& mouseButtons );
  std::vector<unsigned int> getMouseButtons();
  void setNumberButtonEvents( unsigned int numButtonEvents ); 
  unsigned int getNumberButtonEvents() const;
  
 protected:
  arGenericDriver _driver;
};

%{
class arPythonInputSimulator: public arInputSimulator{
 public:
  arPythonInputSimulator(): 
    _drawCallback(NULL),
    _advanceCallback(NULL),
    _keyboardCallback(NULL),
    _positionCallback(NULL),
    _buttonCallback(NULL){
  }
  virtual ~arPythonInputSimulator();
  
  virtual void draw() const;
  virtual void advance();
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);
  
  void setDrawCallback(PyObject* drawCallback);
  void setAdvanceCallback(PyObject* advanceCallback);
  void setKeyboardCallback(PyObject* keyboardCallback);
  void setButtonCallback(PyObject* buttonCallback);
  void setPositionCallback(PyObject* positionCallback);
  
  arInputSource* getDriver(){ return &_driver; }
  
 private:
  PyObject* _drawCallback;
  PyObject* _advanceCallback;
  PyObject* _keyboardCallback;
  PyObject* _buttonCallback;
  PyObject* _positionCallback;
};

arPythonInputSimulator::~arPythonInputSimulator(){
  if (_drawCallback){
    Py_XDECREF(_drawCallback);
  }
  if (_advanceCallback){
    Py_XDECREF(_advanceCallback); 
  }
  if (_keyboardCallback){
    Py_XDECREF(_keyboardCallback); 
  }
  if (_buttonCallback){
    Py_XDECREF(_buttonCallback); 
  }
  if (_positionCallback){
    Py_XDECREF(_positionCallback); 
  }
}
  
void arPythonInputSimulator::draw() const{
  if (!_drawCallback){
    arInputSimulator::draw();
    return;
  }
  PyObject* args = Py_BuildValue("()");
  PyObject* result = PyEval_CallObject(_drawCallback, args);
  // No return value is expected.
  Py_XDECREF(result);
  Py_DECREF(args);
}

void arPythonInputSimulator::advance(){
  if (!_advanceCallback){
    arInputSimulator::advance();
    return;
  }
  PyObject* args = Py_BuildValue("()");
  PyObject* result = PyEval_CallObject(_advanceCallback, args);
  // No return value is expected.
  Py_XDECREF(result);
  Py_DECREF(args);
}

void arPythonInputSimulator::keyboard(unsigned char key, int state, int x, int y){
  if (!_keyboardCallback){
    arInputSimulator::keyboard(key, state, x, y);
    return;
  }
  string temp;
  temp += key;
  PyObject* args = Py_BuildValue("(s,i,i,i)",temp.c_str(), state, x, y);
  PyObject* result = PyEval_CallObject(_keyboardCallback, args);
  // No return value is expected.
  Py_XDECREF(result);
  Py_DECREF(args);
}

void arPythonInputSimulator::mouseButton(int button, int state, int x, int y){
  if (!_buttonCallback){
    arInputSimulator::mouseButton(button, state, x, y);
    return;
  }
  PyObject* args = Py_BuildValue("(i,i,i,i)",button, state, x, y);
  PyObject* result = PyEval_CallObject(_buttonCallback, args);
  // No return value is expected.
  Py_XDECREF(result);
  Py_DECREF(args);
}

void arPythonInputSimulator::mousePosition(int x, int y){
  if (!_positionCallback){
    arInputSimulator::mousePosition(x, y);
    return;
  }
  PyObject* args = Py_BuildValue("(i,i)", x, y);
  PyObject* result = PyEval_CallObject(_positionCallback, args);
  // No return value is expected.
  Py_XDECREF(result);
  Py_DECREF(args);
}

void arPythonInputSimulator::setDrawCallback( PyObject* drawCallback ) {
  if (!PyCallable_Check( drawCallback )) {
    PyErr_SetString(PyExc_TypeError, "arPythonInputSimulator error: drawCallback not callable");
    return;
  }
  Py_XDECREF(_drawCallback);
  Py_XINCREF(drawCallback);
  _drawCallback = drawCallback;
}

void arPythonInputSimulator::setAdvanceCallback( PyObject* advanceCallback ) {
  if (!PyCallable_Check( advanceCallback )) {
    PyErr_SetString(PyExc_TypeError, "arPythonInputSimulator error: advanceCallback not callable");
    return;
  }
  Py_XDECREF(_advanceCallback);
  Py_XINCREF(advanceCallback);
  _advanceCallback = advanceCallback;
}

void arPythonInputSimulator::setKeyboardCallback( PyObject* keyboardCallback ){
  if (!PyCallable_Check( keyboardCallback )){
    PyErr_SetString(PyExc_TypeError, "arPythonInputSimulator error: keyboardCallback not callable.");
    return;
  }
  Py_XDECREF(_keyboardCallback);
  Py_XINCREF(keyboardCallback);
  _keyboardCallback = keyboardCallback;
}

void arPythonInputSimulator::setButtonCallback( PyObject* buttonCallback ){
  if (!PyCallable_Check( buttonCallback )){
    PyErr_SetString(PyExc_TypeError, "arPythonInputSimulator error: buttonCallback not callable.");
    return;
  }
  Py_XDECREF(_buttonCallback);
  Py_XINCREF(buttonCallback);
  _buttonCallback = buttonCallback;
}

void arPythonInputSimulator::setPositionCallback( PyObject* positionCallback ){
  if (!PyCallable_Check( positionCallback )){
    PyErr_SetString(PyExc_TypeError, "arPythonInputSimulator error: positionCallback not callable.");
    return;
  }
  Py_XDECREF(_positionCallback);
  Py_XINCREF(positionCallback);
  _positionCallback = positionCallback;
}
%}

class arPythonInputSimulator: public arInputSimulator{
 public:
  arPythonInputSimulator();
  virtual ~arPythonInputSimulator();
  virtual void draw() const;
  void setDrawCallback(PyObject* drawCallback);
  void setAdvanceCallback(PyObject* advanceCallback);
  void setKeyboardCallback(PyObject* keyboardCallback);
  void setButtonCallback(PyObject* buttonCallback);
  void setPositionCallback(PyObject* positionCallback);
  
  arInputSource* getDriver();
};

%pythoncode{

# Subclass this Python class to make new input simulator modules in Python.
class arPyInputSimulator(arPythonInputSimulator):
  def __init__(self):
    arPythonInputSimulator.__init__(self)
    self.setDrawCallback( self.onDraw )
    self.setAdvanceCallback( self.onAdvance )
    self.setKeyboardCallback( self.onKeyboard )
    self.setButtonCallback( self.onButton )
    self.setPositionCallback( self.onPosition )
  def onDraw( self):
    pass
  def onAdvance( self ):
    pass
  def onKeyboard( self, key, state, x, y ):
    pass
  def onButton( self, button, state, x, y ):
    pass
  def onPosition( self, x, y ):
    pass
    
}

class arSZGAppFramework {
  public:
    arSZGAppFramework();
    virtual ~arSZGAppFramework();
    
    virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    virtual bool createWindows(bool useWindowing) = 0;
    virtual void loopQuantum() = 0;
    virtual void exitFunction() = 0;
    
    virtual void setDataBundlePath(const string&, const string&);
    virtual void loadNavMatrix();
    void speak( const std::string& message );
    bool setInputSimulator( arInputSimulator* sim );
    string getLabel();
    bool getStandalone() const;
    const string getDataPath();
    
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    arHead* getHead();
    virtual void setFixedHeadMode(bool isOn);
    virtual arMatrix4 getMidEyeMatrix();
    virtual arVector3 getMidEyePosition();
    virtual void setUnitConversion( float conv );
    virtual void setUnitSoundConversion( float conv );
    virtual float getUnitConversion();
    virtual float getUnitSoundConversion();
    
    int getButton( const unsigned int index ) const;
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index, bool doUnitConversion=true ) const;
    bool getOnButton( const unsigned int index ) const;
    bool getOffButton( const unsigned int index ) const;
    unsigned int getNumberButtons()  const;
    unsigned int getNumberAxes()     const;
    unsigned int getNumberMatrices() const;
    
    bool setNavTransCondition( char axis,
                               arInputEventType type,
                               unsigned int index,
                               float threshold );
    bool setNavRotCondition( char axis,
                             arInputEventType type,
                             unsigned int index,
                             float threshold );
    void setNavTransSpeed( float speed );
    void setNavRotSpeed( float speed );
    void setNavEffector( const arEffector& effector );
    void ownNavParam( const std::string& paramName );
    void navUpdate();
    void navUpdate( arInputEvent& event );
    
    bool setEventFilter( arFrameworkEventFilter* filter );
    void setEventCallback( arFrameworkEventCallback callback );
    virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );
    void processEventQueue();
    virtual void onProcessEventQueue( arInputEventQueue& theQueue );
    arInputState* getInputState();
    
    bool stopping();
    bool stopped();
    void useExternalThread();
    void externalThreadStarted();
    void externalThreadStopped();
    
    arAppLauncher* getAppLauncher();
    arInputNode* getInputNode();
    arInputNode* getInputDevice();
    arGUIWindowManager* getWindowManager( void );
    arSZGClient* getSZGClient();
};



