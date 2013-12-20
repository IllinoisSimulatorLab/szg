//%Module szgframeworks 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include graphics.sip
%Include szgclient.sip

%ModuleHeaderCode
#include <string>
using namespace std;

/*
extern string rencodeString;
static PyObject* _rencode = NULL;
static PyObject* _rencodeModule = NULL;

static bool _loadRencode() {
  if (!_rencode || !_rencodeModule) {
    if (!(_rencode = Py_CompileString( rencodeString.c_str(), "rencode2", Py_file_input ))) {
      PyErr_SetString( PyExc_RuntimeError, "failed to compile rencode string" );
      return false;
    }
    if (!(_rencodeModule = PyImport_ExecCodeModule( "rencode2", _rencode ))) {
      Py_XDECREF( _rencode );
      PyErr_SetString( PyExc_RuntimeError, "failed to load rencode2 module" );
      return false;
    }
  }
  return true;
}
*/

#include "arSZGAppFramework.h"

static bool _framework_init( arSZGAppFramework*fw, vector<string>& args ) {
  int argc = args.size();
  char** argv = new char*[argc];
  int i;
  for (i=0; i<argc; ++i) {
    const string& s = args[i];
    argv[i] = new char[s.size()+1];
    memcpy( argv[i], s.c_str(), s.size()+1 );
  }
  bool result = fw->init( argc, argv );
  for (i=0; i<argc; ++i) {
    delete[] argv[i];
  }
  delete[] argv;
  return result;
}


// A utility function used by the master/slave framework (on the python side) to share
// data between synchronized program instances. See initSequenceTransfer, setSequence, getSequence.
static bool _msPackSequenceData( PyObject* seq, std::vector<int>& typeData,
                            std::vector<long>& intData, std::vector<double>& floatData,
                            std::vector<char*>& stringData, std::vector<int>& stringSizeData ) {
  if (!PySequence_Check( seq )) {
    PyErr_SetString( PyExc_TypeError, 
         "arMasterSlaveFramework error: invalid sequence type passed to setSequence()." );
    return false;
  }
  int len = PySequence_Size( seq );
  if (len == -1) {
    PyErr_SetString( PyExc_TypeError,
         "arMasterSlaveFramework error: failed to compute sequence length in setSequence()." );
    return false;
  }
  /*cerr << "[" << len;*/
  // push the size of the sequence
  typeData.push_back(len);
  for (int i=0; i<len; ++i) {
    PyObject* item = PySequence_GetItem( seq, i );
 
    // Its an Int
    if (PyInt_Check( item )) {
      intData.push_back( PyInt_AsLong( item ) );
      /*cerr << "L" << PyInt_AsLong(item);*/
      typeData.push_back( AR_LONG );

    // Its a Float
    } else if (PyFloat_Check(item)) {
      floatData.push_back( PyFloat_AsDouble( item ) );
      typeData.push_back( AR_DOUBLE );
      /*cerr << "F" << PyFloat_AsDouble(item);*/

    // Its a String
    } else if (PyString_Check( item )) {
      char* cdata;
      Py_ssize_t ssize;
      PyString_AsStringAndSize( item, &cdata, &ssize );
      stringData.push_back( cdata );
      stringSizeData.push_back( (int)ssize );
      typeData.push_back( AR_CHAR );
      /*cerr << "'" << cdata << "'";*/

    // Its a nested sequence
    } else if (PySequence_Check( item )) {

      // push the sequence marker. the size of the subsequence will be pushed at the 
      // beginning of the recurse.
      typeData.push_back(-1);
      bool stat = _msPackSequenceData( item, typeData, intData, floatData, stringData, stringSizeData );
      if (!stat) {
        Py_XDECREF(item);
        return false;
      }

    // Its a what?
    } else {
      cerr << "arMasterSlaveFramework warning: invalid data type ignored in setSequence().\n";
    }
    Py_XDECREF(item);
  }
  /*cerr << "]";*/
  return true;
}

