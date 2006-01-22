// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

%{
#include "arEffector.h"
#include "arInteractable.h"
#include "arGrabCondition.h"
#include "arDragBehavior.h"
#include "arInteractionSelector.h"

class arPythonInteractable;
class arPythonDragBehavior;
%}

// **************** based on szg/src/interaction/arEffector.h *******************

// A thing for grabbing, poking, etc., other things with.

// class definition that SWIG will use to create binding functions

class arEffector {
  public:
    arEffector();
    // NOTE: the "lo" parameters, e.g. "loButton", tell the effector which indices
    // to grab from the input. If numButtons = 3 and loButton = 2, then input button
    // events with indices 2-4 will be captured here. By default they get mapped to
    // a starting index of 0, e.g. in the case just described asking the effector
    // for button 0 will get you what was input button event 2.
    arEffector( const unsigned int matrixIndex,
                const unsigned int numButtons,
                const unsigned int loButton,
                const unsigned int numAxes,
                const unsigned int loAxis );
    virtual ~arEffector();
    void setInteractionSelector( const arInteractionSelector& selector );
    float calcDistance( const arMatrix4& mat );
    void setUnitConversion( float conv );
    void setTipOffset( const arVector3& offset );
    void updateState( arInputState* state );
    void updateState( arInputEvent& event );
    int getButton( unsigned int index );
    float getAxis( unsigned int index );
    arMatrix4 getMatrix() const;
    arMatrix4 getBaseMatrix() const;
    arMatrix4 getOffsetMatrix() const;
    arMatrix4 getInputMatrix() const;
    arMatrix4 getCenterMatrix() const;
    bool getOnButton( unsigned int index );
    bool getOffButton( unsigned int index );
    virtual void setMatrix( const arMatrix4& matrix );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setTouchedObject( arInteractable* touched );
    bool requestGrab( arPythonInteractable* grabee );
    void requestUngrab( arPythonInteractable* grabee );
    void forceUngrab();

%extend {
    // These two functions are here so that we can get references to
    // actual Python objects, as opposed to SWIG pointers (strings),
    // which is what the non-typecast versions of these methods
    // produce. (arPythonInteractable is defined below).
    arPythonInteractable* getGrabbedObject() {
      return (arPythonInteractable*)self->getGrabbedObject();
    }
    arPythonInteractable* getTouchedObject() {
      return (arPythonInteractable*)self->getTouchedObject();
    }

    bool hasGrabbedObject() { return self->getGrabbedObject() != NULL; }
    bool hasTouchedObject() { return self->getTouchedObject() != NULL; }
}
};

// **************** based on interaction/arInteractionSelector.h *******************

// Algorithms for determining which object, if any, a given arEffector
// should interact with.

// Select an object based on distance from effector (within upper bound).

// NOTE: SWIG prints a warning about the 'public arInteractionSelector'
// bit, BUT IT IS NECESSARY (for the various subclasses to be recognized
// by SWIG as belonging to the same class hierarchy)!!!!

class arDistanceInteractionSelector : public arInteractionSelector {
  public:
    arDistanceInteractionSelector( float maxDistance = -1. );
    virtual ~arDistanceInteractionSelector();
    void setMaxDistance( float maxDistance );
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
};

// Never let go of the object youve got (unless it asks and then disables itself).
class arAlwaysInteractionSelector : public arInteractionSelector {
  public:
    arAlwaysInteractionSelector();
    virtual ~arAlwaysInteractionSelector();
    virtual float calcDistance( const arEffector& /*effector*/,
                                const arMatrix4& /*objectMatrix*/ ) const;
};

// Select object based on angular separation with effector direction.
class arAngleInteractionSelector: public arInteractionSelector {
  public:
    arAngleInteractionSelector( float maxAngle = ar_convertToRad(10.) );
    virtual ~arAngleInteractionSelector();
    void setMaxAngle( float maxAngle );
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
};

// We define a new arInteractionSelector subclass here and a set of corresponding
// Python subclasses below to make the
// interface cleaner. arPythonInteractionSelector contains pointers to callbacks that are
// actually Python function objects.

