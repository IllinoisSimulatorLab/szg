//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MASTER_SLAVE_FRAMEWORK
#define AR_MASTER_SLAVE_FRAMEWORK

#include "arDataServer.h"
#include "arDataClient.h"
#include "arSoundAPI.h"
#include "arBarrierClient.h"
#include "arBarrierServer.h"
#include "arDataUtilities.h"
#include "arGraphicsAPI.h"
#include "arGraphicsWindow.h"
#include "arPerspectiveCamera.h"
#include "arSoundClient.h"
#include "arSZGAppFramework.h"
#include "arVRCamera.h"
#include "arHeadWandSimulator.h"
#include "arFramerateGraph.h"
#include "arMasterSlaveDataRouter.h"
#include <vector>

/// Helper for arMasterSlaveFramework.
class arTransferFieldDescriptor{
 public:
  arTransferFieldDescriptor(arDataType t=AR_GARBAGE, void* d=NULL, int s=-1):
    type(t),
    data(d),
    size(s)
    {}
  arDataType type;
  void* data;
  int size;
};

typedef map<string,arTransferFieldDescriptor,less<string> > 
  arTransferFieldData;

/// Framework for cluster applications using one master and several slaves.
class SZG_CALL arMasterSlaveFramework : public arSZGAppFramework {
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_masterSlaveFrameworkConnectionTask(void*);
  friend void ar_masterSlaveFrameworkMessageTask(void*);
  friend void ar_masterSlaveFrameworkDisplayFunction();
  friend void ar_masterSlaveFrameworkReshapeFunction(int,int);
  friend void ar_masterSlaveFrameworkButtonFunction(int, int, int, int);
  friend void ar_masterSlaveFrameworkMouseFunction(int, int);
  friend void ar_masterSlaveFrameworkKeyboardFunction(unsigned char, int, int);
  friend class arMasterSlaveWindowInitCallback;
  friend class arMasterSlaveRenderCallback;
 public:
  arMasterSlaveFramework();
  virtual ~arMasterSlaveFramework();

  // set the callbacks
  void setStartCallback(bool (*startCallback)(arMasterSlaveFramework& fw, 
                                            arSZGClient&));
  void setPreExchangeCallback(void (*preExchange)(arMasterSlaveFramework&));
  void setPostExchangeCallback(void (*postExchange)(arMasterSlaveFramework&));
  void setWindowCallback(void (*windowCallback)(arMasterSlaveFramework&));
  void setDrawCallback(void (*draw)(arMasterSlaveFramework&));
  void setPlayCallback(void (*play)(arMasterSlaveFramework&));
  void setReshapeCallback(void (*reshape)(arMasterSlaveFramework&, int, int));
  void setExitCallback(void (*cleanup)(arMasterSlaveFramework&));
  void setUserMessageCallback(
    void (*userMessageCallback)( arMasterSlaveFramework&, const string& messageBody ));
  void setOverlayCallback(void (*overlay)(arMasterSlaveFramework&));
  void setKeyboardCallback(void (*keyboard)(arMasterSlaveFramework&,
                                            unsigned char, int, int));

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

  void setBundlePtr(const string& bundlePathName, const string& bundleName);

  virtual void preDraw();
  void drawWindow();
  void postDraw();

  arMatrix4 getProjectionMatrix(float eyeSign); // needed for custom stuff
  arMatrix4 getModelviewMatrix(float eyeSign);  // needed for custom stuff
  // must be able to use a custom screen objects for those rare cases
  // (for instance noneuclidean visualization) where the standard euclidean
  // cameras are no good
  void setWindowCamera( arVRCamera* cam ) {
    if (cam) {
      // Very important to set the head here, since this is, in fact,
      // used by the camera!
      cam->setHead(&_head);
      _graphicsWindow.setCamera(cam);
    } else {
      _graphicsWindow.setCamera(&_defaultCamera);
    }
  }
  bool setViewportCamera( unsigned int vpindex, arCamera* cam ) {
    if (cam) {
      return _graphicsWindow.setViewportCamera( vpindex, cam);
    } else {
      return _graphicsWindow.setViewportCamera( vpindex, &_defaultCamera);
    }
  }
  // the user application might need to adjust the cameras frame-by-frame
  std::vector<arViewport>* getViewports(){ 
    return _graphicsWindow.getViewports();
  }
  int             getWindowSizeX() const { return _windowSizeX; }
  int             getWindowSizeY() const { return _windowSizeY; }

