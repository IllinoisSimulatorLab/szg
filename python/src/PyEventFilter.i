// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

%{

// Some actual code that will get compiled, as opposed to being analyzed
// by SWIG for bindings.

#include "arFrameworkEventFilter.h"

class arPythonEventFilter: public arFrameworkEventFilter {
  public:
    arPythonEventFilter( arSZGAppFramework* fw = NULL );
    ~arPythonEventFilter();
    void setCallback( PyObject* eventCallback );
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    PyObject* _pycallback;
};

arPythonEventFilter::arPythonEventFilter( arSZGAppFramework* fw ) :
  arFrameworkEventFilter(fw),
  _pycallback(NULL) {
}

arPythonEventFilter::~arPythonEventFilter() {
  if (_pycallback != NULL) {
    Py_XDECREF(_pycallback);
  } 
}

bool arPythonEventFilter::_processEvent( arInputEvent& inputEvent ) {
  if (_pycallback == NULL) { // not an error, just no event-handling
    return true;
  }
  PyObject *eventobj = SWIG_NewPointerObj((void *) &inputEvent,
                           SWIGTYPE_p_arInputEvent, 0); 
  PyObject *arglist=Py_BuildValue("(O)",eventobj); 
  PyObject *result=PyEval_CallObject(_pycallback, arglist);  
  if (result==NULL) { 
      PyErr_Print(); 
      string errmsg="A Python exception occurred in the event callback.";
      cerr << errmsg << "\n";
      throw  errmsg; 
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(eventobj); 
  return res; 
}

void arPythonEventFilter::setCallback( PyObject* eventCallback ) {
  if (!PyCallable_Check(eventCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonEventFilter error: eventCallback not callable");
    return;
  }

  Py_XDECREF(_pycallback);
  Py_XINCREF(eventCallback);
  _pycallback = eventCallback;
}


%}

class arPythonEventFilter : public arFrameworkEventFilter {
  public:
    arPythonEventFilter( arSZGAppFramework* fw = NULL );
    virtual ~arPythonEventFilter() {}

    // methods inherited from arIOFilter
    int getButton( const unsigned int index ) const;
    bool getOnButton( const unsigned int index );
    bool getOffButton( const unsigned int index );
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const;
    arInputState* getInputState() const { return _inputState; }
    void insertNewEvent( const arInputEvent& newEvent );

    // methods inherited from arFrameworkEventFilter
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }

    // method specific to this class
    void setCallback( PyObject* eventCallback );
};

%pythoncode %{

class arPyEventFilter(arPythonEventFilter):
  def __init__(self,framework=None):
    arPythonEventFilter.__init__(self,framework)
    self.setCallback( self.onEvent )
  def onEvent( self, event ):
    return True

%}


class arCallbackEventFilter : public arFrameworkEventFilter {
  public:
    arSZGAppFramework* getFramework() const { return _framework; }
    int getButton( const unsigned int index ) const;
    bool getOnButton( const unsigned int index );
    bool getOffButton( const unsigned int index );
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const;
    arInputState* getInputState() const { return _inputState; }
    void insertNewEvent( const arInputEvent& newEvent );
};
