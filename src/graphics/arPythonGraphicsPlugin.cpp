//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsPlugin.h"
#include "arSTLalgo.h"
#include "Python.h"

#include <map>

// Definition of our drawable object.
class arPythonGraphicsPlugin: public arGraphicsPlugin {
  public:
    //  Object must provide a default (0-argument) constructor.
    arPythonGraphicsPlugin();
    virtual ~arPythonGraphicsPlugin();

    // Draw the object. Currently, the object is responsible for restoring any
    // OpenGL state that it messes with.
    virtual void draw( arGraphicsWindow& win, arViewport& view );

    // Update the object's state based on changes to the database made
    // by the controller program.
    virtual bool setState( std::vector<int>& intData,
                           std::vector<long>& longData,
                           std::vector<float>& floatData,
                           std::vector<double>& doubleData,
                           std::vector< std::string >& stringData );
  private:
    static std::map< std::string, PyObject* > __importedModules;
    static PyObject* __getModule( const std::string& moduleName, bool reload );
    // object-specific data.
    bool _makeObject( bool reloadModule );
    void _deleteObject();
    bool _callPythonSetState( std::vector<int>& intData,
                                       std::vector<long>& longData,
                                       std::vector<float>& floatData,
                                       std::vector<double>& doubleData,
                                       std::vector< std::string >& stringData );
    PyObject* _object;
    PyInterpreterState* _interpreterState;
    std::string _moduleName;
    std::string _factoryName;
    bool _triedToMake;
};

// These are the only two functions that the plugin exposes.
// The plugin interface (in arGraphicsPluginNode.cpp) calls the
// baseType() function to determine that yes, this shared library
// is in fact a graphics plugin. It then calls the factory() function
// to get an instance of the object that this plugin defines, and
// any further calls are made to methods of that instance.

extern "C" {
  SZG_CALL void baseType(char* buffer, int size) {
    ar_stringToBuffer("arGraphicsPlugin", buffer, size);
  }
  SZG_CALL void* factory() {
    return (void*) new arPythonGraphicsPlugin();
  }
}

arPythonGraphicsPlugin::arPythonGraphicsPlugin() :
  _object(NULL),
  _interpreterState(NULL),
  _moduleName(""),
  _factoryName(""),
  _triedToMake(false)
{}

arPythonGraphicsPlugin::~arPythonGraphicsPlugin() {
  _deleteObject();
}

void arPythonGraphicsPlugin::draw( arGraphicsWindow& win, arViewport& view ) {
  ar_log_debug() << "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd.\n";
  ar_log_debug() << "draw().\n";
  if (!_object) {
    ar_log_debug() << "draw() NULL object.\n";
    ar_log_debug() << "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd.\n";
    return;
  }
  static PyThreadState* threadState = NULL;
  if (!threadState) {
    _interpreterState = PyInterpreterState_New();
    threadState = PyThreadState_New( _interpreterState );
//    threadState = PyEval_SaveThread();
  }
  ar_log_debug() << "Thread state ==" << threadState << ar_endl;
  PyEval_AcquireThread( threadState );
//  PyGILState_STATE pState = PyGILState_Ensure();
  ar_log_debug() << "draw() has GIL.\n";

  PyObject* result;
  PyObject* pArgs = NULL;
  PyObject* pDraw = PyObject_GetAttrString( _object, "draw" );
  if (!pDraw) {
    ar_log_error() << "arPythonGraphicsPlugin object from " << _moduleName << "." << _factoryName
                   << "() has no draw attribute.\n";
    goto LFail;
  }
  if (!PyCallable_Check( pDraw )) {
    ar_log_error() << "arPythonGraphicsPlugin object from " << _moduleName << "." << _factoryName
                   << "() has uncallable draw attribute.\n";
    goto LFail;
  }
  pArgs = PyTuple_New( 0 );
  if (!pArgs) {
    ar_log_error() << "arPythonGraphicsPlugin failed to allocate argument tuple.\n";
    goto LFailPy;
  }
//  PyTuple_SET_ITEM( pArgs, 0, pIntArgs );
//  PyTuple_SET_ITEM( pArgs, 1, pFloatArgs );
  result = PyEval_CallObject( pDraw, pArgs );
  if (!result) {
    ar_log_error() << "arGraphicsPlugin draw() failed for object of type "
                   << _moduleName << "." << _factoryName << ar_endl;
LFailPy:
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
LFail:
    Py_XDECREF( pArgs );
    Py_XDECREF( pDraw );
    _deleteObject();
    ar_log_debug() << "draw() releasing GIL.\n";
    PyEval_ReleaseThread( threadState );
//    PyGILState_Release( pState );
    ar_log_debug() << "draw() released GIL.\n";
    return;
  }

  Py_DECREF( pArgs );
  Py_DECREF( pDraw );
  ar_log_debug() << "draw() releasing GIL.\n";
  PyEval_ReleaseThread( threadState );
//  PyGILState_Release( pState );
  ar_log_debug() << "draw() released GIL.\n";
}

