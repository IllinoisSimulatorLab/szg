%{

#include "arExperiment.h"

class arPythonExperiment;

class arPythonTrialGenerator : public arTrialGenerator {
  public:
    arPythonTrialGenerator( const std::string& comment = "" );
    virtual ~arPythonTrialGenerator();
    bool init( const std::string experiment,
                     std::string configPath,
                     const arHumanSubject& subject,
                     arExperimentDataRecord& factors,
                     const arStringSetMap_t& legalStringValues,
                     arSZGClient& SZGClient ) {
      return true;
    }
 
    bool newTrial( arExperimentDataRecord& factors );
    void setCallback( PyObject* newTrialCallback );
  private:
    PyObject* _newTrialCallback;
};

arPythonTrialGenerator::arPythonTrialGenerator( const std::string& comment ) :
  arTrialGenerator(),
  _newTrialCallback(NULL) {
  _comment = comment;
}

arPythonTrialGenerator::~arPythonTrialGenerator() {
  Py_XDECREF(_newTrialCallback);
}

void arPythonTrialGenerator::setCallback( PyObject* newTrialCallback ) {
  if (!PyCallable_Check(newTrialCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonTrialGenerator error: newTrialCallback not callable");
    return;
  }

  Py_XDECREF(_newTrialCallback);
  Py_XINCREF(newTrialCallback);
  _newTrialCallback = newTrialCallback;
}

bool arPythonTrialGenerator::newTrial( arExperimentDataRecord& factors ) {
  if (_newTrialCallback == NULL) {
    PyErr_SetString(PyExc_ValueError,"A NULL-pointer exception occurred in the newTrial callback.");
    return false;
  }
  PyObject *facobj = SWIG_NewPointerObj((void *) &factors,
                           SWIGTYPE_p_arExperimentDataRecord, 0); 
  PyObject *arglist=Py_BuildValue("(O)",facobj);
  PyObject *result=PyEval_CallObject(_newTrialCallback, arglist);  
  if (result==NULL) { 
    if (PyErr_Occurred()) {
      PyErr_Print(); 
    }
    PyErr_SetString(PyExc_RuntimeError,"A Python exception occurred in the newTrial callback.");
    Py_DECREF(arglist); 
    Py_DECREF(facobj); 
    return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_DECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(facobj); 
  return res; 
}

/*
class arPythonDataSaver : public arDataSaver {
  public:
    arPythonDataSaver( const std::string& fileSuffix="" );
    virtual ~arPythonDataSaver();
    virtual bool init( const std::string experimentName,
                       std::string dataPath, std::string comment,
                       const arHumanSubject& subjectData,
                       arExperimentDataRecord& factors,
                       arExperimentDataRecord& dataRecords,
                       arSZGClient& SZGClient);
    virtual bool saveData( arExperimentDataRecord& factors, 
                           arExperimentDataRecord& dataRecords );
    void setCallbacks( PyObject* writeHeaderCallback, PyObject* saveTrialCallback );
  private:
    PyObject* _writeHeaderCallback;
    PyObject* _saveDataCallback;
    bool _configured;
    std::string _dataPath;
    std::string _dataFilename;
};

arPythonDataSaver::arPythonDataSaver( const std::string& fileSuffix ) :
  arDataSaver( fileSuffix ),
  _writeHeaderCallback(NULL),
  _saveDataCallback(NULL),
  _configured( false ) {}
}

arPythonDataSaver::~arPythonDataSaver() {
  Py_XDECREF(_writeHeaderCallback);
  Py_XDECREF(_saveDataCallback);
}

void arPythonDataSaver::setCallbacks( PyObject* writeHeaderCallback, PyObject* saveTrialCallback ) {
  if (!PyCallable_Check(writeHeaderCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonTrialGenerator error: writeHeaderCallback not callable");
    return;
  }
  Py_XDECREF(_writeHeaderCallback);
  Py_XINCREF(writeHeaderCallback);
  _writeHeaderCallback = writeHeaderCallback;

  if (!PyCallable_Check(saveTrialCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonTrialGenerator error: saveTrialCallback not callable");
    return;
  }
  Py_XDECREF(_saveTrialCallback);
  Py_XINCREF(saveTrialCallback);
  _saveTrialCallback = saveTrialCallback;
}


bool arPythonDataSaver::init( const string experimentName,
                           string dataPath, string comment,
                           const arHumanSubject& subjectData,
                           arExperimentDataRecord& factors,
                           arExperimentDataRecord& dataRecords,
                           arSZGClient& SZGClient) {
  if (_writeHeaderCallback == NULL) {
    cerr << "arPythonDataSaver error: no writeHeaderCallback.\n";
    errmsg="A NULL-pointer exception occurred in the write-header callback.";
    cerr << errmsg << "\n";
    throw  errmsg; 
    return false;
  }
  if (_saveTrialCallback == NULL) {
    cerr << "arPythonDataSaver error: no saveTrialCallback.\n";
    errmsg="A NULL-pointer exception occurred in the save-trial callback.";
    cerr << errmsg << "\n";
    throw  errmsg; 
    return false;
  }
  stringstream& errStream = szgClient.startResponse();
  if (!setFilePath( dataPath, SZGClient ) {
    cerr << "arPythonDataSaver error: setFilePath() failed.\n";
    errStream << "arPythonDataSaver error: setFilePath() failed.\n";
    return false;
  }
} */

class arPythonExperimentTrialPhase : public arExperimentTrialPhase {
  public:
    arPythonExperimentTrialPhase( const std::string& sname="" ) : 
      arExperimentTrialPhase(sname),
      _initCallback(NULL),
      _updateCallback(NULL),
      _updateEventCallback(NULL) {}
    virtual ~arPythonExperimentTrialPhase();
    virtual bool init( arSZGAppFramework*, arExperiment* );
    virtual bool update( arSZGAppFramework*, arExperiment* );
    virtual bool update( arSZGAppFramework*, arExperiment*, arIOFilter*, arInputEvent& );
    void setCallbacks( PyObject* initCallback,
                       PyObject* updateCallback,
                       PyObject* updateEventCallback );
  private:
    PyObject* _initCallback;
    PyObject* _updateCallback;
    PyObject* _updateEventCallback;
};

arPythonExperimentTrialPhase::~arPythonExperimentTrialPhase() {
  Py_XDECREF(_initCallback);
  Py_XDECREF(_updateCallback);
  Py_XDECREF(_updateEventCallback);
}

void arPythonExperimentTrialPhase::setCallbacks( PyObject* initCallback,
                                                 PyObject* updateCallback,
                                                 PyObject* updateEventCallback ) {
  if (!PyCallable_Check(initCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonExperimentTrialPhase error: initCallback not callable");
    return;
  }
  if (!PyCallable_Check(updateCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonExperimentTrialPhase error: updateCallback not callable");
    return;
  }
  if (!PyCallable_Check(updateEventCallback)) {
    PyErr_SetString(PyExc_TypeError, "arPythonExperimentTrialPhase error: updateEventCallback not callable");
    return;
  }

  Py_XDECREF(_initCallback);
  Py_XINCREF(initCallback);
  _initCallback = initCallback;

  Py_XDECREF(_updateCallback);
  Py_XINCREF(updateCallback);
  _updateCallback = updateCallback;

  Py_XDECREF(_updateEventCallback);
  Py_XINCREF(updateEventCallback);
  _updateEventCallback = updateEventCallback;
}

bool arPythonExperimentTrialPhase::init( arSZGAppFramework* fw, arExperiment* expt ) {
  string errmsg;
  if (_initCallback == NULL) {
    cerr << "arPythonExperimentTrialPhase " << getName() << " error: no init callback.\n";
    errmsg="A NULL-pointer exception occurred in the TrialPhaseInit callback.";
    cerr << errmsg << "\n";
    throw  errmsg; 
    return false;
  }
  PyObject *fwobj = SWIG_NewPointerObj((void *) fw,
                           SWIGTYPE_p_arSZGAppFramework, 0); 
  PyObject *exptobj = SWIG_NewPointerObj((void *) expt,
                           SWIGTYPE_p_arPythonExperiment, 0); 
  PyObject *arglist=Py_BuildValue("(O,O)",fwobj,exptobj); 
  PyObject *result=PyEval_CallObject(_initCallback, arglist);  
  if (result==NULL) { 
    if (PyErr_Occurred()) {
      PyErr_Print(); 
    }
    PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the TrialPhaseInit callback." );
    Py_DECREF(arglist); 
    Py_DECREF(exptobj); 
    Py_DECREF(fwobj); 
    return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_DECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(exptobj); 
  Py_DECREF(fwobj); 
  return res; 
}

bool arPythonExperimentTrialPhase::update( arSZGAppFramework* fw, arExperiment* expt ) {
  string errmsg;
  if (_updateCallback == NULL) {
    cerr << "arPythonExperimentTrialPhase " << getName() << " error: no update callback.\n";
    errmsg="A NULL-pointer exception occurred in the TrialPhaseUpdate callback.";
    cerr << errmsg << "\n";
    throw  errmsg; 
    return false;
  }
  PyObject *fwobj = SWIG_NewPointerObj((void *) fw,
                           SWIGTYPE_p_arSZGAppFramework, 0); 
  PyObject *exptobj = SWIG_NewPointerObj((void *) expt,
                           SWIGTYPE_p_arPythonExperiment, 0); 
  PyObject *arglist=Py_BuildValue("(O,O)",fwobj,exptobj); 
  PyObject *result=PyEval_CallObject(_updateCallback, arglist);  
  if (result==NULL) { 
    if (PyErr_Occurred()) {
      PyErr_Print(); 
    }
    PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the TrialPhaseUpdate callback." );
    Py_DECREF(arglist); 
    Py_DECREF(exptobj); 
    Py_DECREF(fwobj); 
    return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_DECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(exptobj); 
  Py_DECREF(fwobj); 
  return res; 
}

bool arPythonExperimentTrialPhase::update( arSZGAppFramework* fw, arExperiment* expt,
                                           arIOFilter* filt, arInputEvent& event ) {
  string errmsg;
  if (_updateEventCallback == NULL) {
    ostringstream os;
    os << "arPythonExperimentTrialPhase " << getName() << " error: no updateEvent callback.\n";
    std::string msg = os.str();
    PyErr_SetString( PyExc_RuntimeError, msg.c_str() );
    return false;
  }
  PyObject *fwobj = SWIG_NewPointerObj((void *) fw,
                           SWIGTYPE_p_arSZGAppFramework, 0); 
  PyObject *exptobj = SWIG_NewPointerObj((void *) expt,
                           SWIGTYPE_p_arPythonExperiment, 0); 
  PyObject *filtobj = SWIG_NewPointerObj((void *) filt,
                           SWIGTYPE_p_arPythonEventFilter, 0); 
  PyObject *eventobj = SWIG_NewPointerObj((void *) &event,
                           SWIGTYPE_p_arInputEvent, 0); 
  PyObject *arglist=Py_BuildValue("(O,O,O,O)",fwobj,exptobj,filtobj,eventobj); 
  PyObject *result=PyEval_CallObject(_updateEventCallback, arglist);  
  if (result==NULL) { 
    if (PyErr_Occurred()) {
      PyErr_Print(); 
    }
    PyErr_SetString( PyExc_RuntimeError, "A Python exception occurred in the TrialPhaseUpdateEvent callback." );
    Py_DECREF(arglist); 
    Py_DECREF(eventobj); 
    Py_DECREF(filtobj); 
    Py_DECREF(exptobj); 
    Py_DECREF(fwobj); 
    return false;
  }
  bool res=(bool) PyInt_AsLong(result); 
  Py_DECREF(result); 
  Py_DECREF(arglist); 
  Py_DECREF(eventobj); 
  Py_DECREF(filtobj); 
  Py_DECREF(exptobj); 
  Py_DECREF(fwobj); 
  return res; 
}

class arPythonExperiment: public arExperiment {
  public:    
    arPythonExperiment();
    virtual ~arPythonExperiment();
    
    /// Define an experimental parameter or 'factor'..
    /// Note that if address == 0, an internal buffer will
    /// be allocated
    bool addLongFactor( const std::string& sname );
    bool addDoubleFactor( const std::string& sname );
    bool addStringFactor( const std::string& sname );
    // Add a string factor that must belong to a set of values (passed in a list of strings) 
    bool addStringFactorSet( const std::string& sname, PyObject* defaultList );

    /// Get a factors value
    PyObject* getLongFactor( const std::string& sname );
    PyObject* getDoubleFactor( const std::string& sname );
    std::string getStringFactor( const std::string& sname );

    /// Define a data field (before init() only)
    bool addLongDataField( const std::string& sname );
    bool addDoubleDataField( const std::string& sname );
    bool addStringDataField( const std::string& sname );

    /// Set the value of a data field
    bool setLongData( const std::string& sname, PyObject* intDataList );
    bool setDoubleData( const std::string& sname, PyObject* floatDataList );
    bool setStringData( const std::string& sname, const std::string& stringData );
    
    /// Get the value of a data field
    PyObject* getLongData( const std::string& sname );
    PyObject* getDoubleData( const std::string& sname );
    std::string getStringData( const std::string& sname );

    /// Define a parameter whose value is to be read in from the human subject database.
    bool addLongSubjectParameter( const std::string& theName );
    bool addDoubleSubjectParameter( const std::string& theName );
    bool addStringSubjectParameter( const std::string& theName );

    /// Get the value of a named subject parameter for the current human subject.
    long getLongSubjectParameter( const std::string& theName );
    double getDoubleSubjectParameter( const std::string& theName );
    std::string getStringSubjectParameter( const std::string& theName );
    
    /// Set the trial-generation method. This would normally be read from
    /// SZG_EXPT/method, but if this has been called that parameter will be ignored.
    /// Must be called before init().
    //bool setTrialGenMethod( const std::string& method );
    //bool setTrialGenMethod( arTrialGenerator* methodPtr );
};

arPythonExperiment::arPythonExperiment() :
  arExperiment() {
  }

arPythonExperiment::~arPythonExperiment() {}

bool arPythonExperiment::addLongFactor( const  std::string& sname ) {
  return arExperiment::addFactor( sname, AR_LONG );
}

bool arPythonExperiment::addDoubleFactor( const  std::string& sname ) {
  return arExperiment::addFactor( sname, AR_DOUBLE );
}

bool arPythonExperiment::addStringFactor( const  std::string& sname ) {
  return arExperiment::addFactor( sname, AR_CHAR );
}

bool arPythonExperiment::addStringFactorSet( const  std::string& sname, PyObject* defaultList ) {
  if (!PyList_Check(defaultList)) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::addStringFactorSet() Expecting a list");
    return false;
  }
  int size = PyList_Size(defaultList);
  if (size == 0) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::addStringFactorSet() Expecting a NON_EMPTY list");
    return false;
  }
  int i;
  int totalStringLength = 1;
  for (i = 0; i < size; ++i) {
    // Note we do not own s...
    PyObject *s = PyList_GetItem(defaultList,i);
    if (!PyString_Check(s)) {
        PyErr_SetString(PyExc_ValueError, "arPythonExperiment::addStringFactorSet() list items must be strings");
        return false;
    }
    totalStringLength += PyString_Size(s)+1;
  }
  char* tmp = new char[totalStringLength+1];
  if (!tmp) {
    PyErr_SetString(PyExc_MemoryError, "arPythonExperiment::addStringFactorSet() memory allocation failed");
    return false;
  }
  char* inc = tmp;
  *inc++ = '|';
  for (i = 0; i < size; ++i) {
    PyObject *t = PyList_GetItem(defaultList,i);
    char* strptr = PyString_AsString(t);
    int n = PyString_Size(t);
    memcpy( inc, strptr, n );
    inc += n;
    *inc++ = '|';
  }
  *inc = '\0';
  bool result = arExperiment::addCharFactor( sname, tmp );
  delete[] tmp;
  return result;
}

PyObject* arPythonExperiment::getLongFactor( const std::string& sname ) {
  unsigned int size;
  const long* const tmp = (const long* const)arExperiment::getFactor( sname, AR_LONG, size );
  if (!tmp) {
    PyErr_SetString(PyExc_RuntimeError,"arPythonExperiment::getLongFactor() failed to get factor");
    return NULL;
  }
  PyObject* result = PyTuple_New((int)size);
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError, "arPythonExperiment::getLongFactor() PyTuple_New() failed");
    return NULL;
  }
  int i;
  for (i=0; i<size; ++i) {
    PyObject *s = PyInt_FromLong(tmp[i]);
    if (!s) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongFactor() PyInt_FromLong() failed");
      Py_DECREF( result );
      return NULL;
    }
    // PyTuple_SetItem() steals our reference to s, so we dont need to Py_DECREF() it (if successful).
    if (PyTuple_SetItem( result, i, s ) != 0) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongFactor() PyTuple_SetItem() failed");
      Py_DECREF( s );
      Py_DECREF( result );
      return NULL;
    }
  }
  return result;
}

PyObject* arPythonExperiment::getDoubleFactor( const std::string& sname ) {
  unsigned int size;
  const double* const tmp = (const double* const)arExperiment::getFactor( sname, AR_DOUBLE, size );
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getDoubleFactor() failed to get factor");
    return NULL;
  }
  PyObject* result = PyTuple_New((int)size);
  if (!result) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleFactor() PyTuple_New() failed");
    return NULL;
  }
  int i;
  for (i = 0; i < size; ++i) {
    PyObject *s = PyFloat_FromDouble(tmp[i]);
    if (!s) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleFactor() PyFloat_FromDouble() failed");
      Py_DECREF( result );
      return NULL;
    }
    if (PyTuple_SetItem( result, i, s ) != 0) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleFactor() PyTuple_SetItem() failed");
      Py_DECREF( s );
      Py_DECREF( result );
      return NULL;
    }
  }
  return result;
}