  // Various methods of sharing data from the master to the slaves...

  // The simplest way to share data from master to slaves. Global variables
  // are registered with the framework (the transfer fields). Each frame,
  // the master dumps its values and sends them to the slaves. Note:
  // each field must be of a fixed size.
  bool addTransferField( string fieldName, void* data, 
                         arDataType dataType, int size );
  // Another way to share data. Here, the framework manages the memory
  // blocks. This allows them to be resized.
  // Add an internally-allocated transfer field. These can be resized on the
  // master and the slaves will automatically follow suit.
  bool addInternalTransferField( string fieldName, 
                                 arDataType dataType, 
                                 int size );
  bool setInternalTransferFieldSize( string fieldName, 
                                     arDataType dataType, 
                                     int newSize );
  // Get a pointer to either an externally- or an internally-stored 
  // transfer field. Note that the internally stored fields are resizable.
  void* getTransferField( string fieldName, 
                          arDataType dataType,
                          int& size );

  // A final way to share data from the master to the slaves. In this case,
  // the programmer registers arFrameworkObjects with the framework's
  // internal arMasterSlaveDataRouter. An identical set of arFrameworkObjects
  // is created by each of the master and slave instances. The
  // arMasterSlaveDataRouter routes messages between them.
  bool registerFrameworkObject(arFrameworkObject* object){
    return _dataRouter.registerFrameworkObject(object);
  }

  void setPlayTransform();
  void draw();

  // tiny functions that only appear in the .h

  // If set to true (the default), the framework will swap the graphics
  // buffers itself. Otherwise, the application will do so. This is
  // really a bit of a hack.
  void internalBufferSwap(bool state){ _internalBufferSwap = state; }
  void loadNavMatrix() { arMatrix4 temp = ar_getNavInvMatrix();
                         glMultMatrixf( temp.v ); }
  bool setViewMode( const string& viewMode ); 
  float getCurrentEye() const { return _graphicsWindow.getCurrentEyeSign(); }
  /// msec since the first I/O poll (not quite start of the program).
  double getTime() const { return _time; }
  /// How many msec it took to compute/draw the last frame.
  double getLastFrameTime() const { return _lastFrameTime; }
  bool getMaster() const { return _master; }
  bool getConnected() const { return _master || _stateClientConnected; }
  int getNumberSlavesConnected() const;
  bool soundActive() const { return _soundActive; }
  bool inputActive() const { return _inputActive; }

 protected:
  arDataServer*        _stateServer; // used only by master
  arDataClient         _stateClient;
  arGraphicsDatabase   _graphicsDatabase;
  arGraphicsScreen     _defaultScreen;
  arVRCamera           _defaultCamera;
  arSpeakerObject      _speakerObject;

  // Variables pertaining to the data transfer process.
  arTemplateDictionary    _transferLanguage;
  arDataTemplate          _transferTemplate;
  arStructuredData*       _transferData;
  arTransferFieldData     _transferFieldData;
  arTransferFieldData     _internalTransferFieldData;
  arMasterSlaveDataRouter _dataRouter;

  bool                 _userInitCalled;
  bool                 _parametersLoaded;

  arGraphicsWindow     _graphicsWindow;

  // need to store the networks on which we'll try to connect to services
  // as well as the name of the service we'll be using
  string _serviceName;
  string _serviceNameBarrier;
  string _networks;