// A utility function used by the master/slave framework (on the python side) to share
// data between synchronized program instances. See initSequenceTransfer, setSequence, getSequence.
static PyObject* _msUnpackSequenceData( int** typePtrPtr, long** intPtrPtr, double** floatPtrPtr, 
                                 char** charPtrPtr, int** charSizePtrPtr ) {
  int* typePtr = *typePtrPtr;
  long* intDataPtr = *intPtrPtr;
  double* floatDataPtr = *floatPtrPtr;
  char* charPtr = *charPtrPtr;
  int* charSizePtr = *charSizePtrPtr;
  int size = *typePtr++;
  PyObject* seq = PyTuple_New( size );
  if (!seq) {
    PyErr_SetString( PyExc_MemoryError, "unable to get allocate new tuple." );
    return NULL;
  }
  int stringLength;
  PyObject* nestedSequence;
  ostringstream os;
  string errMsg;

  for(int i=0; i<size; ++i) {
    switch (*typePtr++) {
      case AR_LONG:
        PyTuple_SetItem( seq, i, PyInt_FromLong(*intDataPtr++) );
        break;
      case AR_DOUBLE:
        PyTuple_SetItem( seq, i, PyFloat_FromDouble(*floatDataPtr++) );
        break;
      case AR_CHAR:
        stringLength = *charSizePtr++;
/*        cerr << "slave: " << stringLength << ": ";*/
/*        for (j=0; j<stringLength; ++j) {*/
/*          cerr << *(charPtr+j);*/
/*        }*/
/*        cerr << endl;*/
        PyTuple_SetItem( seq, i, PyString_FromStringAndSize( charPtr, stringLength ) );
        charPtr += stringLength;
        break;
      case -1:  // my arbitrary "seqeuence" type
        nestedSequence = _msUnpackSequenceData( &typePtr, &intDataPtr, &floatDataPtr, &charPtr, &charSizePtr );
        if (!nestedSequence) {
          os.str();
          os << "arMasterSlaveFramework error: failed to unpack nested sequence in ar_unpackSequenceData().\n"
             << " Item " << i << " of " << size;
          string errMsg = os.str();
          cerr << errMsg << endl;
          PyErr_SetString(PyExc_TypeError, errMsg.c_str() );
          Py_DECREF( seq );
          return NULL;
        }
        PyTuple_SetItem( seq, i, nestedSequence );
        break;
      default:
        os.str();
        os << "arMasterSlaveFramework error: invalid type in ar_unpackSequenceData().\n"
                   << " Item " << i << " of " << size;
        string errMsg = os.str();
        cerr << errMsg << endl;
        PyErr_SetString(PyExc_TypeError, errMsg.c_str() );
        Py_DECREF( seq );
        return NULL;
    }
  }
  *typePtrPtr = typePtr;
  *intPtrPtr = intDataPtr;
  *floatPtrPtr = floatDataPtr;
  *charPtrPtr = charPtr;
  *charSizePtrPtr = charSizePtr;
  return seq;
}


// C implementation of standard reshape behavior (currently unused, reshape callback is gone,
// I need to add something for the window event callback).
void ar_defaultReshapeCallback( int width, int height ) {
  glViewport( 0, 0, width, height );
}

void ar_defaultDisconnectDraw() {
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho( -1.0, 1.0, -1.0, 1.0, 0.0, 1.0 );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glPushAttrib( GL_ALL_ATTRIB_BITS );
  glDisable( GL_LIGHTING );
  glColor3f( 0.0, 0.0, 0.0 );
  glBegin( GL_QUADS );
  glVertex3f(  1.0,  1.0, -0.5 );
  glVertex3f( -1.0,  1.0, -0.5 );
  glVertex3f( -1.0, -1.0, -0.5 );
  glVertex3f(  1.0, -1.0, -0.5 );
  glEnd();
  glPopAttrib();
}


%End

class arCallbackException {
  public:
    string message;
    arCallbackException( const string& msg );
};


class arAppLauncher {
%TypeHeaderCode
#include "arAppLauncher.h"
%End
  // Needs assignment operator and copy constructor, for pointer members.
 public:
  //arAppLauncher(const char* exeName = NULL);
  // a0 and a1 in MethodCode are _pointers_ to the respective arguments!!!
  arAppLauncher( const string& a0 );
%MethodCode
    sipCpp = new arAppLauncher( a0->c_str() );
%End

  ~arAppLauncher();

  bool setRenderProgram(const string&);
  bool setAppType(const string&);
  bool setVircomp();
  bool setVircomp(const string& vircomp); 
  string getVircomp() const;
  string getLocation() const;

  //bool connectSZG(int& argc, char** argv);
  bool connectSZG( vector<string>& a0 );
  // a0 and a1 in MethodCode are _pointers_ to the respective arguments!!!
%MethodCode
    int argc = a0->size();
    char** argv = new char*[argc];
    int i;
    for (i=0; i<argc; ++i) {
      const string& s = (*a0)[i];
      argv[i] = new char[s.size()+1];
      memcpy( argv[i], s.c_str(), s.size()+1 );
    }
    sipRes = sipCpp->connectSZG( argc, argv );
    for (i=0; i<argc; ++i) {
      delete[] argv[i];
    }
    delete[] argv;
%End