std::string arPythonExperiment::getStringFactor( const  std::string& sname ) {
  unsigned int size;
  const char* const tmp = (const char* const)arExperiment::getFactor( sname, AR_CHAR, size );
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getStringFactor() failed to get factor");
    return "NULL";
  }
  return std::string(tmp);
}

bool arPythonExperiment::addLongDataField( const  std::string& sname ) {
  return arExperiment::addDataField( sname, AR_LONG );
}

bool arPythonExperiment::addDoubleDataField( const  std::string& sname ) {
  return arExperiment::addDataField( sname, AR_DOUBLE );
}

bool arPythonExperiment::addStringDataField( const  std::string& sname ) {
  return arExperiment::addDataField( sname, AR_CHAR );
}

bool arPythonExperiment::setLongData( const std::string& sname, PyObject* intDataList ) {
  if (!PyList_Check(intDataList)) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setLongData() Expecting a list");
    return false;
  }
  int size = PyList_Size(intDataList);
  long* tmp = new long[size];
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setLongData() memory allocation failed");
    return false;
  }
  int i;
  for (i=0; i<size; ++i) {
    PyObject *s = PyList_GetItem(intDataList,i);
    if (!PyInt_Check(s)) {
        delete[] tmp;
        PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setLongData() list items must be ints");
        return false;
    }
    tmp[i] = PyInt_AsLong(s);
  }
  bool result = arExperiment::setDataFieldData( sname, AR_LONG, (const void* const)tmp, (unsigned int)size );
  delete[] tmp;
  return result;
}


