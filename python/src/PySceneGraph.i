// $Id: PySceneGraph.i,v 1.4 2005/12/01 23:24:39 schaeffr Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// ******************** based on arInterfaceObject.h ********************

class arInterfaceObject{
 public:
  arInterfaceObject();
  ~arInterfaceObject();
  
  void setInputDevice(arInputNode*);
  bool start();
  void setNavMatrix(const arMatrix4&);
  arMatrix4 getNavMatrix();
  void setObjectMatrix(const arMatrix4&);
  arMatrix4 getObjectMatrix();

  void setSpeedMultiplier(float);
  
  void setNumMatrices( const int num );
  void setNumButtons( const int num );
  void setNumAxes( const int num );
  
  int getNumMatrices() const;
  int getNumButtons() const;
  int getNumAxes() const;
  
  bool setMatrix( const int num, const arMatrix4& mat );
  bool setButton( const int num, const int but );
  bool setAxis( const int num, const float val );
  void setMatrices( const arMatrix4* matPtr );
  void setButtons( const int* butPtr );
  void setAxes( const float* axisPtr );
  
  arMatrix4 getMatrix( const int num ) const;
  int getButton( const int num ) const;
  float getAxis( const int num ) const;
};

// **************** based on arDistSceneGraphFramework.h *******************

%{

//    void setEventCallback( bool (*callback)( arSZGAppFramework& fw, arInputEvent& event,
//                                             arCallbackEventFilter& filter) );
//
static PyObject *pySGEventFunc = NULL;
static bool pySGEventCallback( arSZGAppFramework& fw, arInputEvent& theEvent, arCallbackEventFilter& filter ) {
    // Note cast from arSZGAppFramework to arDistSceneGraphFramework
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                             SWIGTYPE_p_arDistSceneGraphFramework, 0);
    PyObject *eventobj = SWIG_NewPointerObj((void *) &theEvent,
                             SWIGTYPE_p_arInputEvent, 0);
    PyObject *filterobj = SWIG_NewPointerObj((void *) &filter,
                             SWIGTYPE_p_arCallbackEventFilter, 0);
    PyObject *arglist=Py_BuildValue( "(O,O,O)", fwobj, eventobj, filterobj );
    PyObject *result=PyEval_CallObject(pySGEventFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in Event callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    bool res=(bool) PyInt_AsLong(result);
    Py_XDECREF(result);
    Py_DECREF(arglist);
    Py_DECREF(filterobj);
    Py_DECREF(eventobj);
    Py_DECREF(fwobj);
    return res;
}


//    void setEventQueueCallback( bool (*callback)( arSZGAppFramework& fw,
//                                                  arInputEventQueue& theQueue ) );
//
static PyObject *pySGEventQueueFunc = NULL;
static bool pySGEventQueueCallback( arSZGAppFramework& fw, arInputEventQueue& theQueue ) {
  // Note cast from arSZGAppFramework to arDistSceneGraphFramework
  PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                           SWIGTYPE_p_arDistSceneGraphFramework, 0);
  PyObject *queueobj = SWIG_NewPointerObj((void *) &theQueue,
                           SWIGTYPE_p_arInputEventQueue, 0);
  PyObject *arglist=Py_BuildValue( "(O,O)", fwobj, queueobj );
  PyObject *result=PyEval_CallObject(pySGEventQueueFunc, arglist);
  if (result==NULL) {
    PyErr_Print();
    string errmsg="A Python exception occurred in EventQueue callback.";
    cerr << errmsg << "\n";
    throw  errmsg;
  }
  bool res=(bool) PyInt_AsLong(result);
  Py_XDECREF(result);
  Py_DECREF(arglist);
  Py_DECREF(queueobj);
  Py_DECREF(fwobj);
  return res;
}

//  void setUserMessageCallback(void (*userMessageCallback)(arDistributedSceneGraphFramework&, const string&));
//
static PyObject *pySGUserMessageFunc = NULL;
static void pySGUserMessageCallback(arDistSceneGraphFramework& fw,const string & s) {
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                             SWIGTYPE_p_arDistSceneGraphFramework, 0);
    PyObject *arglist=Py_BuildValue("(O,s)",fwobj,s.c_str());
    PyObject *result=PyEval_CallObject(pySGUserMessageFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in UserMessage callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    Py_XDECREF(result);
    Py_DECREF(arglist);
    Py_DECREF(fwobj);
}

%}


class arDistSceneGraphFramework : public arSZGAppFramework {
 public:
  arDistSceneGraphFramework();
  ~arDistSceneGraphFramework() {}

  arGraphicsDatabase* getDatabase();
                                                                               
  // inherited pure virtual functions
  bool init(int&,char**);
  bool start();
  void stop(bool);
  void loadNavMatrix();
                                                                               
  void setDataBundlePath(const string& bundlePathName, 
                         const string& bundleSubDirectory);

  void setAutoBufferSwap(bool);
  void swapBuffers();
                                                                               
  void setViewer();
  void setPlayer();
                                                                               
  bool restart();
                                                                               
  void setHeadMatrixID(int);
  const string getNavNodeName() const;
  arDatabaseNode* getNavNode();
                                                                               
  arInputNode* getInputDevice() const;
  
  %extend{
  void setEventCallback(PyObject* PyFunc) {
    Py_XDECREF(pySGEventFunc); 
    Py_XINCREF(PyFunc); 
    pySGEventFunc = PyFunc; 
    self->setEventCallback(pySGEventCallback);
  }
  
  void setEventQueueCallback(PyObject* PyFunc) {
    Py_XDECREF(pySGEventQueueFunc); 
    Py_XINCREF(PyFunc); 
    pySGEventQueueFunc = PyFunc; 
    self->setEventQueueCallback(pySGEventQueueCallback);
  }
  
  void setUserMessageCallback(PyObject* PyFunc) {
    Py_XDECREF(pySGUserMessageFunc); 
    Py_XINCREF(PyFunc); 
    pySGUserMessageFunc = PyFunc; 
    self->setUserMessageCallback(pySGUserMessageCallback);
  }

  }
};