  // doesn't own arSZGClient if installed this way.
  bool setSZGClient(arSZGClient* /TransferBack/);

  void killApp();
  bool killAll();
  bool launchApp();
  bool restartServices();
  bool screenSaver();
  bool setParameters();
  bool waitForKill();

  // A "screen" of the v.c. is a window that is rendered into.
  // See doc/GraphicsConfiguration.html.

  bool isMaster() const;
  int getNumberDisplays() const;
  string getMasterName() const;
  string getTriggerName() const;
  string getDisplayName(const int) const;
  bool getRenderProgram(const int num, string& computer, string& renderName);
  void updateRenderers(const string& attribute, const string& value);
};



class arInputSimulator {
%TypeHeaderCode
#include "arInputSimulator.h"
%End
 public:
  arInputSimulator();
  virtual ~arInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );
  void registerInputNode(arInputNode* node /TransferBack/);

  virtual void draw();
  virtual void drawWithComposition();
  virtual void advance();

  // used to capture and process mouse/keyboard data
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( vector<unsigned>& mouseButtons );
  vector<unsigned> getMouseButtons() const /TransferBack/;
  void setNumberButtonEvents( unsigned numButtonEvents ); 
  unsigned getNumberButtonEvents() const;
 protected:
  arInputSimulator( const arInputSimulator& );
};


class arFrameworkEventFilter : arIOFilter {
%TypeHeaderCode
#include "arFrameworkEventFilter.h"
%End
  public:
    arFrameworkEventFilter( arSZGAppFramework* fw = 0 );
    virtual ~arFrameworkEventFilter();
    void saveEventQueue( bool onoff );
    void setFramework( arSZGAppFramework* fw );
    arSZGAppFramework* getFramework() const;
    void queueEvent( const arInputEvent& event );
    arInputEventQueue getEventQueue();
    void flushEventQueue();
  private:
    arFrameworkEventFilter( const arFrameworkEventFilter& );
};


class arSZGAppFramework {
%TypeHeaderCode
#include "arSZGAppFramework.h"
%End
  public:
    virtual ~arSZGAppFramework();
    
    // Event loop management and related methods.
    //virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    virtual bool createWindows(bool useWindowing) = 0;
    virtual void loopQuantum() = 0;
    virtual void exitFunction() = 0;
    
    // Methods common to all frameworks.

    virtual void setDataBundlePath( const string& /*bundlePath*/,
                                    const string& /*bundleSubDir*/ );
    virtual void loadNavMatrix();
    void speak( const string& message );
    bool setInputSimulator( arInputSimulator* sim /TransferBack/ );
    string getLabel() const;
    bool getStandalone() const;
    const string getDataPath() const;

    // Set-up the viewer (i.e. the user's head).
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    arHead* getHead();
    virtual void setFixedHeadMode(bool isOn);
    virtual arMatrix4 getMidEyeMatrix() /TransferBack/;
    virtual arVector3 getMidEyePosition() /TransferBack/;
    virtual void setUnitConversion( float );
    virtual float getUnitConversion();

    // Access to the embedded input node.  Also see getInputNode().
    int getButton( const unsigned ) const;
    float getAxis( const unsigned ) const;
    arMatrix4 getMatrix( const unsigned, bool doUnitConversion=true ) const /TransferBack/;
    bool getOnButton( const unsigned ) const;
    bool getOffButton( const unsigned ) const;
    unsigned getNumberButtons()  const;
    unsigned getNumberAxes()     const;
    unsigned getNumberMatrices() const;

    // Methods for built-in navigation.

    bool setNavTransCondition( char axis,
                               arInputEventType type,
                               unsigned index,
                               float threshold );
    bool setNavRotCondition( char axis,
                             arInputEventType type,
                             unsigned index,
                             float threshold );
    void setNavTransSpeed( float speed );
    void setNavRotSpeed( float speed );
    void setNavEffector( const arEffector& effector );
    void ownNavParam( const string& paramName );
    void navUpdate();
    void navUpdate( const arMatrix4& navMatrix );
    void navUpdate( const arInputEvent& event );
    void setUseNavInputMatrix( const bool onoff );
    void setNavInputMatrixIndex( const unsigned index );
    void setUnitConvertNavInputMatrix( const bool onoff );

    // Methods for event filtering (and the callbacks that allow
    // event-based processing instead of polling-based processing).