bool arPythonExperiment::setDoubleData( const std::string& sname, PyObject* floatDataList ) {
  if (!PyList_Check(floatDataList)) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setDoubleData() Expecting a list");
    return false;
  }
  int size = PyList_Size(floatDataList);
  double* tmp = new double[size];
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setDoubleData() memory allocation failed");
    return false;
  }
  int i;
  for (i = 0; i < size; ++i) {
    PyObject *s = PyList_GetItem(floatDataList,i);
    if (!PyFloat_Check(s)) {
        delete[] tmp;
        PyErr_SetString(PyExc_ValueError, "arPythonExperiment::setDoubleData() list items must be floats");
        return false;
    }
    tmp[i] = PyFloat_AsDouble(s);
  }
  bool result = arExperiment::setDataFieldData( sname, AR_DOUBLE, (const void* const)tmp, (unsigned int)size );
  delete[] tmp;
  return result;
}


bool arPythonExperiment::setStringData( const std::string& sname, const std::string& stringData ) {
  return arExperiment::setDataFieldData( sname, AR_CHAR, (const void* const)stringData.c_str(), (unsigned int)stringData.size() );
}


PyObject* arPythonExperiment::getLongData( const std::string& sname ) {
  unsigned int size;
  const long* const tmp = (const long* const)arExperiment::getDataField( sname, AR_LONG, size );
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongData() getDataField() failed");
    return NULL;
  }
  PyObject* result = PyTuple_New((int)size);
  if (!result) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongData() PyTuple_New() failed");
    return NULL;
  }
  int i;
  for (i = 0; i < size; ++i) {
    PyObject *s = PyInt_FromLong(tmp[i]);
    if (!s) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongData() PyInt_FromLong() failed");
      Py_DECREF( result );
      return NULL;
    }
    if (PyTuple_SetItem( result, i, s ) != 0) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getLongData() PyTuple_SetItem() failed");
      Py_DECREF( s );
      Py_DECREF( result );
      return NULL;
    }
  }
  return result;
}

