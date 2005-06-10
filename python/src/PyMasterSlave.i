// $Id: PyMasterSlave.i,v 1.8 2005/06/10 15:26:59 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// ******************** based on arMasterSlaveFramework.h ********************

%{
#include "arMasterSlaveFramework.h"
#include "arGraphicsWindow.h"
%}

// Callback mechanism based on
// http://gd.tuwien.ac.at/softeng/SWIG/Examples/python/callback2/,
// with a number of changes. See also setDrawCallback(PyObject *PyFunc) etc.
// in the %extend section.
%{
// The following macro defines a C++-functions that serve as callbacks
// for the original C++ classes. Internally, these functions convert
// their arguments to Python classes and call the appropriate Python
// method.
#define MASTERSLAVECALLBACK(cbtype) \
static PyObject *py##cbtype##Func = NULL; \
static void py##cbtype##Callback(arMasterSlaveFramework& fw) { \
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw, \
                             SWIGTYPE_p_arMasterSlaveFramework, 0);\
    PyObject *arglist=Py_BuildValue("(O)",fwobj);   \
    PyObject *result=PyEval_CallObject(py##cbtype##Func, arglist);  \
    if (result==NULL) { \
        PyErr_Print(); \
        string errmsg="A Python exception occurred in " #cbtype " callback.";\
        cerr << errmsg << "\n"; \
        throw  errmsg; \
    }\
    Py_XDECREF(result); \
    Py_DECREF(arglist); \
    Py_DECREF(fwobj); \
}


// Now we create a few callback functions, using the new macro.

//  void setDrawCallback(void (*draw)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Draw)
//  void setPreExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(PreExchange)
//  void setPostExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(PostExchange)
//  void setWindowCallback(void (*windowCallback)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Window)
//  void setPlayCallback(void (*play)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Play)
//  void setOverlayCallback(void (*overlay)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Overlay)
//  void setExitCallback(void (*cleanup)(arMasterSlaveFramework&));
MASTERSLAVECALLBACK(Exit)


// start callback requires special treatment because the signature of
// its callback function differs from everybody elses.
//
//  void setStartCallback(bool (*initCallback)(arMasterSlaveFramework& fw, 
//                                            arSZGClient&));
static PyObject *pyStartFunc = NULL;
static bool pyStartCallback(arMasterSlaveFramework& fw,arSZGClient& cl) {
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                             SWIGTYPE_p_arMasterSlaveFramework, 0);
    PyObject *clobj = SWIG_NewPointerObj((void *) &cl,
                             SWIGTYPE_p_arSZGClient, 0);
    PyObject *arglist=Py_BuildValue("(O,O)",fwobj,clobj);
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
    Py_DECREF(fwobj);
    return res;
}


//  void setUserMessageCallback(
//    void (*userMessageCallback)(arMasterSlaveFramework&, const string&));
//
static PyObject *pyUserMessageFunc = NULL;
static void pyUserMessageCallback(arMasterSlaveFramework& fw,const string & s) {
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                             SWIGTYPE_p_arMasterSlaveFramework, 0);
    PyObject *arglist=Py_BuildValue("(O,s)",fwobj,s.c_str());
    PyObject *result=PyEval_CallObject(pyUserMessageFunc, arglist);
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

// Coded By: Brett Witt (brett.witt@gmail.com)
// Yet another function which requires special treatment because of
// its unique callback signature.
// void setKeyboardCallback(void (*keyboard)(arMasterSlaveFramework&,
//                                           unsigned char, int, int));
static PyObject *pyKeyboardFunc = NULL;
static void pyKeyboardCallback(arMasterSlaveFramework &fw, 
              unsigned char c, int x, int y)
{
   PyObject* fwobj = SWIG_NewPointerObj((void*) &fw,
                                        SWIGTYPE_p_arMasterSlaveFramework, 0);

   PyObject* arglist = Py_BuildValue("(O,s#,i,i)", fwobj, &c, 1, x, y);
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
   Py_DECREF(fwobj);
}

// setSlaveConnectedCallback requires special treatment because the signature of
// its callback function differs from everybody elses.
//
// void setSlaveConnectedCallback(void (*connected)(arMasterSlaveFramework&,int));