%{

// Some actual code that will get compiled, as opposed to being analyzed
// by SWIG for bindings.

class arPythonInteractionSelector : public arInteractionSelector {
  public:
    arPythonInteractionSelector() :
      arInteractionSelector(),
      _distanceCallback(NULL) {
    }
    arPythonInteractionSelector( PyObject* distanceCallback ) :
      arInteractionSelector(),
      _distanceCallback(NULL) {
      setCallback( distanceCallback );
    }
    virtual ~arPythonInteractionSelector();
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
    virtual arInteractionSelector* copy() const;
    void setCallback( PyObject* distanceCallback );
  private:
    PyObject* _distanceCallback;
};

arPythonInteractionSelector::~arPythonInteractionSelector() {
  if (_distanceCallback != NULL) {
    Py_XDECREF(_distanceCallback);
  } 
}

float arPythonInteractionSelector::calcDistance( const arEffector& effector,
                                                 const arMatrix4& objectMatrix ) const {
  if (_distanceCallback == NULL) {
    throw "arPythonInteractionSelector error: no distance callback.";
  }
  PyObject *effobj = SWIG_NewPointerObj( (void *) &effector,
                                         SWIGTYPE_p_arEffector, 0); 
  PyObject *matObj = SWIG_NewPointerObj( (void *) &objectMatrix,
                                         SWIGTYPE_p_arMatrix4, 0); 
  PyObject *arglist = Py_BuildValue( "(O,O)", effobj, matObj ); 
  PyObject *result = PyEval_CallObject( _distanceCallback, arglist );  
  if (result==NULL) { 
      PyErr_Print(); 
      string errmsg="A Python exception occurred in the distance callback.";
      cerr << errmsg << "\n";
      throw  errmsg; 
  }
  if (!PyFloat_Check( result )) {
    throw "arPythonInteractionSelector error: distance callback returned non-Float.";
  }
  float returnVal = (float)PyFloat_AsDouble( result );
  Py_XDECREF( result ); 
  Py_DECREF( arglist ); 
  Py_DECREF( matObj ); 
  Py_DECREF( effobj ); 
  return returnVal;
}

arInteractionSelector* arPythonInteractionSelector::copy() const {
  return (arInteractionSelector*)new arPythonInteractionSelector( _distanceCallback );
}

void arPythonInteractionSelector::setCallback( PyObject* distanceCallback ) {
  if (!PyCallable_Check( distanceCallback )) {
    PyErr_SetString(PyExc_TypeError, "arPythonInteractionSelector error: distanceCallback not callable");
    return;
  }
  Py_XDECREF(_distanceCallback);
  Py_XINCREF(distanceCallback);
  _distanceCallback = distanceCallback;
}

%}

// This is the arPythonInteractionSelector description that SWIG uses to 
// construct the bindings that a Python program will see.

class arPythonInteractionSelector : public arInteractionSelector {
  public:
    arPythonInteractionSelector();
    virtual ~arPythonInteractionSelector();
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix );
    void setCallback( PyObject* distanceCallback );
};

// And THIS is a python class that wraps the arPythonInteractionSelector
// class. It gets copied automagically to PySZG.py.
// Sub-class it and override the onInit and onUpdate methods
// (and call arPyDragBehavior.__init__ in your classes __init__).

%pythoncode %{

class arPyInteractionSelector(arPythonInteractionSelector):
  def __init__(self):
    arPythonInteractionSelector.__init__(self)
    self.setCallback( self.onCalcDistance )
  def onCalcDistance( self, effector, objectMatrix ):
    return 1.

# A Python arPyInteractionSelector subclass that mimics the
# functionality of arDistanceInteractionSelector, provided
# as a demo.
class arPyDistanceInteractionSelector(arPyInteractionSelector):
  def __init__( self, maxDistance ):
    arPyInteractionSelector.__init__(self)
    self._maxDistance = maxDistance
  def onCalcDistance( self, effector, objectMatrix ):
    distance = (ar_extractTranslation( effector.getMatrix() ) -
                    ar_extractTranslation( objectMatrix )).magnitude()
    if (self._maxDistance > 0.) and (distance > self._maxDistance):
      return -1.
    return distance

%}


// **************** based on interaction/arGrabCondition.h *******************