PyObject* arPythonExperiment::getDoubleData( const std::string& sname ) {
  unsigned int size;
  const double* const tmp = (const double* const)arExperiment::getDataField( sname, AR_DOUBLE, size );
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleData() getDataField() failed");
    return NULL;
  }
  PyObject* result = PyTuple_New((int)size);
  if (!result) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleData() PyTuple_New() failed");
    return NULL;
  }
  int i;
  for (i = 0; i < size; ++i) {
    PyObject *s = PyFloat_FromDouble(tmp[i]);
    if (!s) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleData() PyFloat_FromDouble() failed");
      Py_DECREF( result );
      return NULL;
    }
    if (PyTuple_SetItem( result, i, s ) != 0) {
      PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getDoubleData() PyTuple_SetItem() failed");
      Py_DECREF( s );
      Py_DECREF( result );
      return NULL;
    }
  }
  return result;
}

std::string arPythonExperiment::getStringData( const std::string& sname ) {
  unsigned int size;
  const char* const tmp = (const char* const)arExperiment::getDataField( sname, AR_CHAR, size );
  if (!tmp) {
    PyErr_SetString(PyExc_ValueError, "arPythonExperiment::getStringData() getDataField() failed");
    return NULL;
  }
  // What the heck, lets assume 0-termination (I think thats OK)
  return std::string(tmp);
}

bool arPythonExperiment::addLongSubjectParameter( const  std::string& sname ) {
  return arExperiment::addSubjectParameter( sname, AR_LONG );
}