    //bool setEventFilter( arFrameworkEventFilter* filter );
    //void setEventCallback( arFrameworkEventCallback callback );
    //virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );
    void processEventQueue();
    virtual void onProcessEventQueue( arInputEventQueue& theQueue ) /PyName=onInputEventQueue/;

    // This now gets called by default...
    virtual bool onInputEvent( arInputEvent& event, arFrameworkEventFilter& filter );

    virtual void onUserMessage( const int messageID, const string& messageBody );

    // Should this return a copy instead? In some cases it points
    // inside the arInputNode.
    arInputState* getInputState();

    // This used to be in arDistSceneGraphFramework. Moved it up here
    // for the arMasterSlaveFramework sound database.
    const string getNavNodeName() const;

    // Some apps need a thread running external to the library.
    // For deterministic shutdown, we need to register that
    // thread's existence, know when it is shutting down, etc.

    // If an external event loop wants to call exit(),
    // it should wait until stopped() returns true, i.e. stop() has completed.
    bool stopped() const;

    // True iff shutdown has begun.
    bool stopping() const;

    void useExternalThread();

    // Tell stop() that external thread has started.
    void externalThreadStarted();

    // Tell stop() that external thread stopped cleanly.
    void externalThreadStopped();

    // Accessors for various internal objects/services. Necessary for flexibility.
    
    // Info about the virtual computer.
    arAppLauncher* getAppLauncher();
    // For nonstandard use of the input node.
    arInputNode* getInputNode();
    arInputNode* getInputDevice();
    // Allowing the user access to the window manager increases the flexibility
    // of the framework. Lots of info about the GUI becomes available.
    arGUIWindowManager* getWindowManager() const;
    // Some applications want to be able to work with the arSZGClient directly.
    arSZGClient* getSZGClient();

    // Add entries to the data bundle path (used to locate texture maps by
    // szgrender for scene-graph apps in cluster mode and by SoundRender to
    // locate sounds for both types of apps in cluster mode).
    virtual void addDataBundlePathMap(const string& /*bundlePathName*/, 
                            const string& /*bundlePath*/);

    void postInputEvent( arInputEvent& event );

 private:
    arSZGAppFramework();
    arSZGAppFramework( const arSZGAppFramework& );
};


/**
 * @class arMasterSlaveFramework
 * @brief Framework for cluster applications using one master and several slaves.
 *
 * This is the core of a Syzygy program. It coordinates all aspects of the
 * application: Contacting the Syzygy server for cluster configuration
 * info, connecting to other instances of the program, window creation,
 * and so on. It also runs the programs event loop.
 *
 * You create a program by instantiating one of these and then either
 * installing callback functions a la GLUT or by creating a subclass
 * and overriding the callback methods. Callback method names begin
 * with "on". For example, to render the contents of a viewport you
 * can either create a function and install it using
 * setDrawCallback() or you can override the onDraw() method. 
 */
class arMasterSlaveFramework : arSZGAppFramework {
%TypeHeaderCode
#include "arMasterSlaveFramework.h"
%End
 public:
  arMasterSlaveFramework();
  virtual ~arMasterSlaveFramework();
  
  /**
   * @brief   Initializes the framework. Does NOT start the event loop.
   *
   * Pass in sys.argv (vector<string> maps to a Python list of strings).
   *
   * @return  True/False indicating success/failure. If false (usually
   * indicating the the szgserver specified in szg_<user>.conf--by
   * dlogin--could not be found), app must exit.
   */
  bool init( vector<string> a0 );
  // a0 and a1 in MethoCode are _pointers_ to the respective arguments!!!
%MethodCode
    sipRes = _framework_init( sipCpp, *a0 );
%End

  /**
   * @brief  Starts the application event loop.
   *
   * Creates windows, calls the user-supplied startCallback or onStart() method,
   * and starts the event loop.
   *
   * @return True/False. If everything worked, start() never returns, so code
   * after this will only be executed if an error occurs in e.g. window creation
   * or the onStart() method. 
   */
  bool start();

  // Start services, maybe windowing, and maybe an internal event loop.
  // Returns only if useEventLoop is false, or on error.
  // If useEventLoop is false, caller should run the event loop either
  // coarsely via loopQuantum() or finely via preDraw(), postDraw(), etc.
  // Two functions, because default args don't jive with virtual arSZGAppFramework::start().
  bool start(bool useWindowing, bool useEventLoop);

  // Shutdown for much (BUT NOT ALL YET) of the arMasterSlaveFramework services.
  // if the parameter is set to true, block until the display thread exits
  void stop( bool blockUntilDisplayExit );