bool arPythonGraphicsPlugin::setState( std::vector<int>& intData,
                                       std::vector<long>& longData,
                                       std::vector<float>& floatData,
                                       std::vector<double>& doubleData,
                                       std::vector< std::string >& stringData ) {
  static PyThreadState* threadState = NULL;
  ar_log_debug() << "setState().\n";
  if (!Py_IsInitialized()) {
    ar_log_debug() << "About to Py_Initialize()\n";
    Py_Initialize();
    PyEval_InitThreads();
    threadState = PyEval_SaveThread();
//    _interpreterState = PyInterpreterState_New();
//    threadState = PyThreadState_New( _interpreterState );
    ar_log_debug() << "Py_Initialize() done\n";
  }
  if (!threadState) {
    ar_log_error() << "NULL thread state after Python initialized.\n";
    return false;
  }

  PyEval_AcquireThread( threadState );
  ar_log_debug() << "setState has GIL.\n";
  ar_log_debug() << "Python state: Py_IsInitialized==" << Py_IsInitialized() << ", PyEval_ThreadsInitialized=="
                 << PyEval_ThreadsInitialized() << ar_endl;
  if ((intData.size() < 1)||(stringData.size() < 2)) {
    ar_log_error() << "arPythonGraphicsPlugin requires an int (reload flag) and two strings "
                   << "(module and object factory names).\n"
                   << "Got " << intData.size() << " ints and " << stringData.size()
                   << " strings.\n";
    return true; // Really? Why not false?
  }
  bool reloadModule = intData.back();
  intData.pop_back();
  std::string factoryName = stringData.back();
  stringData.pop_back();
  std::string moduleName = stringData.back();
  stringData.pop_back();
  ar_log_debug() << "Module: " << moduleName << ", Factory: " << factoryName
                 << ", reload=" << reloadModule << ar_endl;

//  PyGILState_STATE pState = PyGILState_Ensure();
//  ar_log_debug() << "setState has GIL.\n";
  if ((factoryName != _factoryName) || (moduleName != _moduleName) || reloadModule) {
    _deleteObject();
    _triedToMake = false;
  }
  if (!_object) {
    if (_triedToMake) {
      goto finish;
    }
    _moduleName = moduleName;
    _factoryName = factoryName;
    if (!_makeObject( reloadModule )) {
      _deleteObject();
      goto finish;
    }
  }
  if (!_callPythonSetState( intData, longData, floatData, doubleData, stringData )) {
    ar_log_error() << "arPythonGraphicsPlugin failed to setState() for object from "
                   << _moduleName << "." << _factoryName << ar_endl;
    _deleteObject();
    goto finish;
  }
finish:
  ar_log_debug() << "setState releasing GIL.\n";
  PyEval_ReleaseThread( threadState );
//  PyGILState_Release( pState );
  ar_log_debug() << "setState released GIL.\n";
  if (!PyThreadState_GetDict()) {
    ar_log_debug() << "NULL thread state dict.\n";
  } else {
    ar_log_debug() << "Non-NULL thread state dict.\n";
  }
  ar_log_debug() << "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss.\n";
  return true;
}

bool arPythonGraphicsPlugin::_callPythonSetState( std::vector<int>& intData,
                                       std::vector<long>& longData,
                                       std::vector<float>& floatData,
                                       std::vector<double>& doubleData,
                                       std::vector< std::string >& stringData ) {
  ar_log_debug() << "_callPythonSetState().\n";
  int count(0);
  PyObject* result;
  PyObject* pArgs = NULL;
  PyObject* pIntArgs = NULL;
  PyObject* pFloatArgs = NULL;
  PyObject* pSetState = PyObject_GetAttrString( _object, "setState" );
  // Loose copypaste from above (see LFail).
  if (!pSetState) {
    ar_log_error() << "arPythonGraphicsPlugin object from " << _moduleName << "." << _factoryName
                   << "() does not have a setState attribute.\n";
    goto LFail;
  }
  if (!PyCallable_Check( pSetState )) {
    ar_log_error() << "arPythonGraphicsPlugin object from " << _moduleName << "." << _factoryName
                   << "() setState attribute is not callable.\n";
    goto LFail;
  }
  pArgs = PyTuple_New( 2 );
  if (!pArgs) {
    ar_log_error() << "arPythonGraphicsPlugin failed to allocate argument tuple.\n";
    goto LFailPy;
  }
  pIntArgs = PyTuple_New( intData.size()+longData.size() );
  if (!pIntArgs) {
    ar_log_error() << "arPythonGraphicsPlugin failed to allocate int tuple.\n";
    goto LFailPy;
  }
  PyTuple_SET_ITEM( pArgs, 0, pIntArgs );
  pFloatArgs = PyTuple_New( floatData.size()+doubleData.size() );
  if (!pFloatArgs) {
    ar_log_error() << "arPythonGraphicsPlugin failed to allocate float tuple.\n";
    goto LFailPy;
  }
  PyTuple_SET_ITEM( pArgs, 1, pFloatArgs );
  count = 0;
  std::vector<int>::iterator intIter;
  for (intIter = intData.begin(); intIter != intData.end(); ++intIter) {
    PyTuple_SET_ITEM( pIntArgs, count++, PyInt_FromLong((long)*intIter) );
  }
  std::vector<long>::iterator longIter;
  for (longIter = longData.begin(); longIter != longData.end(); ++longIter) {
    PyTuple_SET_ITEM( pIntArgs, count++, PyInt_FromLong(*longIter) );
  }
  count = 0;
  std::vector<float>::iterator floatIter;
  for (floatIter = floatData.begin(); floatIter != floatData.end(); ++floatIter) {
    PyTuple_SET_ITEM( pFloatArgs, count++, PyFloat_FromDouble((double)*floatIter) );
  }
  std::vector<double>::iterator doubleIter;
  for (doubleIter = doubleData.begin(); doubleIter != doubleData.end(); ++doubleIter) {
    PyTuple_SET_ITEM( pFloatArgs, count++, PyFloat_FromDouble(*doubleIter) );
  }
  result = PyEval_CallObject( pSetState, pArgs );
  if (!result) {
    ar_log_error() << "arGraphicsPlugin setState() failed for object of type "
                   << _moduleName << "." << _factoryName << ar_endl;
LFailPy:
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
LFail:
    Py_XDECREF( pArgs );
    Py_XDECREF( pSetState );
    _deleteObject();
    return false;
  }

  Py_DECREF( pArgs );
  Py_DECREF( pSetState );
  return true;
}