bool arPythonExperiment::addDoubleSubjectParameter( const  std::string& sname ) {
  return arExperiment::addSubjectParameter( sname, AR_DOUBLE );
}

bool arPythonExperiment::addStringSubjectParameter( const  std::string& sname ) {
  return arExperiment::addSubjectParameter( sname, AR_CHAR );
}

long arPythonExperiment::getLongSubjectParameter( const  std::string& sname ) {
  long* tmp;
  if (!arExperiment::getSubjectParameter( sname, AR_LONG, (void*)tmp )) {
    PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getLongSubjectParameter() failed to get parameter");
    return -1;
  }
  return *tmp;
}

double arPythonExperiment::getDoubleSubjectParameter( const  std::string& sname ) {
  double tmp;
  // Allow one exception because this one is already universally defined to be a float
  if (sname == "eye_spacing_cm") {
    float ftmp;
    if (!arExperiment::getSubjectParameter( sname, AR_FLOAT, (void*)&ftmp )) {
      PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getDoubleSubjectParameter() failed to get parameter");
      return -1.;
    }
    return (double)ftmp;
  }
  if (!arExperiment::getSubjectParameter( sname, AR_DOUBLE, (void*)&tmp )) {
    PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getDoubleSubjectParameter() failed to get parameter");
    return -1.;
  }
  return tmp;
}

std::string arPythonExperiment::getStringSubjectParameter( const  std::string& theName ) {
  std::string value;
  if (!getCharSubjectParameter( theName, value )) {
    cerr << "arPythonExperiment::getStringSubjectParameter() failed to get parameter.\n";
    PyErr_SetString(PyExc_ValueError,"arPythonExperiment::getStringSubjectParameter() failed to get parameter");
    return "NULL";
  }
  return value;
}

%}

class arExperimentDataRecord  {
  public:
    arExperimentDataRecord( const string& name="" );
    ~arExperimentDataRecord();

    void setName( const string& name );
    std::string getName() const;

    unsigned int getNumberFields();
    vector<string> getFieldNames() const;

    bool fieldExists( const string& name );
    bool fieldExists( const string& name, arDataType typ );

%extend{
    bool addStringField( const string& name ) {
      return self->addField( name, AR_CHAR );
    }
    bool addLongField( const string& name ) {
      return self->addField( name, AR_LONG );
    }
    bool addDoubleField( const string& name ) {
      return self->addField( name, AR_DOUBLE );
    }

    bool setStringField( const string& name, const string& val ) {
      return self->setStringFieldValue( name, val);
    }
    bool setLongField( const string& name, const vector<long>& val ) {
      long* tmp = new long[val.size()];
      if (!tmp) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: new in setLongField() failed.");
        return false;
      }
      long* it = tmp;
      vector<long>::const_iterator iter;
      for (iter = val.begin(); iter != val.end(); ++iter) {
        *it++ = *iter;
      }
      bool stat = self->setFieldValue( name, AR_LONG, (const void* const)tmp, val.size() );
      delete[] tmp;
      return stat;
    }
    bool setDoubleField( const string& name, const vector<double>& val ) {
      double* tmp = new double[val.size()];
      if (!tmp) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: new in setDoubleField() failed.");
        return false;
      }
      double* it = tmp;
      vector<double>::const_iterator iter;
      for (iter = val.begin(); iter != val.end(); ++iter) {
        *it++ = *iter;
      }
      bool stat = self->setFieldValue( name, AR_DOUBLE, (const void* const)tmp, val.size() );
      delete[] tmp;
      return stat;
    }

    string getStringField( const string& name ) {
      string value;
      if (!self->getStringFieldValue( name, value )) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: getStringFieldValue() failed.");
        return "NULL";
      }
      return value;
    }
    vector<long> getLongField( const string& name ) {
      vector<long> result;
      unsigned int size;
      long* tmp = (long*)self->getFieldAddress( name, AR_LONG, size );
      if (!tmp) {
        string msg = "arExperimentDataRecord error: getFieldAddress("+name+") failed.";
        PyErr_SetString(PyExc_ValueError,msg.c_str());
        return result;
      }
      for (unsigned int i=0; i<size; ++i) {
        result.push_back( tmp[i] );
      }
      return result;
    }    
    vector<double> getDoubleField( const string& name ) {
      vector<double> result;
      unsigned int size;
      double* tmp = (double*)self->getFieldAddress( name, AR_DOUBLE, size );
      if (!tmp) {
        string msg = "arExperimentDataRecord error: getFieldAddress("+name+") failed.";
        PyErr_SetString(PyExc_ValueError,msg.c_str());
        return result;
      }
      for (unsigned int i=0; i<size; ++i) {
        result.push_back( tmp[i] );
      }
      return result;
    }    
} // extend

%pythoncode %{
def setFieldSequence( self, dataFieldSequence ):
  for dataFieldTuple in dataFieldSequence:
    dataFieldName = dataFieldTuple[0]
    dataFieldType = dataFieldTuple[1]
    dataFieldValue = dataFieldTuple[2]
    stat = False
    if dataFieldType == 'long':
      stat = self.setLongField( dataFieldName, dataFieldValue )
    elif dataFieldType == 'double': 
      stat = self.setDoubleField( dataFieldName, dataFieldValue )
    elif dataFieldType == 'string': 
      stat = self.setStringField( dataFieldName, dataFieldValue )
    else:
      raise PySZGException, 'arExperimentDataRecord error: setFieldSequence() called with invalid type ' \
         +str(dataFieldType)+' for dataField '+str(dataFieldName)
      return False
    if not stat:
      raise PySZGException, 'arExperimentDataRecord error: setFieldSequence() failed for dataField ' \
         +str(dataFieldName)
      return False
  return True
%}

};