// A condition on an arEffectors state that must be met for a grab
// to occur, e.g. (AR_EVENT_BUTTON, 1, 0.5) means that the value
// of button event #1 must be greater than 0.5 for an object to
// be grabbed.

class arGrabCondition {
  public:
    arGrabCondition( arInputEventType eventType,
                     unsigned int eventIndex,
                     float thresholdValue );
    virtual ~arGrabCondition();
};


// **************** based on arDragBehavior.h *******************

// How an object moves in response to arEffector movement
// when it is grabbed.

// We define a new arDragBehavior subclass here and a set of corresponding
// Python subclasses below to make the
// interface cleaner. arPythonDragBehavior contains pointers to callbacks that are
// actually Python function objects.

%{

// Some actual code that will get compiled, as opposed to being analyzed
// by SWIG for bindings.

class arPythonDragBehavior : public arDragBehavior {
  public:
    arPythonDragBehavior() :
      arDragBehavior(),
      _initCallback(NULL),
      _updateCallback(NULL) {
    }
    arPythonDragBehavior( PyObject* initCallback, PyObject* updateCallback ) :
      arDragBehavior(),
      _initCallback(NULL),
      _updateCallback(NULL) {
      setCallbacks( initCallback, updateCallback );
    }
    virtual ~arPythonDragBehavior();
    virtual void init( const arEffector* const effector,
                       const arInteractable* const object );
    virtual void update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const grabCondition );
    virtual arDragBehavior* copy() const;
    void setCallbacks( PyObject* initCallback,
                       PyObject* updateCallback );
  private:
    PyObject* _initCallback;
    PyObject* _updateCallback;
};

arPythonDragBehavior::~arPythonDragBehavior() {
  if (_initCallback != NULL) {
    Py_XDECREF(_initCallback);
  } 
  if (_updateCallback != NULL) {
    Py_XDECREF(_updateCallback);
  } 
}

void arPythonDragBehavior::init( const arEffector* const effector,
                                   const arInteractable* const object ) {
  if (_initCallback == NULL) {
    cerr << "arPythonDragBehavior warning: no init callback.\n";
    return;
  }
  PyObject *effobj = SWIG_NewPointerObj((void *) effector,
                           SWIGTYPE_p_arEffector, 0); 
  // NOTE TYPECAST FROM arInteractable to arPythonInteractable!
  // (I think this should be OK, as interactables that get this
  // far should only come from Python)
  PyObject *inter = SWIG_NewPointerObj((void *) object,
                           SWIGTYPE_p_arPythonInteractable, 0); 
  PyObject *arglist=Py_BuildValue("(O,O)",effobj,inter); 
  PyObject *result=PyEval_CallObject(_initCallback, arglist);  
  if (result==NULL) { 
      PyErr_Print(); 
      string errmsg="A Python exception occurred in the DragInit callback.";
      cerr << errmsg << "\n";
      throw  errmsg; 
  }
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(inter); 
  Py_DECREF(effobj); 
}

void arPythonDragBehavior::update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const grabCondition ) {
  if (_updateCallback == NULL) {
    cerr << "arPythonDragBehavior warning: no update callback.\n";
    return;
  }
  PyObject *effobj = SWIG_NewPointerObj((void *) effector,
                           SWIGTYPE_p_arEffector, 0); 
  // NOTE TYPECAST FROM arInteractable to arPythonInteractable!
  // (I think this should be OK, as interactables that get this
  // far should only come from Python)
  PyObject *inter = SWIG_NewPointerObj((void *) object,
                           SWIGTYPE_p_arPythonInteractable, 0); 
  PyObject *grab = SWIG_NewPointerObj((void *) grabCondition,
                           SWIGTYPE_p_arGrabCondition, 0); 
  PyObject *arglist=Py_BuildValue("(O,O,O)",effobj,inter,grab); 
  PyObject *result=PyEval_CallObject(_updateCallback, arglist);  
  if (result==NULL) { 
      PyErr_Print(); 
      string errmsg="A Python exception occurred in the DragUpdate callback.";
      cerr << errmsg << "\n";
      throw  errmsg; 
  }
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(grab); 
  Py_DECREF(inter); 
  Py_DECREF(effobj); 
}