  /// Callbacks.
  bool (*_startCallback)(arMasterSlaveFramework&, arSZGClient&);
  void (*_preExchange)(arMasterSlaveFramework&);
  void (*_postExchange)(arMasterSlaveFramework&);
  void (*_window)(arMasterSlaveFramework&);
  void (*_drawCallback)(arMasterSlaveFramework&);
  void (*_play)(arMasterSlaveFramework&);
  void (*_reshape)(arMasterSlaveFramework&, int, int);
  void (*_cleanup)(arMasterSlaveFramework&);
  void (*_userMessageCallback)(arMasterSlaveFramework&, const string&);
  void (*_overlay)(arMasterSlaveFramework&);
  void (*_keyboardCallback)(arMasterSlaveFramework&, unsigned char key,
			    int x, int y);

  int   _windowSizeX;
  int   _windowSizeY;
  int   _windowPositionX;
  int   _windowPositionY;
  bool  _stereoMode;
  bool  _internalBufferSwap;
  bool  _framerateThrottle;

  arBarrierServer* _barrierServer; //< used only by master
  arBarrierClient* _barrierClient; //< used only by slave
  arSignalObject   _swapSignal;

  // While we don't want to modify _master after start(), sometimes we
  // need to set it after the constructor.  So it's not declared const.
  bool    _master;
  bool    _stateClientConnected;	//< Used only if !_master.
  bool    _inputActive;	//< Set by master's _start().
  bool    _soundActive;	//< Set by master's _start().
  ARchar* _inBuffer;
  ARint   _inBufferSize;
  
  // Input-event stuff
  arInputEventQueue _eventQueue;
  arMutex _eventLock;

  // time variables
  ar_timeval _startTime;
  double     _hackStartTime;	//< only temporary, used for the glut's time
  double     _time;
  double     _lastFrameTime;
  double     _lastComputeTime;
  double     _lastSyncTime;
  bool       _firstTimePoll;
  
  string _texturePath;
  char   _textPath[256]; //< \todo fixed size buffer
  char   _inputIP[256]; //< \todo fixed size buffer

  arThread _connectionThread;
  arThread _soundThread;

  bool _screenshotFlag;
  int  _screenshotStartX;
  int  _screenshotStartY;
  int  _screenshotWidth;
  int  _screenshotHeight; 
  int  _whichScreenshot;

  // misc. configuration variables
  // Pausing the visualization.
  bool           _pauseFlag;
  arMutex        _pauseLock;
  arConditionVar _pauseVar;

  // Allow a different screen color (for lighting effects).
  arVector3 _defaultColor;

  // Information about the various threads that are unique to the
  // arMasterSlaveFramework (some info about the status of thread types
  // shared with other frameworks is in arSZGAppFramework.h)
  bool _connectionThreadRunning;
  bool _useGLUT;

  // Used in "standalone" mode
  string              _standaloneControlMode;
  arHeadWandSimulator _simulator;
  arFramerateGraph    _framerateGraph;
  bool                _showPerformance;

  // storage for the brokered port on which the master will
  // offer the state data.
  int  _masterPort[1];

  // in standalone mode, we produce sound locally
  arSoundClient* _soundClient;

  // small utility functions
  void _setMaster(bool);
  void _stop();
  bool _sync();

  // graphics utility functions
  void _createGLUTWindow();
  void _handleScreenshot();

  // data transfer functions
  bool _sendData();
  bool _getData();
  void _pollInputData();
  void _packInputData();
  void _unpackInputData();
  void _eventCallback( arInputEvent& event );

  // functions pertaining to initing/starting services
  bool _determineMaster(stringstream& initResponse);
  bool _initStandaloneObjects();
  bool _startStandaloneObjects();
  bool _initMasterObjects(stringstream& initResponse);
  bool _initSlaveObjects(stringstream& initResponse);
  bool _startMasterObjects(stringstream& startResponse);
  bool _startSlaveObjects(stringstream& startResponse);
  bool _startObjects();
  bool _startStandalone(bool);
  bool _start(bool);

  // systems level functions
  bool _loadParameters();
  void _messageTask();
  void _connectionTask();

  // draw-related utility functions
  virtual void _draw();
  void _display();
};

#endif