  bool createWindows(bool useWindowing);
  void loopQuantum();
  void exitFunction();  
  virtual void preDraw();
  // Different than onDraw (and the corresponding draw callbacks). 
  // Essentially causes the window manager to draw all the windows.
  void draw( int windowID = -1 );
  virtual void postDraw();
  void swap( int windowID = -1 );

  // Another layer of indirection to promote object-orientedness. 
  // We MUST have callbacks to subclass this object in Python.
  // At each point the callbacks were formerly invoked, the code instead calls 
  // these virtual (hence overrideable) methods, which in turn invoke the callbacks.
  // This allows subclassing instead of callbacks.
  // (If you override these, the corresponding callbacks are of course ignored).

  // Convenient options for arSZGClient.
  virtual bool onStart( arSZGClient& SZGClient );
  virtual void onWindowStartGL( arGUIWindowInfo* ) /PyName=_onWindowStartGL/;
  virtual void onPreExchange() /PyName=_onPreExchange/;
  virtual void onPostExchange() /PyName=_onPostExchange/;
  virtual void onWindowInit() /PyName=_onWindowInit/;
  virtual void onDraw( arGraphicsWindow& win, arViewport& vp ) /PyName=_onDraw/;
  virtual void onDisconnectDraw() /PyName=_onDisconnectDraw/;
  virtual void onPlay() /PyName=_onPlay/;
  virtual void onWindowEvent( arGUIWindowInfo* ) /PyName=_onWindowEvent/;
  virtual void onCleanup() /PyName=_onCleanup/;
  virtual void onOverlay() /PyName=_onOverlay/;
  virtual void onKey( arGUIKeyInfo* ) /PyName=_onKey/;
//  virtual void onMouse( arGUIMouseInfo* ) /PyName=_onMouse/;

  void setDataBundlePath( const string& bundlePathName,
                          const string& bundleSubDirectory );
  void loadNavMatrix();
  void setPlayTransform();
  void drawGraphicsDatabase();
  void usePredeterminedHarmony();
  int getNumberSlavesExpected();
  bool allSlavesReady();
  // How many slaves are currently connected?
  int getNumberSlavesConnected() const;

  bool sendMasterMessage( const string& messageBody );

  // Three ways to share data from the master to the slaves.

  // 1: simplest way.  Register global variables
  // with the framework (the transfer fields). Every frame,
  // the master dumps its values to the slaves.
  // Each field has a fixed size.
  //bool addTransferField( string fieldName, void* data,
  //                       arDataType dataType, int size );

  // 2: the framework manages the memory blocks, so they can resize.
  // Add an internally-allocated transfer field.
  // Resize them on the master, and the slaves automatically follow.
//  bool addInternalTransferField( string fieldName,
//                                 arDataType dataType,
//                                 int size );
//  bool setInternalTransferFieldSize( string fieldName,
//                                     arDataType dataType,
//                                     int newSize );
  // Get a pointer to an externally- or internally-stored
  // transfer field.  Internally stored fields are resizable.
  //void* getTransferField( string fieldName,
  //                        arDataType dataType,
  //                        int& size );

  // 3: register arFrameworkObjects with the framework's
  // internal arMasterSlaveDataRouter. An identical set of arFrameworkObjects
  // is created by each of the master and slave instances. The
  // arMasterSlaveDataRouter routes messages between them.
  //bool registerFrameworkObject( arFrameworkObject* object );

  void initStringTransfer( const string& a0 );
%MethodCode
    if (!sipCpp->addInternalTransferField( *a0, AR_CHAR, 1 )) {
      PyErr_SetString( PyExc_RuntimeError, "arMasterSlaveFramework failed to add transfer field." );
      sipIsErr = 1;
    }
%End

  int getStringSize( const string& a0 );
%MethodCode
    if (!sipCpp->getTransferField( *a0, AR_CHAR, sipRes )) {
      PyErr_SetString( PyExc_RuntimeError, "getStringSize() unknown transfer field." );
      sipIsErr = 1;
    }
%End

  void setString( const string& a0, const string& a1 );
%MethodCode
    int osize;
    if (!sipCpp->getTransferField( *a0, AR_CHAR, osize )) {
      PyErr_SetString( PyExc_RuntimeError, "setString() unknown transfer field." );
      sipIsErr = 1;
    } else {
      int size = (int)a1->size()+1;
      if (osize!=size) {
          if (!sipCpp->setInternalTransferFieldSize( *a0, AR_CHAR, size )) {
              PyErr_SetString(PyExc_MemoryError,
                      "setString() unable to resize transfer field");
              sipIsErr = 1;
          }
      }
      if (!sipIsErr) {
        char* ptr=(char *) sipCpp->getTransferField( *a0, AR_CHAR, size );
        memcpy( ptr, a1->c_str(), size );
      }
    }
%End