class arPythonTrialGenerator : public arTrialGenerator {
  public:
    arPythonTrialGenerator( const string& comment = "" );
    virtual ~arPythonTrialGenerator();
    std::string comment() { return _comment; }
    void setCallback( PyObject* newTrialCallback );
    unsigned long trialNumber() const;
    virtual long numberTrials() const;
};

class arPythonExperimentTrialPhase : public arExperimentTrialPhase {
  public:
    arPythonExperimentTrialPhase( const string& sname="" );
    virtual ~arPythonExperimentTrialPhase();
    string getName() const;
    void setCallbacks( PyObject* initCallback,
                       PyObject* updateCallback,
                       PyObject* updateEventCallback );
};

class arPythonExperiment: public arExperiment {
  public:    
    arPythonExperiment();
    virtual ~arPythonExperiment();
    
    /// Define an experimental parameter or 'factor'..
    /// Note that if address == 0, an internal buffer will
    /// be allocated
    bool addLongFactor( const string& sname );
    bool addDoubleFactor( const string& sname );
    bool addStringFactor( const string& sname );
    // Add a string factor that must belong to a set of values (passed in a list of strings) 
    bool addStringFactorSet( const  string& sname, PyObject* defaultList );

%pythoncode %{
def addFactorSequence( self, factorSequence ):
  for factorTuple in factorSequence:
    factorName = factorTuple[0]
    factorType = factorTuple[1]
    stat = False
    if factorType == 'long':
      stat = self.addLongFactor( factorName )
    elif factorType == 'double': 
      stat = self.addDoubleFactor( factorName )
    elif factorType == 'string': 
      stat = self.addStringFactor( factorName )
    elif factorType == 'stringset': 
      defaultList = factorTuple[2]
      stat = self.addStringFactorSet( factorName, defaultList )
    else:
      raise PySZGException, 'arPythonExperiment error: addFactorSequence() called with invalid type ' \
         +str(factorType)+' for factor '+str(factorName)
      return False
    if not stat:
      raise PySZGException, 'arPythonExperiment error: addFactorSequence() failed for factor ' \
         +str(factorName)
      return False
  return True
%}

    /// Get a factors value
    PyObject* getLongFactor( const string& sname );
    PyObject* getDoubleFactor( const string& sname );
    string getStringFactor( const string& sname );

    /// Define a data field (before init() only)
    bool addLongDataField( const string& sname );
    bool addDoubleDataField( const string& sname );
    bool addStringDataField( const string& sname );

%pythoncode %{
def addDataFieldSequence( self, dataFieldSequence ):
  for dataFieldTuple in dataFieldSequence:
    dataFieldName = dataFieldTuple[0]
    dataFieldType = dataFieldTuple[1]
    stat = False
    if dataFieldType == 'long':
      stat = self.addLongDataField( dataFieldName )
    elif dataFieldType == 'double': 
      stat = self.addDoubleDataField( dataFieldName )
    elif dataFieldType == 'string': 
      stat = self.addStringDataField( dataFieldName )
    else:
      raise PySZGException, 'arPythonExperiment error: addDataFieldSequence() called with invalid type ' \
         +str(dataFieldType)+' for dataField '+str(dataFieldName)
      return False
    if not stat:
      raise PySZGException, 'arPythonExperiment error: addDataFieldSequence() failed for dataField ' \
         +str(dataFieldName)
      return False
  return True
%}

    /// Set the value of a data field
    bool setLongData( const string& sname, PyObject* intDataList );
    bool setDoubleData( const string& sname, PyObject* floatDataList );
    bool setStringData( const string& sname, const string& stringData );
    
%pythoncode %{
def setDataFieldSequence( self, dataFieldSequence ):
  for dataFieldTuple in dataFieldSequence:
    dataFieldName = dataFieldTuple[0]
    dataFieldType = dataFieldTuple[1]
    dataFieldValue = dataFieldTuple[2]
    stat = False
    if dataFieldType == 'long':
      stat = self.setLongData( dataFieldName, dataFieldValue )
    elif dataFieldType == 'double': 
      stat = self.setDoubleData( dataFieldName, dataFieldValue )
    elif dataFieldType == 'string': 
      stat = self.setStringData( dataFieldName, dataFieldValue )
    else:
      raise PySZGException, 'arPythonExperiment error: setDataFieldSequence() called with invalid type ' \
         +str(dataFieldType)+' for dataField '+str(dataFieldName)
      return False
    if not stat:
      raise PySZGException, 'arPythonExperiment error: setDataFieldSequence() failed for dataField ' \
         +str(dataFieldName)
      return False
  return True
%}

    /// Get the value of a data field
    PyObject* getLongData( const string& sname );
    PyObject* getDoubleData( const string& sname );
    string getStringData( const string& sname );

    /// Define a parameter whose value is to be read in from the human subject database.
    bool addLongSubjectParameter( const string& theName );
    bool addDoubleSubjectParameter( const string& theName );
    bool addStringSubjectParameter( const string& theName );

    /// Get the value of a named subject parameter for the current human subject.
    long getLongSubjectParameter( const string& theName );
    double getDoubleSubjectParameter( const string& theName );
    string getStringSubjectParameter( const string& theName );
    
    /// Set the experiment name. This would normally be the name of the executable
    /// (minus .exe under windows). _Use this only in special cases_, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool setName( const string& sname );

    /// Get the experiment name.
    string getName() const;

    /// Determine whether or not data will be saved. This would normally be read
    /// from SZG_EXPT/save_data, but if this has been called that parameter will
    /// be ignored. Use this only in special cases, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool saveData( bool yesno );