static PyObject *pySlaveConnectedFunc = NULL;
static void pySlaveConnectedCallback(arMasterSlaveFramework& fw, int numConnected)
{
   PyObject* fwobj = SWIG_NewPointerObj((void*) &fw,
                                        SWIGTYPE_p_arMasterSlaveFramework, 0);
   PyObject* arglist = Py_BuildValue("(O,i)", fwobj, numConnected);
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
   Py_DECREF(fwobj);
}

// William Baker (wtbaker@uiuc.edu) 2004
//
// setReshapeCallback requires special treatment because the signature of
// its callback function differs from everybody else's.
//
// void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));
static PyObject *pyReshapeFunc = NULL;
static void pyReshapeCallback(arMasterSlaveFramework& fw, int x, int y) {
    PyObject *fwobj = SWIG_NewPointerObj((void *) &fw,
                             SWIGTYPE_p_arMasterSlaveFramework, 0);
    PyObject *arglist=Py_BuildValue("(O,i,i)",fwobj,x,y);
    PyObject *result=PyEval_CallObject(pyReshapeFunc, arglist);
    if (result==NULL) {
        PyErr_Print();
        string errmsg="A Python exception occurred in Reshape callback.";
        cerr << errmsg << "\n";
        throw  errmsg;
    }
    Py_XDECREF(result);
    Py_DECREF(arglist);
    Py_DECREF(fwobj);
}

bool ar_packSequenceData( PyObject* seq, std::vector<int>& typeData,
                            std::vector<long>& intData, std::vector<double>& floatData,
                            std::vector<char*>& stringData, std::vector<int>& stringSizeData ) {
  if (!PySequence_Check( seq )) {
    PyErr_SetString( PyExc_TypeError, 
         "arMasterSlaveFramework error: ar_packSequenceData() requires a sequence." );
    return false;
  }
  int len = PySequence_Size( seq );
  // push the size of the sequence
  typeData.push_back(len);
  for (int i=0; i<len; ++i) {
    PyObject* item = PySequence_GetItem( seq, i );
 
    // Its an Int
    if (PyInt_Check( item )) {
      intData.push_back( PyInt_AsLong( item ) );
      typeData.push_back( AR_LONG );

    // Its a Float
    } else if (PyFloat_Check(item)) {
      floatData.push_back( PyFloat_AsDouble( item ) );
      typeData.push_back( AR_DOUBLE );

    // Its a String
    } else if (PyString_Check( item )) {
      char* cdata;
      int ssize;
      PyString_AsStringAndSize( item, &cdata, &ssize );
      stringData.push_back( cdata );
      stringSizeData.push_back( ssize+1 );
      typeData.push_back( AR_CHAR );

    // Its a nested sequence
    } else if (PySequence_Check( item )) {

      // push the sequence marker. the size of the subsequence will be pushed at the 
      // beginning of the recurse.
      typeData.push_back(-1);
      bool stat = ar_packSequenceData( item, typeData, intData, floatData, stringData, stringSizeData );
      if (!stat) {
        return false;
      }

    // Its a what?
    } else {
      cerr << "arMasterSlaveFramework warning: invalid data type ignored in setSequence().\n";
    }
  }
}

