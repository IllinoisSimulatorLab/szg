// $Id: PyMasterSlave.i,v 1.2 2005/03/31 18:04:18 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// ******************** based on arMasterSlaveFramework.h ********************

// Callback mechanism based on
// http://gd.tuwien.ac.at/softeng/SWIG/Examples/python/callback2/,
// with a number of changes. See also setDrawCallback(PyObject *PyFunc) etc.
// in the %extend section.

// NOTE: (JAC 3/31/05)!!!!!!!!!
// The reference to the framework object is now discarded, since you can pass a method
// of a Python framework subclass as the callback (and have access to all the framework
// methods via the 'self' parameter). I suggest you subclass the arPyMasterSlaveFramework
// below and override the provided callback methods (e.g. onDraw() for the draw callback,
// onPreExchange() for the preExchange callback, etc.

%{
// The following macro defines a C++-functions that serve as callbacks
// for the original C++ classes. Internally, these functions convert
// their arguments to Python classes and call the appropriate Python
// method.
#define MASTERSLAVECALLBACK(cbtype) \
static PyObject *py##cbtype##Func = NULL; \
static void py##cbtype##Callback( arMasterSlaveFramework& fw ) { \
    PyObject *arglist=Py_BuildValue("()");   \
    PyObject *result=PyEval_CallObject(py##cbtype##Func, arglist);  \
    if (result==NULL) { \
        PyErr_Print(); \
        string errmsg="A Python exception occurred in " #cbtype " callback.";\
        cerr << errmsg << "\n"; \
        throw  errmsg; \
    }\
    Py_XDECREF(result); \
    Py_DECREF(arglist); \
}


// Now we create a few callback functions, using the new macro.

//  void setDrawCallback(void (*draw)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Draw)
//  void setPreExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(PreExchange)
//  void setPostExchangeCallback(void (*postExchange)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(PostExchange)
//  void setWindowCallback(void (*windowCallback)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Window)
//  void setPlayCallback(void (*play)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Play)
//  void setOverlayCallback(void (*overlay)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Overlay)
//  void setExitCallback(void (*cleanup)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Exit)


// setInitcallback requires special treatment because the signature of
// its callback function differs from everybody else's.
//
//  void setStartCallback(bool (*initCallback)(arMasterSlaveFramework& fw, 
//                                            arSZGClient&));
static PyObject *pyStartFunc = NULL;
static bool pyStartCallback(arMasterSlaveFramework& fw,arSZGClient& cl) {
    PyObject *clobj = SWIG_NewPointerObj((void *) &cl,
                             SWIGTYPE_p_arSZGClient, 0);
    PyObject *arglist=Py_BuildValue("(O)",clobj);
    PyObject *result=PyEval_CallObject(pyStartFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in arMasterSlaveFramework start callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    bool res=(bool) PyInt_AsLong(result);
    Py_XDECREF(result);
    Py_DECREF(arglist);
    Py_DECREF(clobj);
    return res;
}


//  void setUserMessageCallback(
//    void (*userMessageCallback)(arMasterSlaveFramework&, const string&));
//
static PyObject *pyUserMessageFunc = NULL;
static void pyUserMessageCallback(arMasterSlaveFramework& fw,const string & s) {
    PyObject *arglist=Py_BuildValue("(s)",s.c_str());
    PyObject *result=PyEval_CallObject(pyUserMessageFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in UserMessage callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    Py_XDECREF(result);
    Py_DECREF(arglist);
}

// setReshapeCallback requires special treatment because the signature of
// its callback function differs from everybody else's.
//
// void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));

// Coded By: Brett Witt (brett.witt@gmail.com)
// Yet another function which requires special treatment because of
// its unique callback signature.
// void setKeyboardCallback(void (*keyboard)(arMasterSlaveFramework&,
//                                           unsigned char, int, int));
static PyObject *pyKeyboardFunc = NULL;
static void pyKeyboardCallback(arMasterSlaveFramework &fw, 
              unsigned char c, int x, int y)
{
   PyObject* arglist = Py_BuildValue("(s#,i,i)", &c, 1, x, y);
   PyObject* result = PyEval_CallObject(pyKeyboardFunc, arglist);

   if(result == NULL)
   {
      PyErr_Print();
      string errMsg = "A Python exception occured in Keyboard callback.";
      cerr << errMsg << "\n";
      throw errMsg;
   }

   Py_XDECREF(result);
   Py_DECREF(arglist);
}

// setSlaveConnectedCallback requires special treatment because the signature of
// its callback function differs from everybody else's.
//
// void setSlaveConnectedCallback(void (*connected)(arMasterSlaveFramework&,int));

static PyObject *pySlaveConnectedFunc = NULL;
static void pySlaveConnectedCallback(arMasterSlaveFramework& fw, int numConnected)
{
   PyObject* arglist = Py_BuildValue("(i)", numConnected);
   PyObject* result = PyEval_CallObject(pySlaveConnectedFunc, arglist);

   if(result == NULL)
   {
      PyErr_Print();
      string errMsg = "A Python exception occured in slaveConnected callback.";
      cerr << errMsg << "\n";
      throw errMsg;
   }

   Py_XDECREF(result);
   Py_DECREF(arglist);
}

// William Baker (wtbaker@uiuc.edu) 2004
//
// setReshapeCallback requires special treatment because the signature of
// its callback function differs from everybody else's.
//
// void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));
static PyObject *pyReshapeFunc = NULL;
static void pyReshapeCallback(arMasterSlaveFramework& fw, int x, int y) {
    PyObject *arglist=Py_BuildValue("(i,i)",x,y);
    PyObject *result=PyEval_CallObject(pyReshapeFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in Reshape callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    Py_XDECREF(result);
    Py_DECREF(arglist);
}

%}

/// Framework for cluster applications using one master and several slaves.
class arMasterSlaveFramework : public arSZGAppFramework {
 public:
  arMasterSlaveFramework();
  ~arMasterSlaveFramework();

// The callbacks are commented out because they required special
// handling (see extend section and macros)

//  void setDrawCallback(void (*draw)(arMasterSlaveFramework&));
//  void setStartCallback(bool (*initCallback)(arMasterSlaveFramework& fw, 
//                                            arSZGClient&));
//  void setPreExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
//  void setWindowCallback(void (*windowCallback)(arMasterSlaveFramework&));
//  void setPlayCallback(void (*play)(arMasterSlaveFramework&));
//  void setOverlayCallback(void (*overlay)(arMasterSlaveFramework&));
//  void setExitCallback(void (*cleanup)(arMasterSlaveFramework&));
//  void setUserMessageCallback(
//    void (*userMessageCallback)(arMasterSlaveFramework&, const string&));
//  void setPostExchangeCallback(bool (*postExchange)(arMasterSlaveFramework&));
//  void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));
//  void setKeyboardCallback(void (*keyboard)(arMasterSlaveFramework&,
//                                            unsigned char, int, int));
//  void setSlaveConnectedCallback(void (*connected)(arMasterSlaveFramework&, int));

  bool getStandalone(); // Are we running in stand-alone mode?
  int getNumberSlavesConnected() const; // If master, number of slaves connected.

  // initializes the various pieces but does not start the event loop
  bool init(int&, char**);
  // starts services and the default GLUT-based event loop
  bool start();
  // lets us work without the GLUT event loop... i.e. it starts the various
  // services but lets the user control the event loop
  bool startWithoutGLUT();
  // shut-down for much (BUT NOT ALL YET) of the arMasterSlaveFramework
  // if the parameter is set to true, we will block until the display thread
  // exits
  void stop(bool blockUntilDisplayExit);

  void preDraw();
  void drawWindow();
  void postDraw();

  arMatrix4 getProjectionMatrix(float eyeSign); // needed for custom stuff
  arMatrix4 getModelviewMatrix(float eyeSign);  // needed for custom stuff

  int             getWindowSizeX() const { return _windowSizeX; }
  int             getWindowSizeY() const { return _windowSizeY; }

  // Various methods of sharing data from the master to the slaves...

  // The simplest way to share data from master to slaves. Global variables
  // are registered with the framework (the transfer fields). Each frame,
  // the master dumps its values and sends them to the slaves. Note:
  // each field must be of a fixed size.
  //bool addTransferField( string fieldName, void* data, 
  //                       arDataType dataType, int size );
  // Another way to share data. Here, the framework manages the memory
  // blocks. This allows them to be resized.
  // Add an internally-allocated transfer field. These can be resized on the
  // master and the slaves will automatically follow suit.
  //bool addInternalTransferField( string fieldName, 
  //                               arDataType dataType, 
  //                               int size );
  //bool setInternalTransferFieldSize( string fieldName, 
  //                                   arDataType dataType, 
  //                                   int newSize );
  // Get a pointer to either an externally- or an internally-stored 
  // transfer field. Note that the internally stored fields are resizable.
  //void* getTransferField( string fieldName, 
  //                        arDataType dataType,
  //                        int& size );

  // A final way to share data from the master to the slaves. In this case,
  // the programmer registers arFrameworkObjects with the frameworks
  // internal arMasterSlaveDataRouter. An identical set of arFrameworkObjects
  // is created by each of the master and slave instances. The
  // arMasterSlaveDataRouter routes messages between them.
  bool registerFrameworkObject(arFrameworkObject* object){
    return _dataRouter.registerFrameworkObject(object);
  }

  void setPlayTransform();
  void draw();

  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory);

  // tiny functions that only appear in the .h

  // If set to true (the default), the framework will swap the graphics
  // buffers itself. Otherwise, the application will do so. This is
  // really a bit of a hack.
  void internalBufferSwap(bool state){ _internalBufferSwap = state; }
  void loadNavMatrix() { arMatrix4 temp = ar_getNavInvMatrix();
                         glMultMatrixf( temp.v ); }
  //void setAnaglyphMode(bool isOn) { _anaglyphMode = isOn; }
  bool setViewMode( const string& viewMode ); 
  float getCurrentEye() const { return _graphicsWindow.getCurrentEyeSign(); }
  /// msec since the first I/O poll (not quite start of the program).
  double getTime() const { return _time; }
  /// How many msec it took to compute/draw the last frame.
  double getLastFrameTime() const { return _lastFrameTime; }
  bool getMaster() const { return _master; }
  bool getConnected() const { return _master || _stateClientConnected; }
  bool soundActive() const { return _soundActive; }
  bool inputActive() const { return _inputActive; }

%extend{

// The following functions expose a version of the data transfer mechanism
// to Python. Fundamentally, the idea is to share lists of integers (or
// lists of floats) between the master and the slaves.
//
// Basic API:
// 1. Create a transfer list:
//      fw.initIntTransfer('foo')
// 2. Modifiy the list (only for Master in preExchange callback):
//      fw.setIntList('foo',range(100))   # send in a complete list
// or
//      fw.setIntListItem('foo',5,3)      # set 5th entry to 3
// 3. Retrieve the list (in postExchange callback):
//      fw.getIntList('foo')    # retrieve the complete list
// or
//      fw.getIntListItem('foo',5)      # retrieve 5th entry
//
// There are similar methods for lists of floats (transferFloatList, etc.)
// and for strings (transferString, getStringSize, setString, getString).
//
// The string transfer mechanism provides a way of transferring Python
// objects via cPickle.dumps and cPickle.loads. This approach is encapsulated
// in the methods initObjectTransfer, setObject, and getObject:
//      fw.initObjectTransfer('spam')
//      fw.setObject('spam',someObject)
//      someObject=fw.getObject('spam')

PyObject *initIntTransfer(char* name) {
    if (self->addInternalTransferField(string(name),AR_INT,1)) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
        return NULL;
    }
}
PyObject *getIntListSize(char *name) {
    int size;
    int *ptr=(int *) self->getTransferField(string(name),AR_INT,size);
    return PyInt_FromLong(size);
}
PyObject *setIntListItem(char* name,int idx,int val) {
    int size;
    int *ptr=(int *) self->getTransferField(string(name),AR_INT,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    ptr[idx]=val;
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getIntListItem(char* name,int idx) {
    int size;
    int *ptr=(int *) self->getTransferField(string(name),AR_INT,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    return PyInt_FromLong(ptr[idx]);
}
PyObject *setIntList(char* name,PyObject *lst) {
    int osize;
    int *ptr=(int *) self->getTransferField(string(name),AR_INT,osize);
    int size=PyList_Size(lst);
    if (osize!=size) {
        if (!self->setInternalTransferFieldSize(string(name),AR_INT,size)) {
            PyErr_SetString(PyExc_MemoryError,
                    "unable to resize transfer field");
            return NULL;
        }
    }
    ptr=(int *) self->getTransferField(string(name),AR_INT,size);
    for(int i=0;i<size;i++) {
        ptr[i]=PyInt_AsLong(PyList_GetItem(lst,i));
    }
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getIntList(char* name) {
    int size;
    int *ptr=(int *) self->getTransferField(string(name),AR_INT,size);
    PyObject *lst=PyList_New(size);
    for(int i=0;i<size;i++) {
        PyList_SetItem(lst,i,PyInt_FromLong(ptr[i]));
    }
    return lst;
}

PyObject *initFloatTransfer(char* name) {
    if (self->addInternalTransferField(string(name),AR_FLOAT,1)) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
        return NULL;
    }
}
PyObject *getFloatListSize(char *name) {
    int size;
    int *ptr=(int *) self->getTransferField(string(name),AR_FLOAT,size);
    return PyInt_FromLong(size);
}
PyObject *setFloatListItem(char* name,int idx,float val) {
    int size;
    float *ptr=(float *) self->getTransferField(string(name),AR_FLOAT,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    ptr[idx]=val;
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getFloatListItem(char* name,int idx) {
    int size;
    float *ptr=(float *) self->getTransferField(string(name),AR_FLOAT,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    return PyFloat_FromDouble(ptr[idx]);
}
PyObject *setFloatList(char* name,PyObject *lst) {
    int osize;
    float *ptr=(float *) self->getTransferField(string(name),AR_FLOAT,osize);
    int size=PyList_Size(lst);
    if (osize!=size) {
        if (!self->setInternalTransferFieldSize(string(name),AR_FLOAT,size)) {
            PyErr_SetString(PyExc_MemoryError,
                    "unable to resize transfer field");
            return NULL;
        }
    }
    ptr=(float *) self->getTransferField(string(name),AR_FLOAT,size);
    for(int i=0;i<size;i++) {
        ptr[i]=PyFloat_AsDouble(PyList_GetItem(lst,i));
    }
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getFloatList(char* name) {
    int size;
    float *ptr=(float *) self->getTransferField(string(name),AR_FLOAT,size);
    PyObject *lst=PyList_New(size);
    for(int i=0;i<size;i++) {
        PyList_SetItem(lst,i,PyFloat_FromDouble(ptr[i]));
    }
    return lst;
}

PyObject *initStringTransfer(char* name) {
    if (self->addInternalTransferField(string(name),AR_CHAR,1)) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
        return NULL;
    }
}
PyObject *getStringSize(char *name) {
    int size;
    char *ptr=(char *) self->getTransferField(string(name),AR_CHAR,size);
    return PyInt_FromLong(size);
}
PyObject *setString(char *name,PyObject *val) {
    if (!PyString_Check(val)) {
        PyErr_SetString(PyExc_ValueError, "arMasterSlaveFramework::setString() called with non-string");
        return NULL;
    }
    int osize;
    char *ptr=(char *) self->getTransferField(string(name),AR_CHAR,osize);
    int size = PyString_Size(val);
    if (osize!=size) {
        if (!self->setInternalTransferFieldSize(string(name),AR_CHAR,size)) {
            PyErr_SetString(PyExc_MemoryError,
                    "unable to resize transfer field");
            return NULL;
        }
    }
    ptr=(char *) self->getTransferField(string(name),AR_CHAR,size);
    char* buf;
    if (PyString_AsStringAndSize( val, &buf, &osize )==-1) {
        PyErr_SetString(PyExc_RuntimeError, "arMasterSlaveFramework::setString() failed to get string pointer.");
        return NULL;
    }
    memcpy( ptr, buf, size );
    Py_INCREF(Py_None);
    return Py_None;
}
/*PyObject *setString(char *name,char *val) {*/
/*    int osize;*/
/*    char *ptr=(char *) self->getTransferField(string(name),AR_CHAR,osize);*/
/*    int size=strlen(val);*/
/*    if (osize!=size) {*/
/*        if (!self->setInternalTransferFieldSize(string(name),AR_CHAR,size)) {*/
/*            PyErr_SetString(PyExc_MemoryError,*/
/*                    "unable to resize transfer field");*/
/*            return NULL;*/
/*        }*/
/*    }*/
/*    ptr=(char *) self->getTransferField(string(name),AR_CHAR,size);*/
/*    for(int i=0;i<size;i++) {*/
/*        ptr[i]=val[i];*/
/*    }*/
/*    Py_INCREF(Py_None);*/
/*    return Py_None;*/
/*}*/
PyObject *getString(char* name) {
    int size;
    char *ptr=(char *) self->getTransferField(string(name),AR_CHAR,size);
    return PyString_FromStringAndSize(ptr,size);
}

// A macro that yields the body of methods that set callback functions
// Note how reference counters are increased and decreased to avoid
// memory leaks (or so I hope...)
#define SETMASTERSLAVECALLBACK(cbtype) \
void set##cbtype##Callback(PyObject *PyFunc) {\
    Py_XDECREF(py##cbtype##Func); \
    Py_XINCREF(PyFunc); \
    py##cbtype##Func = PyFunc; \
    self->set##cbtype##Callback(py##cbtype##Callback);\
}


//  void setDrawCallback(void (*draw)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(Draw)

//  void setStartCallback(bool (*initCallback)(arMasterSlaveFramework& fw, 
//                                            arSZGClient&));
    SETMASTERSLAVECALLBACK(Start)

// void setKeyboardCallback(void (*keyboard)(arMasterSlaveFramework&,
//                                           unsigned char, int, int));
    SETMASTERSLAVECALLBACK(Keyboard)

//  void setPreExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(PreExchange)

//  void setWindowCallback(void (*windowCallback)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(Window)

//  void setPlayCallback(void (*play)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(Play)

//  void setOverlayCallback(void (*overlay)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(Overlay)

//  void setExitCallback(void (*cleanup)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(Exit)

//  void setUserMessageCallback(
//    void (*userMessageCallback)(arMasterSlaveFramework&, const string&));
    SETMASTERSLAVECALLBACK(UserMessage)

//  void setPostExchangeCallback(bool (*postExchange)(arMasterSlaveFramework&));
    SETMASTERSLAVECALLBACK(PostExchange)

//  void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));
    SETMASTERSLAVECALLBACK(Reshape)

//  void setSlaveConnectedCallback(void (*connected)(arMasterSlaveFramework&, int));
    SETMASTERSLAVECALLBACK(SlaveConnected)

}

%pythoncode %{
    def initObjectTransfer(self,name): self.initStringTransfer(name)
    def setObject(self,name,obj): self.setString(name,cPickle.dumps(obj))
    def getObject(self,name): return cPickle.loads(self.getString(name))
%}
};

%pythoncode %{

# Python master-slave framework subclass that installs its own methods as the
# callbacks. Sub-class and override.
#
# NOTE: the defaults for the window init and reshape callbacks make opengl
# calls. In order to avoid making the pyszg module dependent on pyopengl
# (and thus on numarray), I have not installed those two callbacks below.
# Since these two callbacks are not used all that often, this seemed like
# the easier thing to do. If you want to use them, youll need to install
# them in your framework class __init__(), as below.

class arPyMasterSlaveFramework( arMasterSlaveFramework ):
  def __init__(self):
    arMasterSlaveFramework.__init__(self)
    self.setStartCallback( self.onStart )
    self.setSlaveConnectedCallback( self.onSlaveConnected )
    self.setPreExchangeCallback( self.onPreExchange )
    self.setPostExchangeCallback( self.onPostExchange )
    self.setOverlayCallback( self.onOverlay )
    self.setDrawCallback( self.onDraw )
    self.setPlayCallback( self.onPlay )
    self.setKeyboardCallback( self.onKey )
    self.setUserMessageCallback( self.onUserMessage )
    self.setExitCallback( self.onExit )
    print '-----------------------------------------------------------------------------------------------'
    print 'arPyMasterSlaveFramework remark: so as not to force module dependence on pyopengl and numarray,'
    print '    window init and reshape callbacks are not installed. If you need to override the default'
    print '    behaviors, call setReshapeCallback() or setWindowCallback() in your frameworks __init__().'
    print '-----------------------------------------------------------------------------------------------'

# Not installed, see note above.
#    self.setWindowCallback( self.onWindowInit )
#    self.setReshapeCallback( self.onReshape )
#  def onReshape( self, width, height ):
#    glViewport( 0, 0, width, height )
#  def onWindowInit( self ):
#    glEnable(GL_DEPTH_TEST)
#    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE )
#    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

  def onStart( self, client ):
    return True
  def onSlaveConnected( self, numConnected ):
    pass
  def onPreExchange( self ):
    pass
  def onPostExchange( self ):
    pass
  def onOverlay( self ):
    pass
  def onDraw( self ):
    pass
  def onPlay( self ):
    pass
  def onKey( self, key, mouseX, mouseY ):
    pass
  def onUserMessage( self, messageBody ):
    pass
  def onExit( self ):
    pass

# Utility classes

# Auto-resizeable container for identical drawable objects.
# Usage:
# (1) add or delete objects from the objList property as desired on the master.
# (2) call dumpStateToString() in the preExchange() & use the frameworks
#       setString() method.
# (3) call frameworks getString() in postExchange() & setStateFromString()
#       method to finish synchronization.
# The class __init__ method will be called with the appropriate stateTuple
# for each new object if the list has been expanded on the master, so the
# __init__() should call setStateArgs(). If the
# correct number of objects already exists, then each will have its
# setStateArgs() method called directly.
class arMasterSlaveListSync:
  # Note that classFactory should be the name of the object class.
  def __init__(self,classFactory,useCPickle=False):
    self.objList = []
    self.classFactory = classFactory
    self.useCPickle = useCPickle
  def draw(self):
    for item in self.objList:
      item.draw()
  def dumpStateToString(self):
    stateList = []
    for item in self.objList:
      stateList.append(item.getStateArgs())
    # pickle seems to be considerably less flaky than cPickle here
    if self.useCPickle:
      pickleString = cPickle.dumps(stateList)
    else:
      if hasattr(pickle,'HIGHEST_PROTOCOL'):
        pickleString = pickle.dumps(stateList,pickle.HIGHEST_PROTOCOL)
      else:
        pickleString = pickle.dumps(stateList,True)
    return pickleString
  def setStateFromString(self,pickleString):
    stateList = cPickle.loads(pickleString)
    numItems = len(stateList)
    myNumItems = len(self.objList)
    if numItems < myNumItems:
      self.objList = self.objList[:numItems]
      myNumItems = numItems
    for i in range(myNumItems):
      self.objList[i].setStateArgs( stateList[i] )
    if numItems > myNumItems:
      for i in range(myNumItems,numItems):
        self.objList.append( self.classFactory( stateList[i] ) )


%}