  string getString( const string& a0 );
%MethodCode
    int size;
    char *ptr=(char *) sipCpp->getTransferField( *a0, AR_CHAR, size );
    if (!ptr) {
      PyErr_SetString( PyExc_RuntimeError, "getString() failed to get transfer field." );
      sipIsErr = 1;
    } else {
      sipRes = new string( ptr, size );
    }
%End

  void initSequenceTransfer( const string& a0 );
%MethodCode
    if (!sipCpp->addInternalTransferField( *a0+string("_INTDATA"), AR_LONG, 1 )) {
      PyErr_SetString(PyExc_MemoryError,"initSequenceTransfer unable to create transfer field");
      sipIsErr = 1;
    }
    if (!sipCpp->addInternalTransferField( *a0+string("_FLOATDATA"), AR_DOUBLE, 1 )) {
      PyErr_SetString(PyExc_MemoryError,"initSequenceTransfer unable to create transfer field");
      sipIsErr = 1;
    }
    if (!sipCpp->addInternalTransferField( *a0+string("_CHARDATA"), AR_CHAR, 1 )) {
      PyErr_SetString(PyExc_MemoryError,"initSequenceTransfer unable to create transfer field");
      sipIsErr = 1;
    }
    if (!sipCpp->addInternalTransferField( *a0+string("_CHARSIZEDATA"), AR_INT, 1 )) {
      PyErr_SetString(PyExc_MemoryError,"initSequenceTransfer unable to create transfer field");
      sipIsErr = 1;
    }
    if (!sipCpp->addInternalTransferField( *a0+string("_TYPES"), AR_INT, 1 )) {
      PyErr_SetString(PyExc_MemoryError,"initSequenceTransfer unable to create transfer field");
      sipIsErr = 1;
    }
%End


  void setSequence( const string& a0, SIP_PYOBJECT a1 );
%MethodCode
    std::vector<int> typeData;
    std::vector<long> intData;
    std::vector<double> floatData;
    std::vector<char *> stringData;
    std::vector<int> stringSizeData;

    string intString = *a0+string("_INTDATA");
    string floatString = *a0+string("_FLOATDATA");
    string charString = *a0+string("_CHARDATA");
    string charSizeString = *a0+string("_CHARSIZEDATA");
    string typeString = *a0+string("_TYPES");

    int totalStringSize(0);
    std::vector<int>::iterator iter;
    int i, osize;

    long* typePtr;
    long* longPtr;
    double* doublePtr;
    int* stringSizePtr;
    char* charPtr;

    /*cerr << "setSequence: " << name;*/
    if (!_msPackSequenceData( a1, typeData, intData, floatData, stringData, stringSizeData )) {
      cerr << "arMasterSlaveFramework error: setSequence() failed.\n";
      sipIsErr = 1;
      goto packdone;
    }
    /*cerr << endl;*/
    for (iter = stringSizeData.begin(); iter != stringSizeData.end(); ++iter) {
      totalStringSize += *iter;
    }
    if (!sipCpp->setInternalTransferFieldSize( typeString, AR_INT, (int)typeData.size() )) {
      cerr << "arMasterSlaveFramework error: unable to resize 'type' transfer field to "
           << typeData.size() << endl;
      PyErr_SetString(PyExc_RuntimeError, "setInternalTransferFieldSize() 1 failed.");
      sipIsErr = 1;
      goto packdone;
    }
    if (!intData.empty()) {
      if (!sipCpp->setInternalTransferFieldSize( intString, AR_LONG, (int)intData.size() )) {
        cerr << "arMasterSlaveFramework error: unable to resize 'intData' transfer field to "
             << intData.size() << endl;
        PyErr_SetString(PyExc_RuntimeError, "setInternalTransferFieldSize() 2 failed.");
        sipIsErr = 1;
        goto packdone;
      }
    }
    if (!floatData.empty()) {
      if (!sipCpp->setInternalTransferFieldSize( floatString, AR_DOUBLE, (int)floatData.size() )) {
        cerr << "arMasterSlaveFramework error: unable to resize 'floatData' transfer field to "
             << floatData.size() << endl;
        PyErr_SetString(PyExc_RuntimeError, "setInternalTransferFieldSize() 3 failed.");
        sipIsErr = 1;
        goto packdone;
      }
    }
    if (totalStringSize > 0) {
      if (!sipCpp->setInternalTransferFieldSize( charSizeString, AR_INT, (int)stringSizeData.size() )) {
        cerr << "arMasterSlaveFramework error: unable to resize 'stringSize' transfer field to "
             << stringSizeData.size() << endl;
        PyErr_SetString(PyExc_RuntimeError, "setInternalTransferFieldSize() 4 failed.");
        sipIsErr = 1;
        goto packdone;
      }
      if (!sipCpp->setInternalTransferFieldSize( charString, AR_CHAR, totalStringSize )) {
        cerr << "arMasterSlaveFramework error: unable to resize 'stringData' transfer field to "
             << totalStringSize << endl;
        PyErr_SetString(PyExc_RuntimeError, "setInternalTransferFieldSize() 5 failed.");
        sipIsErr = 1;
        goto packdone;
      }
    }