PyObject* ar_unpackSequenceData( int** typePtrPtr, long** intPtrPtr, double** floatPtrPtr, char** charPtrPtr) {
  int* typePtr = *typePtrPtr;
  long* intDataPtr = *intPtrPtr;
  double* floatDataPtr = *floatPtrPtr;
  char* charPtr = *charPtrPtr;
  int size = *typePtr++;
  PyObject* seq = PyTuple_New( size );
  if (!seq) {
    PyErr_SetString( PyExc_MemoryError, "unable to get allocate new tuple." );
    return NULL;
  }
  int stringLength;
  PyObject* nestedSequence;
  for(int i=0; i<size; ++i) {
    switch (*typePtr++) {
      case AR_LONG:
        PyTuple_SetItem( seq, i, PyInt_FromLong(*intDataPtr++) );
        break;
      case AR_DOUBLE:
        PyTuple_SetItem( seq, i, PyFloat_FromDouble(*floatDataPtr++) );
        break;
      case AR_CHAR:
        stringLength = strlen( charPtr );
        PyTuple_SetItem( seq, i, PyString_FromStringAndSize( charPtr, stringLength ) );
        charPtr += stringLength+1;
        break;
      case -1:  // my arbitrary "seqeuence" type
        nestedSequence = ar_unpackSequenceData( &typePtr, &intDataPtr, &floatDataPtr, &charPtr );
        if (!nestedSequence) {
          PyErr_SetString(PyExc_TypeError, "arMasterSlaveFramework error: ar_unpackSequenceData() failed.");
          Py_DECREF( seq );
          return NULL;
        }
        PyTuple_SetItem( seq, i, nestedSequence );
        break;
      default:
        PyErr_SetString(PyExc_TypeError, "arMasterSlaveFramework error: invalid type in getSequenceAsTuple.");
        Py_DECREF( seq );
        return NULL;
    }
  }
  *typePtrPtr = typePtr;
  *intPtrPtr = intDataPtr;
  *floatPtrPtr = floatDataPtr;
  *charPtrPtr = charPtr;
  return seq;
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

  void setGlutDisplayMode( unsigned int glutDisplayMode ) { _glutDisplayMode = glutDisplayMode; }
  unsigned int getGlutDisplayMode() const { return _glutDisplayMode; }

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
  void draw();  // HEAVILY deprecated, use the following:
  void drawGraphicsDatabase();

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

PyObject *initSequenceTransfer(char* name) {
  string nameStr(name);
  if (!self->addInternalTransferField(nameStr+string("_INTDATA"),AR_LONG,1)) {
      PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
      return NULL;
  }
  if (!self->addInternalTransferField(nameStr+string("_FLOATDATA"),AR_DOUBLE,1)) {
      PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
      return NULL;
  }
  if (!self->addInternalTransferField(nameStr+string("_CHARDATA"),AR_CHAR,1)) {
      PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
      return NULL;
  }
  if (!self->addInternalTransferField(nameStr+string("_TYPES"),AR_INT,1)) {
      PyErr_SetString(PyExc_MemoryError,"unable to create transfer field");
      return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* setSequence( char* name, PyObject *seq ) {
  std::vector<int> typeData;
  std::vector<long> intData;
  std::vector<double> floatData;
  std::vector<char *> stringData;
  std::vector<int> stringSizeData;
  if (!ar_packSequenceData( seq, typeData, intData, floatData, stringData, stringSizeData )) {
    cerr << "arMasterSlaveFramework error: setSequence() failed.\n";
    return NULL;
  }

  string nameStr(name);
  string intString = nameStr+string("_INTDATA");
  string floatString = nameStr+string("_FLOATDATA");
  string charString = nameStr+string("_CHARDATA");
  string typeString = nameStr+string("_TYPES");

  int totalStringSize(0);
  std::vector<int>::iterator iter;
  for (iter = stringSizeData.begin(); iter != stringSizeData.end(); ++iter) {
    totalStringSize += *iter;
  }
  if (!self->setInternalTransferFieldSize( typeString, AR_INT, (int)typeData.size() )) {
    cerr << "arMasterSlaveFramework error: unable to resize 'type' transfer field to "
         << typeData.size() << endl;
    return NULL;
  }
  if (!intData.empty()) {
    if (!self->setInternalTransferFieldSize( intString, AR_LONG, (int)intData.size() )) {
      cerr << "arMasterSlaveFramework error: unable to resize 'intData' transfer field to "
           << intData.size() << endl;
      return NULL;
    }
  }
  if (!floatData.empty()) {
    if (!self->setInternalTransferFieldSize( floatString, AR_DOUBLE, (int)floatData.size() )) {
      cerr << "arMasterSlaveFramework error: unable to resize 'floatData' transfer field to "
           << floatData.size() << endl;
      return NULL;
    }
  }
  if (totalStringSize > 0) {
    if (!self->setInternalTransferFieldSize( charString, AR_CHAR, totalStringSize )) {
      cerr << "arMasterSlaveFramework error: unable to resize 'stringData' transfer field to "
           << totalStringSize << endl;
      return NULL;
    }
  }

  int i, osize;
  long* typePtr=(long *)self->getTransferField( typeString, AR_INT, osize );
  if (!typePtr) {
    PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
    return NULL;
  }
  for(i=0; i<(int)typeData.size(); ++i) {
      typePtr[i] = typeData[i];
  }
  if (!intData.empty()) {
    long* longPtr=(long *)self->getTransferField( intString, AR_LONG, osize );
    if (!longPtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      return NULL;
    }
    for(i=0; i<(int)intData.size(); ++i) {
        longPtr[i] = intData[i];
    }
  }
  if (!floatData.empty()) {
    double* doublePtr=(double *)self->getTransferField( floatString, AR_DOUBLE, osize );
    if (!doublePtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      return NULL;
    }
    for(i=0; i<(int)floatData.size(); ++i) {
        doublePtr[i] = floatData[i];
    }
  }
  if (totalStringSize > 0) {
    char* charPtr=(char *)self->getTransferField( charString, AR_CHAR, osize );
    if (!charPtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      return NULL;
    }
    for(i=0; i<(int)stringData.size(); ++i) {
      memcpy( charPtr, stringData[i], stringSizeData[i] );
      charPtr += stringSizeData[i];
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* getSequence( char* name ) {
  string nameStr(name);
  int size;
  int* typePtr=(int *)self->getTransferField( nameStr+string("_TYPES"), AR_INT, size );
  if (!typePtr) {
    PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
    return NULL;
  }
  long* intDataPtr=(long *)self->getTransferField( nameStr+string("_INTDATA"), AR_LONG, size );
  if (!intDataPtr) {
    PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
    return NULL;
  }
  double* floatDataPtr=(double *)self->getTransferField( nameStr+string("_FLOATDATA"), AR_DOUBLE, size );
  if (!floatDataPtr) {
    PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
    return NULL;
  }
  char* charPtr=(char *)self->getTransferField( nameStr+string("_CHARDATA"), AR_CHAR, size );
  if (!charPtr) {
    PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
    return NULL;
  }
  return ar_unpackSequenceData( &typePtr, &intDataPtr, &floatDataPtr, &charPtr );
}


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
    if (self->addInternalTransferField(string(name),AR_LONG,1)) {
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
    long *ptr=(long *) self->getTransferField(string(name),AR_LONG,size);
    return PyInt_FromLong((long)size);
}
PyObject *setIntListItem(char* name,int idx,long val) {
    int size;
    long *ptr=(long *) self->getTransferField(string(name),AR_LONG,size);
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
    long *ptr=(long *) self->getTransferField(string(name),AR_LONG,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    return PyInt_FromLong(ptr[idx]);
}
PyObject *setIntList(char* name,PyObject *lst) {
    int osize;
    long *ptr=(long *) self->getTransferField(string(name),AR_LONG,osize);
    int size=PyList_Size(lst);
    if (osize!=size) {
        if (!self->setInternalTransferFieldSize(string(name),AR_LONG,size)) {
            PyErr_SetString(PyExc_MemoryError,
                    "unable to resize transfer field");
            return NULL;
        }
    }
    ptr=(long *) self->getTransferField(string(name),AR_LONG,size);
    for(int i=0;i<size;i++) {
        ptr[i]=PyInt_AsLong(PyList_GetItem(lst,i));
    }
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getIntList(char* name) {
    int size;
    long *ptr=(long *) self->getTransferField(string(name),AR_LONG,size);
    PyObject *lst=PyList_New(size);
    for(int i=0;i<size;i++) {
        PyList_SetItem(lst,i,PyInt_FromLong(ptr[i]));
    }
    return lst;
}

PyObject *initFloatTransfer(char* name) {
    if (self->addInternalTransferField(string(name),AR_DOUBLE,1)) {
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
    double *ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,size);
    return PyInt_FromLong((long)size);
}
PyObject *setFloatListItem(char* name,int idx,double val) {
    int size;
    double *ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,size);
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
    double *ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,size);
    if ((idx>=size) || (idx<0)) {
        PyErr_SetString(PyExc_IndexError,"index out of range");
        return NULL;
    }
    return PyFloat_FromDouble(ptr[idx]);
}
PyObject *setFloatList(char* name,PyObject *lst) {
    int osize;
    double *ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,osize);
    int size=PyList_Size(lst);
    if (osize!=size) {
        if (!self->setInternalTransferFieldSize(string(name),AR_DOUBLE,size)) {
            PyErr_SetString(PyExc_MemoryError,
                    "unable to resize transfer field");
            return NULL;
        }
    }
    ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,size);
    for(int i=0;i<size;i++) {
        ptr[i]=PyFloat_AsDouble(PyList_GetItem(lst,i));
    }
    Py_INCREF(Py_None);
    return Py_None;
}
PyObject *getFloatList(char* name) {
    int size;
    double *ptr=(double *) self->getTransferField(string(name),AR_DOUBLE,size);
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
    return PyInt_FromLong((long)size);
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

    def initStructTransfer( self, name ):
      self.initStringTransfer( name+'_FORMAT' )
      self.initStringTransfer( name+'_DATA' )
    def setStruct( self, name, format, obj ):
      self.setString( name+'_FORMAT', format )
      tmp = struct.pack( format, *obj )
      self.setString( name+'_DATA', tmp )
    def getStruct( self, name ):
      format = self.getString( name+'_FORMAT' )
      data = self.getString( name+'_DATA' )
      return struct.unpack( format, data )
%}
};

void ar_defaultWindowInitCallback();
void ar_defaultReshapeCallback( PyObject* widthObject, PyObject* heightObject );

%{

void ar_defaultReshapeCallback( PyObject* widthObject, PyObject* heightObject ) {
  if ((!PyInt_Check(widthObject))||(!PyInt_Check(heightObject))) {
    PyErr_SetString(PyExc_TypeError,"ar_defaultReshapeCallback error: glViewport params must be ints");
  }
  GLsizei width = (GLsizei)PyInt_AsLong( widthObject );
  GLsizei height = (GLsizei)PyInt_AsLong( heightObject );
  glViewport( 0, 0, width, height );
}
%}

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
    self.setStartCallback( self.startCallback )
    self.setSlaveConnectedCallback( self.slaveConnectedCallback )
    self.setPreExchangeCallback( self.preExchangeCallback )
    self.setPostExchangeCallback( self.postExchangeCallback )
    self.setWindowCallback( self.windowInitCallback )
    self.setReshapeCallback( self.reshapeCallback )
    self.setOverlayCallback( self.overlayCallback )
    self.setDrawCallback( self.drawCallback )
    self.setPlayCallback( self.playCallback )
    self.setKeyboardCallback( self.keyboardCallback )
    self.setUserMessageCallback( self.userMessageCallback )
    self.setExitCallback( self.exitCallback )

  def startCallback( self, framework, client ):
    return self.onStart( client )
  def onStart( self, client ):
    return True
  def slaveConnectedCallback( self, framework, numConnected ):
    self.onSlaveConnected( numConnected )
  def onSlaveConnected( self, numConnected ):
    pass
  def preExchangeCallback( self, framework ):
    self.onPreExchange()
  def onPreExchange( self ):
    pass
  def postExchangeCallback( self, framework ):
    self.onPostExchange()
  def onPostExchange( self ):
    pass
  def windowInitCallback( self, framework ):
    self.onWindowInit()
  def onWindowInit( self ):
    ar_defaultWindowInitCallback()
  def reshapeCallback( self, framework, width, height ):
    self.onReshape( width, height )
  def onReshape( self, width, height ):
    ar_defaultReshapeCallback( width, height )
  def overlayCallback( self, framework ):
    self.onOverlay()
  def onOverlay( self ):
    pass
  def drawCallback( self, framework ):
    self.onDraw()
  def onDraw( self ):
    pass
  def playCallback( self, framework ):
    self.onPlay()
  def onPlay( self ):
    pass
  def keyboardCallback( self, framework, key, mouseX, mouseY ):
    self.onKey( key, mouseX, mouseY )
  def onKey( self, key, mouseX, mouseY ):
    pass
  def userMessageCallback( self, framework, messageBody ):
    self.onUserMessage( messageBody )
  def onUserMessage( self, messageBody ):
    pass
  def exitCallback( self, framework ):
    self.onExit()
  def onExit( self ):
    pass

# Utility classes

import UserDict

class arMasterSlaveDict(UserDict.IterableUserDict):
  """
  arMasterSlaveDict: An IterableUserDict subclass designed for use in Syzygy Master/slave apps.
  Provides an easy method for synchronizing its contents between master and slaves. Also
  supports easy user interaction. Most of its methods are intended for use only in the
  onPreExchange() method in the master copy of the application; in the slaves, only
  unpackState() should be called in onPostExchange() and draw() in onDraw().
  It supports most of the usual methods for insertion, deletion, and extraction, including
  d[key], d[key] = value, del d[key], and d.clear(). Note that keys MUST be Ints, Floats,
  or Strings. It also has a delValue() method for deleting objects by reference. It does
  not support d.copy() or d.popitem(). It supports iteration across either keys, values,
  or items (key/value tuples).
  Additional docs are available for:
  __init__()
  start()
  packState()
  unpackState()
  delValue()
  push()
  draw()
  processInteraction()
  """

  CONSTRUCT_MESSAGE = 0
  DESTRUCT_MESSAGE = 1
  STATE_MESSAGE = 2
  # DUMMY_MESSAGE to work around a bug in framework.setSequence()--it crashes with empty sequences
  DUMMY_MESSAGE = 3
  keyTypes = [type(''),type(0),type(0.)]
  def __init__( self, name, classData ):
    """
    d = arMasterSlaveDict( name, classData )
    'name' should be a string. This is used by the master/slave framework sequence data transfer
    methods, e.g. framework.initSequenceTransfer (called in the this class' start() method).

    'classData' establishes a mapping between a string, a class, and a class factory. It is
    used for two functions: First, whenever you try to insert an object into the arMasterSlaveDict,
    it is checked against the set of classes that you have provided; if not present, TypeError
    is raised. Second, if you insert an object into the dictionary in the master instance of
    your application, it is used to construct an instance of the same class in the slaves.
    You have to provide an entry for each class of object that you intent to insert
    in this container.  classData can be either a list or a tuple. Each item must be a tuple
    containing either 2 or 3 items. The first item must be a string; typically, it should be
    the name of the class in quotes. The second item should be an unambiguous reference to the
    class object (see below for examples). The third, optional item should be a reference to a
    callable object that takes no parameters and returns an object of the specified class
    (a 'class factory'). If the class factory item is not presented, then the class itself will
    be used as the factory; it will be called without arguments (again, see below), so the class
    must provide a zero-argument constructor. Take three examples:

    1) You've defined class Foo, either in the current file or in a module Bar that youve imported
    using 'from Bar import *'. Now  the class object Foo is in the global namespace. So you could call
    self.dict = arMasterSlaveDict( 'mydict', [('Foo',Foo)] ).
    In this case, inserted objects are type-checked against the type of Foo and the class itself is
    used as the factory, i.e. Foo() is called to generate new instances in the slaves.

    2) You've defined class Foo in module Bar and imported it using 'import Bar'. Now Foo is not in the
    global namespace, it's in the Bar namespace. So you would call
    self.dict = arMasterSlaveDict( 'mydict', [('Foo',Bar.Foo)] ).
    Now, objects you insert are type-checked agains Bar.Foo and Bar.Foo() is called to create new
    instances in the slaves.

    3) You want to pass a parameter to the constructor of Foo, e.g. a reference to the framework object.
    So you define a framework method newFoo():

    def newFoo(self): return Foo(self)

    and call self.dict = arMasterSlaveDict( 'mydict', [('Foo',Foo,self.newFoo)] ).
    Now inserted objects are type-checked against Foo and framework.newFoo() is called to generate
    new instances in slaves.
    """
    UserDict.IterableUserDict.__init__(self)
    import types
    self._name = name
    self._classFactoryDict = {}
    self._classDict = {}
    if type(classData) != types.TupleType and type(classData) != types.ListType:
      raise TypeError, 'arMasterSlaveDict() error: classData parameter must be a list or tuple.'
    for item in classData:
      if type(item) != types.TupleType:
        raise TypeError, 'arMasterSlaveDict() error: each item of classData must be a tuple.'
      if len(item) < 2 or len(item) > 3:
        raise TypeError, 'arMasterSlaveDict() error: each item of classData must contain 2 or 3 elements.'
      className = item[0]
      classRef = item[1]
      if len(item) == 3:
        classFactory = item[2]
      else:
        classFactory = classRef
      if type(className) != types.StringType:
        raise TypeError, 'arMasterSlaveDict(): each item of classData must be of the form '+ \
            '( classNameString, class [, class factory] ) (classNameString error)'
      if type(classRef) != types.ClassType and type(classRef) != types.TypeType:
        raise TypeError, 'arMasterSlaveDict(): each item of classData must be of the form '+ \
            '( classNameString, class [, class factory] ) (classRef error)'
      if not callable( classFactory ):
        raise TypeError, 'arMasterSlaveDict(): each item of classData must be of the form '+ \
            '( classNameString, class [, class factory] ) (class factory error)'
      self._classFactoryDict[className] = classFactory
      self._classDict[classRef] = className
    self._messages = [(arMasterSlaveDict.DUMMY_MESSAGE,0.,'foo')]
    self.pushKey = 0
  def start( self, framework ):  # call in framework onStart() method or start callback
    """ d.start( framework ).
    Should be called in your framework's onStart() (start callback)."""
    framework.initSequenceTransfer( self._name )
  def packState( self, framework ):
    """ d.packState( framework ).
    Should be called in your framework's onPreExchange(). It generates a sequence of messages
    starting with construction and deletion messages to signal insertion or deletion of objects
    (note that when you replace an item with one of a different type, that generates both a deletion
    and a construction message). Then it iterates through the dictionary's contents, calls each
    item's getState() method (which should return a sequence type containing only Ints, Floats, Strings,
    and nested sequences) and adding a state-change message with each returned tuple to its message
    queue. You can instead have your class' getState() method return None if an object's state has not changed,
    in which case no message is queued for that object; you might want to do this if e.g. you had a
    gazillion objects, only a few of which were changing state on each frame.
    Needless to say, all classes must provide a getState(). Finally, it calls
    framework.setSequence() to hand its message queue to the framework.
    """
    if len(self.data) > 0:
      for key in self.data:
        item = self.data[key]
        itemState = item.getState()
        if itemState:
          self._messages.append( (arMasterSlaveDict.STATE_MESSAGE, (key, item.getState())) )
    framework.setSequence( self._name, self._messages )
    self._messages = [(arMasterSlaveDict.DUMMY_MESSAGE,0.,'foo')]
  def unpackState( self, framework ):
    """ d.unpackState( framework ).
    Should be called in your framework's onPostExchange(), optionally only in slaves. Calls
    framework.getSequence to get the message queue from the master (see doc string for packState()),
    applies each message to either construct a new object (using the factories specified in __init__()),
    delete an object, or set its state using the object's setState() method. This method should expect a tuple
    with the same structure as that returned by getState(). For example, if your class' getState() method
    returned a 3-element numarray array, then setState() should expect a 3-element tuple.
    """
    messages = framework.getSequence( self._name )
    for message in messages:
      messageType = message[0]
      messageParam = message[1]
      if messageType == arMasterSlaveDict.STATE_MESSAGE:
        key = messageParam[0]
        newState = messageParam[1]
        self.data[key].setState( newState )
      elif messageType == arMasterSlaveDict.CONSTRUCT_MESSAGE:
        key = messageParam[0]
        className = messageParam[1]
        classFactory = self._classFactoryDict[className]
        self.data[key] = classFactory()
      elif messageType == arMasterSlaveDict.DESTRUCT_MESSAGE:
        key = messageParam
        del self.data[key]
    # If someones been adding and deleting stuff on slaves, we need to keep this
    # from growing uncontrollably...
    self._messages = [(arMasterSlaveDict.DUMMY_MESSAGE,0.,'foo')]
  def __setitem__(self, key, newItem):
    if not type(key) in arMasterSlaveDict.keyTypes:
      raise KeyError, 'arMasterSlaveDict keys must be Ints, Floats, or Strings.'
    className = None
    newType = type(newItem)
    for item in self._classDict.iteritems():
      if item[0] == newType:
        className = item[1]
        break
    if not className:
      raise TypeError, 'arMasterSlaveDict error: non-registered type '+str(newType)+' in __setitem__().'
    if self.has_key(key):
      if type(self.data[key]) == newType:
        self.data[key] = newItem
        return
      else:
        del self[key]
    self.data[key] = newItem
    self._messages.append( (arMasterSlaveDict.CONSTRUCT_MESSAGE, (key, className))  )
  def __delitem__( self, key ):
    self._messages.append( (arMasterSlaveDict.DESTRUCT_MESSAGE, key) )
    del self.data[key]
  def delValue( self, value ):
    """ d.delValue( contained object reference ).
    Delete an item from the dictionary by reference, i.e. if you have an external reference to
    an item in the dictionary, call delValue() with that reference to delete it."""
    for item in self.iteritems():
      if item[1] is value:
        del self[item[0]]
        return
    raise LookupError, 'arMasterSlaveDict.delValue() error: item not found.'
  def push( self, object ):
    """ d.push( object ).
    arMasterSlaveDict maintains an internal integer key for use with this method. Each time you call it,
    it checks to see whether that key already exists. If so, it increments it in a loop until it finds an
    unused key. Then it inserts the object using the key and increments it again. """
    while self.data.has_key( self.pushKey ):
      self.pushKey += 1
    self[self.pushKey] = object
    self.pushKey += 1
  def clear(self):
    for key in self.data.keys():
      self._messages.append( (arMasterSlaveDict.DESTRUCT_MESSAGE, key) )
    self.data.clear()
  def copy(self):
    raise AttributeError, 'arMasterSlaveDict does not allow copying of itself.'
  def popitem(self):
    raise AttributeError, 'arMasterSlaveDict does not support popitem().'
  def draw( self, framework=None ):
    """ d.draw( framework=None ).
    Loops through the dictionary, checks each item for an attribute named 'draw'. If
    an object has this attribute, it calls it, passing 'framework' as an argument if it is not None.  """
    for item in self.data.itervalues():
      if not hasattr( item, 'draw' ):
        continue
      drawMethod = getattr( item, 'draw' )
      if framework:
        item.draw( framework )
      else:
        item.draw()
  def processInteraction( self, effector ):
    """ d.processInteraction( effector ).
    Handles interaction between an arEffector and the objects in the dictionary. This is
    too complex a subject to describe here, see the Syzygy documentation on user interaction.
    Note that not all objects in the container have to be interactable, any objects that
    aren't instances of sub-classes of arPyInteractable are skipped.
    """
    # Interact with the grabbed object, if any.
    if effector.hasGrabbedObject():
      # get the grabbed object.
      grabbedPtr = effector.getGrabbedObject()
      # If this effector has grabbed an object not in this list, dont
      # interact with any of this list
      grabbedObject = None
      for item in self.data.itervalues():
        if isinstance( item, arPyInteractable ):
          # HACK! Found object in list
          if ar_getAddressFromSwigPtr(item.this) == ar_getAddressFromSwigPtr(grabbedPtr.this): 
            grabbedObject = item
            break
      if not grabbedObject:
        return True # not an error, just means no interaction occurred
      if grabbedObject.enabled():
        # If it's grabbed an object in this set, interact only with that object.
        return grabbedObject.processInteraction( effector )
    # Figure out the closest interactable to the effector (as determined
    # by their matrices). Go ahead and touch it (while untouching the
    # previously touched object if such are different).
    minDist = 1.e10    # A really big number, haven't found how to get
                       # the max in python yet
    touchedObject = None
    for item in self.data.itervalues():
      if isinstance( item, arPyInteractable ):
        if item.enabled():
          dist = effector.calcDistance( item.getMatrix() )
          if (dist >= 0.) and (dist < minDist):
            minDist = dist
            touchedObject = item
    if effector.hasTouchedObject():
      touchedPtr = effector.getTouchedObject()
      if not touchedObject or ar_getAddressFromSwigPtr(touchedObject.this) != ar_getAddressFromSwigPtr(touchedPtr.this):
        # ar_interactableUntouch( touchedPtr, effector )
        touchedPtr.untouch( effector )
    if not touchedObject:
      # Not touching any objects.
      return True
    # Finally, and most importantly, process the action of the effector on
    # the interactable.
    return touchedObject.processInteraction( effector )


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