arDragBehavior* arPythonDragBehavior::copy() const {
  return (arDragBehavior*)new arPythonDragBehavior( _initCallback, _updateCallback );
}

void arPythonDragBehavior::setCallbacks( PyObject* initCallback,
                                         PyObject* updateCallback ) {
  if (!PyCallable_Check(initCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonDragBehavior error: initCallback not callable");
    return;
  }
  if (!PyCallable_Check(updateCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonDragBehavior error: updateCallback not callable");
    return;
  }

  Py_XDECREF(_initCallback);
  Py_XINCREF(initCallback);
  _initCallback = initCallback;

  Py_XDECREF(_updateCallback);
  Py_XINCREF(updateCallback);
  _updateCallback = updateCallback;
}

%}

// These are the arDragBehavior subclass descriptions that
// SWIG will use to construct the bindings that a Python program will see.

class arNullDragBehavior : public arDragBehavior {
  public:
    arNullDragBehavior();
    virtual ~arNullDragBehavior();
};

class arWandRelativeDrag : public arDragBehavior {
  public:
    arWandRelativeDrag();
    virtual ~arWandRelativeDrag();
};

class arWandTranslationDrag : public arDragBehavior {
  public:
    arWandTranslationDrag( bool allowOffset = true );
    virtual ~arWandTranslationDrag();
};

class arPythonDragBehavior : public arDragBehavior {
  public:
    arPythonDragBehavior();
    virtual ~arPythonDragBehavior();
    void setCallbacks( PyObject* initCallback,
                       PyObject* updateCallback );
};

// And THIS is a python class that wraps the arPythonDragBehavior
// class. It gets copied automagically to PySZG.py.
// Sub-class it and override the onInit and onUpdate methods
// (and call arPyDragBehavior.__init__ in your classes __init__).

%pythoncode %{

class arPyDragBehavior(arPythonDragBehavior):
  def __init__(self):
    arPythonDragBehavior.__init__(self)
    self.setCallbacks( self.onInit, self.onUpdate )
  def onInit( self, effector, object ):
    pass
  def onUpdate( self, effector, object, grabCondition ):
    pass

# Here are some arPyDragBehavior subclasses that mimic the
# functionality of the C++ arDragBehavior subclasses above
# (here for illustrative purposes).

# Do nothing. You can still grab the object (i.e. all
# events from the effector will go to it until you let go),
# but it wont move.
class arPyNullDrag(arPyDragBehavior):
  def __init__(self):
    arPyDragBehavior.__init__(self)
  def onInit( self, effector, object ):
    pass
  def onUpdate( self, effector, object, grabCondition ):
    pass
    
# The object will maintain a fixed relationship
# to the effector tip, as though rigidly attached.
class arPyWandRelativeDrag(arPyDragBehavior):
  def __init__(self):
    arPyDragBehavior.__init__(self)
    self.diffMatrix = arMatrix4()
  def onInit( self, effector, object ):
    self.diffMatrix = effector.getMatrix().inverse() * object.getMatrix()
  def onUpdate( self, effector, object, grabCondition ):
    object.setMatrix( effector.getMatrix() * self.diffMatrix )
    
# As above, object maintains a fixed _positional_
# relationship with the effector tip as it moves around,
# but the objects orientation does not change.
class arPyWandTranslationDrag(arPyDragBehavior):
  def __init__(self,allowOffset=True):
    arPyDragBehavior.__init__(self)
    self.allowOffset = allowOffset
    self.positionOffsetMatrix = arMatrix4()  
    self.objectOrientMatrix = arMatrix4()  
  def onInit( self, effector, object ):
    effMatrix = effector.getMatrix()
    objMatrix = object.getMatrix()
    if self.allowOffset:
      self.positionOffsetMatrix = ar_translationMatrix( ar_extractTranslation( objMatrix ) - ar_extractTranslation( effMatrix ) )
    else:
      self.positionOffsetMatrix = ar_identityMatrix()
    self.objectOrientMatrix = ar_extractRotationMatrix( objMatrix )
  def onUpdate( self, effector, object, grabCondition ):
    object.setMatrix( ar_extractTranslationMatrix( effector.getMatrix() ) * self.positionOffsetMatrix * self.objectOrientMatrix )

%}

// **************** based on interaction/arInteractable.h *******************

// A thing you can interact with.

// As with the arPythonDragBehavior, we define a new Python-friendly
// arInteractable subclass here and a Python wrapper subclass below.

%{

// Some actual code that will get compiled, as opposed to being analyzed
// by SWIG for bindings.

class arPythonInteractable : public arInteractable {
  public:
    arPythonInteractable();
    ~arPythonInteractable();
    virtual void setMatrix( const arMatrix4& matrix );
    void setCallbacks( PyObject* touchCallback,
                       PyObject* processCallback,
                       PyObject* untouchCallback,
                       PyObject* matrixCallback );
    void clearCallbacks();
  protected:
    virtual bool _processInteraction( arEffector& effector );
    virtual bool _touch( arEffector& effector );
    virtual bool _untouch( arEffector& effector );
    PyObject* _touchCallback;
    PyObject* _processCallback;
    PyObject* _untouchCallback;
    PyObject* _matrixCallback;
};

arPythonInteractable::arPythonInteractable() :
  arInteractable(),
  _touchCallback(NULL),
  _processCallback(NULL),
  _untouchCallback(NULL),
  _matrixCallback(NULL) {
}

arPythonInteractable::~arPythonInteractable() {
/*  cout << "arPythonInteractable remark: destructor called.\n";*/
  _cleanup();
  Py_XDECREF(_touchCallback);
  Py_XDECREF(_processCallback);
  Py_XDECREF(_untouchCallback);
  Py_XDECREF(_matrixCallback);
}

bool arPythonInteractable::_processInteraction( arEffector& effector ) {
  if ((_processCallback == NULL)||(_processCallback == Py_None)) {
    return true;
  }
  PyObject *effobj = SWIG_NewPointerObj((void *) &effector,
                           SWIGTYPE_p_arEffector, 0); 
  PyObject *arglist=Py_BuildValue("(O)",effobj); 
  PyObject *result=PyEval_CallObject(_processCallback, arglist);  
  if (result==NULL) { 
      PyErr_Print(); 
      PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the Process callback." );
      return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(effobj); 
  return res; 
}

bool arPythonInteractable::_touch( arEffector& effector ) {
  if ((_touchCallback == NULL)||(_touchCallback == Py_None)) {
    return true;
  }
  PyObject *effobj = SWIG_NewPointerObj((void *) &effector,
                           SWIGTYPE_p_arEffector, 0); 
  PyObject *arglist=Py_BuildValue("(O)",effobj); 
  PyObject *result=PyEval_CallObject(_touchCallback, arglist);  
  if (result==NULL) { 
      PyErr_Print(); 
      PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the Touch callback." );
      return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(effobj); 
  return res; 
}

bool arPythonInteractable::_untouch( arEffector& effector ) {
  if ((_untouchCallback == NULL)||(_untouchCallback == Py_None)) {
    return true;
  }
  PyObject *effobj = SWIG_NewPointerObj((void *) &effector,
                           SWIGTYPE_p_arEffector, 0); 
  PyObject *arglist=Py_BuildValue("(O)",effobj);
  PyObject *result=PyEval_CallObject(_untouchCallback, arglist);  
  if (result==NULL) { 
    PyErr_Print(); 
    PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the Untouch callback." );
    return false;
  }
  bool res = (bool) PyInt_AsLong(result);
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(effobj); 
  return res;
}

void arPythonInteractable::setMatrix( const arMatrix4& matrix ) {
  arInteractable::setMatrix( matrix );
  if ((_matrixCallback == NULL)||(_matrixCallback == Py_None)) {
    return;
  }
  PyObject *matobj = SWIG_NewPointerObj((void *) &matrix, 
                           SWIGTYPE_p_arMatrix4, 0);
  PyObject *arglist=Py_BuildValue("(O)",matobj);
  PyObject *result=PyEval_CallObject( _matrixCallback, arglist );  
  if (result==NULL) { 
    PyErr_Print(); 
    PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the setMatrix callback." );
    return;
  }
  Py_XDECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(matobj); 
}

void arPythonInteractable::setCallbacks( PyObject* touchCallback,
                                         PyObject* processCallback,
                                         PyObject* untouchCallback,
                                         PyObject* matrixCallback ) {
  if (touchCallback != Py_None) {
    if (!PyCallable_Check(touchCallback)) {
      PyErr_SetString(PyExc_TypeError, "arPythonInteractable error: touchCallback not callable");
      return;
    }
  }
  if (processCallback != Py_None) {
    if (!PyCallable_Check(processCallback)) {
      PyErr_SetString(PyExc_TypeError, "arPythonInteractable error: processCallback not callable");
      return;
    }
  }
  if (untouchCallback != Py_None) {
    if (!PyCallable_Check(untouchCallback)) {
      PyErr_SetString(PyExc_TypeError, "arPythonInteractable error: untouchCallback not callable");
      return;
    }
  }
  if (matrixCallback != Py_None) {
    if (!PyCallable_Check(matrixCallback)) {
      PyErr_SetString(PyExc_TypeError, "arPythonInteractable error: matrixCallback not callable");
      return;
    }
  }

  Py_XDECREF(_touchCallback);
  Py_XDECREF(_processCallback);
  Py_XDECREF(_untouchCallback);
  Py_XDECREF(_matrixCallback);

  Py_XINCREF(touchCallback);
  Py_XINCREF(processCallback);
  Py_XINCREF(untouchCallback);
  Py_XINCREF(matrixCallback);

  _touchCallback = touchCallback;
  _processCallback = processCallback;
  _untouchCallback = untouchCallback;
  _matrixCallback = matrixCallback;
}

void arPythonInteractable::clearCallbacks() {
  setCallbacks( Py_None, Py_None, Py_None, Py_None );
}

%}

// And here is the description for SWIG binding generation.

class arPythonInteractable : public arInteractable {
  public:
    arPythonInteractable();
    virtual ~arPythonInteractable();

    virtual bool touch( arEffector& effector );
    virtual bool processInteraction( arEffector& effector );
    virtual bool untouch( arEffector& effector );
    virtual bool untouchAll();
    
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );
    virtual void setMatrix( const arMatrix4& matrix );

    void setCallbacks( PyObject* touchCallback,
                       PyObject* processCallback,
                       PyObject* untouchCallback,
                       PyObject* matrixCallback );

    void clearCallbacks();
};


%{
void ar_activateColor( const arVector4& colvec ) {
  glColor4f( colvec.v[0], colvec.v[1], colvec.v[2], colvec.v[3] );
}

/*int ar_compareSwigPtrs( PySwigObject* a, PySwigObject* b ) {*/
/*  return PySwigObject_compare(a,b) == 0;*/
/*}*/

%}

void ar_activateColor( const arVector4& color );
//int ar_compareSwigPtrs( PySwigObject* a, PySwigObject* b );

// And here is the Python wrapper class.

%pythoncode %{

# Base class for objects you want to interact with
# (basically a framework/arInteractableThing)

class arPyInteractable(arPythonInteractable):
  def __init__(self):
    arPythonInteractable.__init__(self)
    self._visible = True
    self._highlighted = False
    self._color = arVector4(1,1,1,1)
    self._texture = None
    # NOTE!! The last argument, incRefCounts, defaults to True and should _only_
    # be set to False this method is used for this particular purpose, i.e. installing
    # the classes own methods as its callbacks.
    self.setCallbacks( self.onTouch, self.onProcessInteraction, self.onUntouch, self.onMatrixSet )
  def setHighlight( self, flag ):
    self._highlighted = flag
  def getHighlight( self ):
    return self._highlighted   
  def setColor( self, col ):
    self._color = col
  def getColor( self ):
    return self._color
  def setTexture( self, tex ):
    self._texture = tex
  def getTexture( self ):
    return self._texture
  def activateTexture( self ):
    if not self._texture:
      return False
    self._texture.activate()
    return True
  def deactivateTexture( self ):
    if self._texture:
      self._texture.deactivate()
  def activateColor( self ):
    ar_activateColor( self._color )
  def setVisible( self, vis ):
    self._visible = vis
  def getVisible( self ):
    return self._visible
  def onTouch( self, effector ):
    #print self.this, 'touched.'
    self.setHighlight( True )
    return True
  def onProcessInteraction( self, effector ):
    return True
  def onUntouch( self, effector ):
    #print self.this, 'untouched.'
    self.setHighlight( False )
    return True
  def onMatrixSet( self, matrix ):
    pass

%}

%pythoncode %{

# Utility routines for handling interaction between a single arEffector and a list
# of arPyInteractables.

# A Hack for getting the address portion of a SWIG pointer
# (used for determining object identity).

# going to try & write this better in C++...
def ar_getAddressFromSwigPtr( ptr ):
  # is this ever an ugly hack
  # A SWIG pointer (currently) has the form
  # _<address>_p_<type>
  tmp = ptr[1:] # throw away initial '_'
  return tmp[0:tmp.find('_')]


# The main interaction algorithm.

def ar_processInteractionList( effector, interactionList ):

  # Interact with the grabbed object, if any.
  if effector.hasGrabbedObject():

    # get the grabbed object.
    grabbedPtr = effector.getGrabbedObject()
    grabbedAddress = ar_getAddressFromSwigPtr(grabbedPtr.this)
    #grabbedAddress = grabbedPtr.this

    # If this effector has grabbed an object not in this list, dont
    # interact with any of this list
    grabbedObject = None
    for item in interactionList:
      # HACK! Found object in list
      if ar_getAddressFromSwigPtr(item.this) == grabbedAddress: 
      #if ar_compareSwigPtrs( item.this, grabbedAddress ):
      #if item.this == grabbedAddress:
        grabbedObject = item
        break
    if not grabbedObject:
      return False # not an error, just means no interaction occurred

    if grabbedObject.enabled():
      # If its grabbed an object in this set, interact only with that object.
      return grabbedObject.processInteraction( effector )

  # check to see if effector is already touching an object
  # if so, and it does not belong to this list, then abort
  oldTouchedObject = None
  oldTouchedPtr = None
  oldTouchedAddress = None
  if effector.hasTouchedObject():
    oldTouchedPtr = effector.getTouchedObject()
    oldTouchedAddress = ar_getAddressFromSwigPtr(oldTouchedPtr.this)
    #oldTouchedAddress = oldTouchedPtr.this
    for item in interactionList:
      # HACK! Found object in list
      if ar_getAddressFromSwigPtr(item.this) == oldTouchedAddress: 
      #if ar_compareSwigPtrs( item.this, oldTouchedAddress ):
      #if item.this == oldTouchedAddress:
        oldTouchedObject = item
    if not oldTouchedObject:
      return False # not an error, just means no interaction occurred
                   # with items in this list

  # Figure out the closest interactable to the effector (as determined
  # by their matrices). Go ahead and touch it (while untouching the
  # previously touched object if such are different).
  minDist = 1.e10    # A really big number, havent found how to get
                     # the max in python yet
  newTouchedObject = None
  for item in interactionList:
    if item.enabled():
      dist = effector.calcDistance( item.getMatrix() )
      if (dist >= 0.) and (dist < minDist):
        minDist = dist
        newTouchedObject = item
  newTouchedAddress = None
  if newTouchedObject:
    #newTouchedAddress = newTouchedObject.this
    newTouchedAddress = ar_getAddressFromSwigPtr(newTouchedObject.this)

  if oldTouchedPtr and oldTouchedAddress:
    if (not newTouchedAddress) or (newTouchedAddress != oldTouchedAddress):
    #if (not newTouchedAddress) or not ar_compareSwigPtrs(newTouchedAddress, oldTouchedAddress):
    #if (not newTouchedAddress) or newTouchedAddress != oldTouchedAddress:
      oldTouchedPtr.untouch( effector )

  if not newTouchedObject:
    # Not touching any objects.
    return False

  # Finally, and most importantly, process the action of the effector on
  # the interactable.
  return newTouchedObject.processInteraction( effector )



%}

/*    virtual void setTexture( arTexture* tex ) {_texture = tex; }*/
/*    virtual arTexture* getTexture() { return _texture; }*/
/*    virtual bool activateTexture() { if (!_texture) return false; _texture->activate(); return true; }*/
/*    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }*/