    typePtr=(long *)sipCpp->getTransferField( typeString, AR_INT, osize );
    if (!typePtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      sipIsErr = 1;
      goto packdone;
    }
    for(i=0; i<(int)typeData.size(); ++i) {
        typePtr[i] = typeData[i];
    }
    if (!intData.empty()) {
      longPtr=(long *)sipCpp->getTransferField( intString, AR_LONG, osize );
      if (!longPtr) {
        PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
        sipIsErr = 1;
        goto packdone;
      }
      for(i=0; i<(int)intData.size(); ++i) {
          longPtr[i] = intData[i];
      }
    }
    if (!floatData.empty()) {
      doublePtr=(double *)sipCpp->getTransferField( floatString, AR_DOUBLE, osize );
      if (!doublePtr) {
        PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
        sipIsErr = 1;
        goto packdone;
      }
      for(i=0; i<(int)floatData.size(); ++i) {
          doublePtr[i] = floatData[i];
      }
    }
    if (totalStringSize > 0) {
      stringSizePtr=(int *)sipCpp->getTransferField( charSizeString, AR_INT, osize );
      if (!stringSizePtr) {
        PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
        sipIsErr = 1;
        goto packdone;
      }
      for(i=0; i<(int)stringSizeData.size(); ++i) {
          stringSizePtr[i] = stringSizeData[i];
      }
      charPtr=(char *)sipCpp->getTransferField( charString, AR_CHAR, osize );
      if (!charPtr) {
        PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
        sipIsErr = 1;
        goto packdone;
      }
      for(i=0; i<(int)stringData.size(); ++i) {
        memcpy( charPtr, stringData[i], stringSizeData[i] );
  /*      cerr << "master: " << stringSizeData[i] << ": ";*/
  /*      for (j=0; j<stringSizeData[i]; ++j) {*/
  /*        cerr << *(charPtr+j);*/
  /*      }*/
  /*      cerr << endl;*/
        charPtr += stringSizeData[i];
      }
    }
packdone:
%End

  SIP_PYTUPLE getSequence( const string& a0 );
%MethodCode
    int typeSize;
    int longSize;
    int* typePtr;
    long* intDataPtr;
    int doubleSize;
    double* floatDataPtr;
    int charSize;
    char* charPtr;
    int charSizeSize;
    int* charSizePtr;

    typePtr=(int *)sipCpp->getTransferField( *a0+string("_TYPES"), AR_INT, typeSize );
    if (!typePtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      sipIsErr = 1;
      goto unpackdone;
    }
    intDataPtr=(long *)sipCpp->getTransferField( *a0+string("_INTDATA"), AR_LONG, longSize );
    if (!intDataPtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      sipIsErr = 1;
      goto unpackdone;
    }
    floatDataPtr=(double *)sipCpp->getTransferField( *a0+string("_FLOATDATA"), AR_DOUBLE, doubleSize );
    if (!floatDataPtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      sipIsErr = 1;
      goto unpackdone;
    }
    charPtr=(char *)sipCpp->getTransferField( *a0+string("_CHARDATA"), AR_CHAR, charSize );
    if (!charPtr) {
      PyErr_SetString(PyExc_MemoryError, "unable to get transfer field pointer");
      sipIsErr = 1;
      goto unpackdone;
    }
    charSizePtr=(int *)sipCpp->getTransferField( *a0+string("_CHARSIZEDATA"), AR_INT, charSizeSize );
    if (!charSizePtr) {
  /*cerr << *a0+string("_CHARSIZEDATA") << endl;*/
      PyErr_SetString(PyExc_MemoryError, "unable to get charSize transfer field pointer");
      sipIsErr = 1;
      goto unpackdone;
    }
    sipRes = _msUnpackSequenceData( &typePtr, &intDataPtr, &floatDataPtr, &charPtr, &charSizePtr );
    if (!sipRes) {
      cerr << "unpack sizes: " << typeSize << ", " << longSize << ", " << doubleSize << ", " << charSize << endl;
    }
unpackdone:
%End