    /// Set the trial-generation method. This would normally be read from
    /// SZG_EXPT/method, but if this has been called that parameter will be ignored.
    /// Must be called before init().
    //bool setTrialGenMethod( const string& method );
    //bool setTrialGenMethod( arTrialGenerator* methodPtr );
%extend{
    bool setTrialGenerator( arPythonTrialGenerator* trialGen ) {
      return self->setTrialGenMethod( trialGen );
    }
}

    /// Initialize the experiment.
    bool init( arSZGAppFramework* framework, arSZGClient& SZGClient );

    /// Start the experiment.
    bool start();
    /// Stop the experiment.
    bool stop();
    /// Save data from last trial, get parameters for next.
    bool newTrial( bool saveLastTrial=true );
    /// Return current trial number.
    long currentTrialNumber() const;
    /// Return total number of trials (-1 if undefined, not prespecified).
    long numberTrials() const;
    /// Return completion status.
    bool completed() const;
    
    /// Add or replace a trial phase object by name
    bool setTrialPhase( const string& theName, arPythonExperimentTrialPhase* thePhase );
    /// Get the name of the current trial phase
    string currentTrialPhase();
    /// Activate a trial phase by name
    bool activateTrialPhase( const string& theName, arSZGAppFramework* fw );
    /// Calls update() method of active trial phase object
    bool updateTrialPhase( arSZGAppFramework* fw );
%extend{
    bool updateTrialPhaseEvent( arSZGAppFramework* fw, arPythonEventFilter* filter, arInputEvent& event ) {
      return self->updateTrialPhase( fw, filter, event );
    }
}
};


%pythoncode %{

class arPyTrialGenerator(arPythonTrialGenerator):
  def __init__(self, comment=''):
    arPythonTrialGenerator.__init__(self, comment)
    self.setCallback( self.onNewTrial )
  def setFactorSequence( self, factorRecord, factorSequences ):
    for factorSequence in factorSequences:
      factorName = factorSequence[0]
      factorType = factorSequence[1]
      factorValue = factorSequence[2]
      stat = False
      if factorType == 'double': 
        stat = factorRecord.setDoubleField( factorName, factorValue )
      elif factorType == 'long':
        stat = factorRecord.setLongField( factorName, factorValue )
      elif factorType == 'string':
        stat = factorRecord.setStringField( factorName, factorValue )
      else:
        raise PySZGException, 'arPyTrialGenerator: setFactorTuples() called with bad type for factor "'+ \
                str(factorName)+'" with type "'+str(factorType)
      if not stat:
        raise PySZGException, 'arPyTrialGenerator: setFactorTuples() failed for factor "'+ \
                str(factorName)+'" with type "'+str(factorType)+'" and value:\n'+str(factorValue)
        return False
    return True
  def onNewTrial( self, factors ):
    raise PySZGException, 'arPyTrialGenerator: you must override onNewTrial().'
    return False

class arPyExperimentTrialPhase(arPythonExperimentTrialPhase):
  def __init__(self,name):
    arPythonExperimentTrialPhase.__init__(self,name)
    self.setCallbacks( self.onInit, self.onUpdate, self.onUpdateEvent )   
  def onInit( self, framework, expt ):
    print 'arPyExperimentTrialPhase: you must override onInit().'
    raise PySZGException, 'arPyExperimentTrialPhase: you must override onInit().'
    return False
  def onUpdate( self, framework, expt ):
    print 'arPyExperimentTrialPhase: you must override onUpdate().'
    raise PySZGException, 'arPyExperimentTrialPhase: you must override onUpdate().'
    return False
  def onUpdateEvent( self, framework, expt, filter, event ):
    print 'arPyExperimentTrialPhase: you must override onUpdateEvent().'
    raise PySZGException, 'arPyExperimentTrialPhase: you must override onUpdateEvent().'
    return False
    

%}


%{
#include "arGluQuadric.h"
#include "arGluSphere.h"
#include "arGluCylinder.h"
#include "arGluDisk.h"
#include "arPatternedBox.h"
#include "arGluTessObject.h"
%}

class arGluQuadric : public arInteractableThing {
  public:
    arGluQuadric();
    arGluQuadric( const arGluQuadric& x );
/*    arGluQuadric& operator=( const arGluQuadric& x );*/
    virtual ~arGluQuadric();
    void setPointStyle() { _drawStyle = GLU_POINT; }
    void setLineStyle() { _drawStyle = GLU_LINE; }
    void setSilhouetteStyle() { _drawStyle = GLU_SILHOUETTE; }
    void setFillStyle() { _drawStyle = GLU_FILL; }
    void setNormalsOutside( bool trueFalse ) {
      _normalDirection = (trueFalse)?(GLU_OUTSIDE):(GLU_INSIDE);
    }
    void setNoNormals() { _normalStyle = GLU_NONE; }
    void setFlatNormals() { _normalStyle = GLU_FLAT; }
    void setSmoothNormals() { _normalStyle = GLU_SMOOTH; }
    virtual void draw( arMasterSlaveFramework* fw=0 )  = 0;
};

class arGluSphere : public arGluQuadric {
  public:
    arGluSphere( double radius, int slices=30, int stacks=5 );
    arGluSphere( const arGluSphere& x );
    virtual ~arGluSphere() {}
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadius( double radius );
    void setSlicesStacks( int slices, int stacks );

  // inherited from arInteractable
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );

  // inherited from arInteractableThing
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }

  // inherited from arGluQuadric
    void setPointStyle() { _drawStyle = GLU_POINT; }
    void setLineStyle() { _drawStyle = GLU_LINE; }
    void setSilhouetteStyle() { _drawStyle = GLU_SILHOUETTE; }
    void setFillStyle() { _drawStyle = GLU_FILL; }
    void setNormalsOutside( bool trueFalse ) {
      _normalDirection = (trueFalse)?(GLU_OUTSIDE):(GLU_INSIDE);
    }
    void setNoNormals() { _normalStyle = GLU_NONE; }
    void setFlatNormals() { _normalStyle = GLU_FLAT; }
    void setSmoothNormals() { _normalStyle = GLU_SMOOTH; }

};