bool arPythonGraphicsPlugin::_makeObject( bool reloadModule ) {
  ar_log_debug() << "_makeObject()\n";
  _triedToMake = true;
  _deleteObject();
  PyObject* pModule = arPythonGraphicsPlugin::__getModule( _moduleName, reloadModule );
  if (!pModule) {
    return false;
  }

  ar_log_debug() << "__getModule() succeeded\n";
  // Get a borrowed ref. to the module's dictionary.
  PyObject* pDict = PyModule_GetDict( pModule );
  // Get a borrowed ref to factory attribute of module.
  PyObject* pFunc = PyDict_GetItemString( pDict, _factoryName.c_str() );
  if (!pFunc) {
    ar_log_error() << "arPythonGraphicsPlugin couldn't get object factory " << _moduleName
                   << "." << _factoryName << ar_endl;
    goto abort;
  }

  if (!PyCallable_Check( pFunc )) {
    ar_log_error() << "uncallable arPythonGraphicsPlugin " << _moduleName << "." << _factoryName
                   << ".\n";
    return false;
  }

  PyObject* pArgs = PyTuple_New(0);
  if (!pArgs) {
    ar_log_error() << "arPythonGraphicsPlugin couldn't create empty argument tuple.\n";
    goto abort;
  }

  PyObject* pObject = PyObject_CallObject(pFunc, pArgs);
  Py_DECREF(pArgs);
  if (!pObject) {
    ar_log_error() << "arPythonGraphicsPlugin call to " << _moduleName << "." << _factoryName
                   << "() failed.\n";
abort:
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    return false;
  }

  _object = pObject;
  return true;
}

void arPythonGraphicsPlugin::_deleteObject() {
  ar_log_debug() << "_deleteObject()\n";
  if (!_object) {
    ar_log_debug() << "_deleteObject() object == NULL.\n";
    return;
  }
  Py_DECREF(_object);
  _object = NULL;
}

std::map< std::string, PyObject* > arPythonGraphicsPlugin::__importedModules;
PyObject* arPythonGraphicsPlugin::__getModule( const std::string& moduleName, bool reload ) {
  ar_log_debug() << "__getModule()\n";
  std::map< std::string, PyObject* >::iterator iter;
  PyObject* pModule;
  iter = arPythonGraphicsPlugin::__importedModules.find( moduleName );
  if (iter != arPythonGraphicsPlugin::__importedModules.end() && !reload) {
    // We found it and user hasn't specified a reload.
    return iter->second;
  }

  // We didn't find it.
  if (iter == arPythonGraphicsPlugin::__importedModules.end()) {
    PyObject* pName = PyString_FromString( moduleName.c_str() );
    if (!pName) {
      ar_log_error() << "arPythonGraphicsPlugin failed to generate python string from module name "
                     << moduleName << ar_endl;
      return NULL;
    }

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (!pModule) {
      ar_log_error() << "arPythonGraphicsPlugin failed to import module " << moduleName << ar_endl;
      goto abort;
    }
    arPythonGraphicsPlugin::__importedModules[moduleName] = pModule;
    return pModule;
  }

  // We found it, but user specified that we should reload it...
  pModule = PyImport_ReloadModule( iter->second );
  Py_DECREF( iter->second );
  // Insert it in map even if NULL.
  arPythonGraphicsPlugin::__importedModules[moduleName] = pModule;
  if (!pModule) {
    ar_log_error() << "arPythonGraphicsPlugin failed to reload module " << moduleName << ar_endl;
abort:
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    return NULL;
  }

  return pModule;
}