  // Tiny functions that only appear in the .h

  // msec since the first I/O poll (not quite start of the program).
  double getTime() const;

  // msec taken to compute/draw the last frame.
  double getLastFrameTime() const;

  // Is this instance the master?
  bool getMaster() const;

  // Is this instance connected to the master?
  bool getConnected() const;

  // Is sound enabled?
  bool soundActive() const;

  // Is the input device enabled?
  bool inputActive() const;

  // Shared random numbers.  Set the seed on the master,
  // then make sure a data exchange occurs, then generate random numbers
  // by calling randUniformFloat() identically on all machines.
  void setRandomSeed( const long newSeed );
  bool randUniformFloat( float& value /Out/ );

  SIP_PYTUPLE getGUIXMLWindowConstructs();
%MethodCode
    vector< arGUIXMLWindowConstruct* >* constructs = sipCpp->getGUIXMLWindowConstructs();
    sipRes = PyTuple_New( constructs->size() );
    if (sipRes == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "unable to allocate tuple for arMasterSlaveFramework.getGUIXMLWindowConstructs()");
      sipIsErr = 1;
    } else {
      int i=0;
      vector< arGUIXMLWindowConstruct* >::iterator iter;
      for (iter=constructs->begin(); iter!=constructs->end(); ++iter) {
        PyObject* o = sipConvertFromInstance( *iter, sipClass_arGUIXMLWindowConstruct, Py_None );
        if (o==NULL) {
          Py_DECREF( sipRes );
          PyErr_SetString( PyExc_TypeError, "arGUIXMLWindowConstruct.getGUIXMLWindowConstructs() failed to convert result" );
          sipIsErr = true;
          break;
        }
        PyTuple_SetItem( sipRes, i++, o );
      }
    }
%End

 protected:
  void _stop(const char*, const arCallbackException&);

 private:
  arMasterSlaveFramework( const arMasterSlaveFramework& );
};


// By having these default callbacks, we can avoid making PySZG depend on PyOpenGL.
void ar_defaultWindowInitCallback();
void ar_defaultReshapeCallback( int width, int height );
void ar_defaultDisconnectDraw();


class arInteractableThing : arInteractable {
%TypeHeaderCode
#include "arInteractableThing.h"
%End
  public:
    arInteractableThing();
    arInteractableThing( const arInteractableThing& x );
    virtual ~arInteractableThing();
    virtual void setTexture( arTexture* tex );
    virtual arTexture* getTexture();
    virtual void setHighlight( bool flag );
    virtual bool getHighlight() const;
    virtual void setColor( float r, float g, float b, float a=1. );
    virtual void setColor( const arVector4& col );
    virtual void setColor( const arVector3& col );
    virtual void setAlpha( float a );
    virtual float getAlpha();
    virtual arVector4 getColor() const;
    virtual void setVisible( bool vis );
    virtual bool getVisible() const;
    virtual void activateColor() const;
    virtual bool activateTexture();
    virtual void deactivateTexture();
    virtual void draw();
    virtual void draw( arMasterSlaveFramework&, arGraphicsWindow&, arViewport& );
};

/*
class arDistSceneGraphFramework : arSZGAppFramework {
%TypeHeaderCode
#include "arDistSceneGraphFramework.h"
%End
 public:
  arDistSceneGraphFramework();
  ~arDistSceneGraphFramework();
                                       
  // Initializes the various objects but does not start the event loop.
  // bool init( int&, char** );
  bool init( vector<string> a0 );
  // a0 and a1 in MethoCode are _pointers_ to the respective arguments!!!
%MethodCode
    sipRes = _framework_init( sipCpp, *a0 );
%End

  bool start();
  void stop(bool);
  bool createWindows(bool useWindowing);
  void loopQuantum();
  void exitFunction();
  
  arGraphicsDatabase* getDatabase();
  void setDataBundlePath(const string& bundlePathName, 
                         const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, 
                            const string& bundlePath);
  void setAutoBufferSwap(bool);
  void swapBuffers();
  const string getNavNodeName() const;
  arDatabaseNode* getNavNode();
  
  void loadNavMatrix();
  void setViewer();
  void setPlayer();
};
*/
