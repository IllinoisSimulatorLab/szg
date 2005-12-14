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
  void advance();

  // used to capture and process mouse/keyboard data
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( std::vector<unsigned int>& mouseButtons );
  std::vector<unsigned int> getMouseButtons();
  void setNumberButtonEvents( unsigned int numButtonEvents ); 
  unsigned int getNumberButtonEvents() const;
};

%{
class arPythonInputSimulator: public arInputSimulator{
 public:
  arPythonInputSimulator(): 
    _drawCallback(NULL){
  }
  virtual ~arPythonInputSimulator();
  
  virtual void draw() const;
  
  void setDrawCallback(PyObject* drawCallback);
  
 private:
  PyObject* _drawCallback;
};

arPythonInputSimulator::~arPythonInputSimulator(){
  if (_drawCallback){
    Py_XDECREF(_drawCallback);
  }
}
  
void arPythonInputSimulator::draw() const{
  if (!_drawCallback){
    throw "arPythonInputSimulator error: no draw callback.\n";
  }
  PyObject* args = Py_BuildValue("()");
  PyObject* result = PyEval_CallObject(_drawCallback, args);
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
%}

class arPythonInputSimulator: public arInputSimulator{
 public:
  arPythonInputSimulator();
  virtual ~arPythonInputSimulator();
  virtual void draw() const;
  void setDrawCallback(PyObject* drawCallback);
};

%pythoncode{

# Subclass this Python class to make new input simulator modules in Python.
class arPyInputSimulator(arPythonInputSimulator):
  def __init__(self):
    arPythonInputSimulator.__init__(self)
    self.setDrawCallback( self.onDraw )
  def onDraw( self):
    pass
    
}

class arSZGAppFramework {
  public:
    arSZGAppFramework();
    virtual ~arSZGAppFramework();
    
    bool setInputSimulator( arInputSimulator* sim );
    
    virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    virtual bool createWindows(bool useWindowing);
    virtual void loopQuantum();
    virtual void exitFunction();
    
    string  getLabel(){ return _label; }
    bool getStandalone() const;
    arSZGClient* getSZGClient();

    virtual void loadNavMatrix() = 0;
    
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    virtual void setFixedHeadMode(bool isOn);
    virtual arMatrix4 getMidEyeMatrix();
    virtual arVector3 getMidEyePosition();
    virtual void setUnitConversion( float conv );
    virtual void setUnitSoundConversion( float conv );
    virtual float getUnitConversion();
    virtual float getUnitSoundConversion();
    const string getDataPath();
    int getButton(       const unsigned int index ) const;
    float getAxis(       const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const;
    bool getOnButton(    const unsigned int index ) const;
    bool getOffButton(   const unsigned int index ) const;
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

    void speak( const string& message );

    arInputState* getInputState();
      
    bool stopping();
    bool stopped();
    void useExternalThread();
    void externalThreadStarted();
    void externalThreadStopped();
    void processEventQueue();
    arAppLauncher* getAppLauncher();
    arInputNode* getInputNode();
};