class arGluCylinder : public arGluQuadric {
  public:
    arGluCylinder( double startRadius, double endRadius, double length, int slices=30, int stacks=5 );
    arGluCylinder( const arGluCylinder& x );
/*    arGluCylinder& operator=( const arGluCylinder& x );*/
    virtual ~arGluCylinder() {}
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadii( double startRadius, double endRadius );
    void setLength( double length );
    void setSlicesStacks( int slices, int stacks );

  // inherited from arInteractable
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );

  // inherited from arInteractableThing
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }

  // inherited from arGluQuadric
    void setPointStyle() { _drawStyle = GLU_POINT; }
    void setLineStyle() { _drawStyle = GLU_LINE; }
    void setSilhouetteStyle() { _drawStyle = GLU_SILHOUETTE; }
    void setFillStyle() { _drawStyle = GLU_FILL; }
    void setNormalsOutside( bool trueFalse ) {
      _normalDirection = (trueFalse)?(GLU_OUTSIDE):(GLU_INSIDE);
    }
    void setNoNormals() { _normalStyle = GLU_NONE; }
    void setFlatNormals() { _normalStyle = GLU_FLAT; }
    void setSmoothNormals() { _normalStyle = GLU_SMOOTH; }

};


class arGluDisk : public arGluQuadric {
  public:
    arGluDisk( double innerRadius, double outerRadius, int slices=30, int rings=5 );
    arGluDisk( const arGluDisk& x );
/*    arGluDisk& operator=( const arGluDisk& x );*/
    virtual ~arGluDisk() {}
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadii( double innerRadius, double outerRadius );
    void setSlicesRings( int slices, int rings );

  // inherited from arInteractable
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );

  // inherited from arInteractableThing
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }

  // inherited from arGluQuadric
    void setPointStyle() { _drawStyle = GLU_POINT; }
    void setLineStyle() { _drawStyle = GLU_LINE; }
    void setSilhouetteStyle() { _drawStyle = GLU_SILHOUETTE; }
    void setFillStyle() { _drawStyle = GLU_FILL; }
    void setNormalsOutside( bool trueFalse ) {
      _normalDirection = (trueFalse)?(GLU_OUTSIDE):(GLU_INSIDE);
    }
    void setNoNormals() { _normalStyle = GLU_NONE; }
    void setFlatNormals() { _normalStyle = GLU_FLAT; }
    void setSmoothNormals() { _normalStyle = GLU_SMOOTH; }

};


class arPatternedBox : public arInteractableThing {
  public:
    arPatternedBox();
    virtual ~arPatternedBox(void);
    arPatternedBox( const arPatternedBox& b );
/*    arPatternedBox& operator=( const arPatternedBox& b );*/
    
    void setUseTexture( bool useTexture ) { _useTexture = useTexture; }
    bool getUseTexture() const { return _useTexture; }

    void setSize( const float w, const float h, const float d, const float s=0 );
    virtual void draw( arMasterSlaveFramework* fw=0 );
  
    void setTextureScale( float sc ) { _texScale = sc; }

  // inherited from arInteractable
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );

  // inherited from arInteractableThing
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }
};


class arGluTessObject : public arInteractableThing {
  public:
    arGluTessObject( bool useDisplayList = false );
    arGluTessObject( const arGluTessObject& x );
    arGluTessObject& operator=( const arGluTessObject& x );
    virtual ~arGluTessObject();
    void setScaleFactors( const arVector3& scales );
    void setScaleFactors( float x, float y, float z );
    arVector3 getScaleFactors();
    void setTextureScales( float sScale, float tScale );
    void setTextureOffsets( float sOffset, float tOffset );
    void useDisplayList( bool use );
    bool buildDisplayList();
    virtual void draw();

  // inherited from arInteractable
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );

  // inherited from arInteractableThing
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }

%extend{
PyObject* addContour( PyObject* vertexSequence  ) {
  if (!PySequence_Check( vertexSequence )) {
    PyErr_SetString(PyExc_TypeError,
        "arGluTessObject.addContour() error: vertexSequence argument must be a sequence.");
    return NULL;
  }
  std::vector< arVector3 > contVec;
  int numItems = PySequence_Size( vertexSequence );
  for (int i=0; i<numItems; ++i) {
    PyObject* vecSeq = PySequence_GetItem( vertexSequence, i );
    if (!PySequence_Check( vecSeq )) {
      PyErr_SetString(PyExc_TypeError,
         "arGluTessObject.addContour() error: each item in vertexSequence must be a sequence.");
      return NULL;
    }
    if (PySequence_Size( vecSeq ) != 3) {
      PyErr_SetString(PyExc_ValueError, 
         "arGluTessObject.addContour() error: each sequence in vertexSequence must have length 3.");
      return NULL;
    }
    arVector3 tmpVec;
    for (unsigned int j=0; j<3; ++j) {
      PyObject* tmpNum = PySequence_GetItem( vecSeq, j );
      if (PyFloat_Check( tmpNum )) {
        tmpVec.v[j] = (float)PyFloat_AsDouble( tmpNum );
      } else if (PyInt_Check( tmpNum )) {
        tmpVec.v[j] = (float)PyInt_AsLong( tmpNum );
      } else {
        PyErr_SetString(PyExc_ValueError, 
           "arGluTessObject.addContour() error: each sequence in vertexSequence must contain 3 numbers.");
        return NULL;
      }
      Py_XDECREF( tmpNum );
    }
    Py_XDECREF( vecSeq );
    contVec.push_back( tmpVec );
  }
  self->addContour( contVec );
  Py_RETURN_NONE;
}
} // extend

};
