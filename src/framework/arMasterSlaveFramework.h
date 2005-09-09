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
#include "arSoundClient.h"
#include "arSZGAppFramework.h"
#include "arVRCamera.h"
#include "arInputSimulator.h"
#include "arFramerateGraph.h"
#include "arMasterSlaveDataRouter.h"
#include "arGUIWindowManager.h"
#include "arGUIInfo.h"
#include <vector>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arFrameworkCalling.h"

class arGUIXMLParser;

/// Helper for arMasterSlaveFramework.
class arTransferFieldDescriptor {
 public:
  arTransferFieldDescriptor( arDataType t = AR_GARBAGE, void* d = NULL, int s = -1 ):
    type( t ),
    data( d ),
    size( s )
    {}

  arDataType type;
  void* data;
  int size;
};

typedef map<std::string, arTransferFieldDescriptor > arTransferFieldData;

/// Framework for cluster applications using one master and several slaves.
class SZG_CALL arMasterSlaveFramework : public arSZGAppFramework {
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_masterSlaveFrameworkConnectionTask( void* );
  friend void ar_masterSlaveFrameworkMessageTask( void* );
  friend void ar_masterSlaveFrameworkWindowEventFunction( arGUIWindowInfo* );
  friend void ar_masterSlaveFrameworkWindowInitGLFunction( arGUIWindowInfo* );
  friend void ar_masterSlaveFrameworkKeyboardFunction( arGUIKeyInfo* );
  friend void ar_masterSlaveFrameworkMouseFunction( arGUIMouseInfo* );

  friend class arMasterSlaveWindowInitCallback;
  friend class arMasterSlaveRenderCallback;
 public:
  arMasterSlaveFramework( void );
  virtual ~arMasterSlaveFramework( void );

  bool setInputSimulator( arInputSimulator* sim );

  // We've added another layer of indirection. Now, at the point where
  // the callbacks were formerly called, we instead call these virtual
  // (hence overrideable) methods, which in turn call the callbacks.
  // Now those of us who prefer sub-classing to callbacks can do so.
  // (If you override these, the corresponding callbacks are of course
  // ignored).

  // not really necessary to pass SZGClient, but convenient.
  virtual bool onStart( arSZGClient& SZGClient );
  virtual void onWindowStartGL( arGUIWindowInfo* );
  virtual void onPreExchange( void );
  virtual void onPostExchange( void );
  virtual void onWindowInit( void );
  virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
  virtual void onDisconnectDraw( void );
  virtual void onPlay( void );
  virtual void onWindowEvent( arGUIWindowInfo* );
  virtual void onCleanup( void );
  virtual void onUserMessage( const std::string& messageBody );
  virtual void onOverlay( void );
  virtual void onKey( unsigned char key, int x, int y );
  virtual void onKey( arGUIKeyInfo* );
  virtual void onMouse( arGUIMouseInfo* );

  //
  // set the callbacks
  void setStartCallback( bool (*startCallback)( arMasterSlaveFramework& fw,
                                                arSZGClient& ) );
  void setWindowStartGLCallback( void (*windowStartGL)( arMasterSlaveFramework&,
                                                      arGUIWindowInfo* ) );
  void setPreExchangeCallback( void (*preExchange)( arMasterSlaveFramework& ) );
  void setPostExchangeCallback( void (*postExchange)( arMasterSlaveFramework& ) );
  void setWindowCallback( void (*windowCallback)( arMasterSlaveFramework& ) );
  void setDrawCallback( void (*draw)( arMasterSlaveFramework&, arGraphicsWindow&, arViewport& ) );
  void setDrawCallback( void (*draw)( arMasterSlaveFramework& ) );
  void setDisconnectDrawCallback( void (*disConnDraw)( arMasterSlaveFramework& ) );
  void setPlayCallback( void (*play)( arMasterSlaveFramework& ) );
  void setWindowEventCallback( void (*windowEvent)( arMasterSlaveFramework&,
                                                    arGUIWindowInfo* ) );
  void setExitCallback( void (*cleanup)( arMasterSlaveFramework& ) );
  void setUserMessageCallback( void (*userMessageCallback)( arMasterSlaveFramework&,
                                                            const std::string& messageBody ) );
  void setOverlayCallback( void (*overlay)( arMasterSlaveFramework& ) );
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&,
                                              unsigned char, int, int ) );
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&, arGUIKeyInfo* ) );
  void setMouseCallback( void (*mouse)( arMasterSlaveFramework&, arGUIMouseInfo* ) );
  void setSlaveConnectedCallback( void (*connectCallback)( arMasterSlaveFramework&, int ) );

  // initializes the various pieces but does not start the event loop
  bool init( int&, char** );

  // starts services and the default GLUT-based event loop
  bool start( void );
  // lets us work without the GLUT event loop... i.e. it starts the various
  // services but lets the user control the event loop
  bool startWithoutWindowing( void );

  // shut-down for much (BUT NOT ALL YET) of the arMasterSlaveFramework
  // if the parameter is set to true, we will block until the display thread
  // exits
  void stop( bool blockUntilDisplayExit );

  void usePredeterminedHarmony();
  bool allSlavesReady();

  void setDataBundlePath( const std::string& bundlePathName,
                          const std::string& bundleSubDirectory );

  virtual void preDraw( void );
  void draw( int windowID = -1 );
  void postDraw( void );

  void swap( int windowID = -1 );

  // Various methods of sharing data from the master to the slaves...

  // The simplest way to share data from master to slaves. Global variables
  // are registered with the framework (the transfer fields). Each frame,
  // the master dumps its values and sends them to the slaves. Note:
  // each field must be of a fixed size.
  bool addTransferField( std::string fieldName, void* data,
                         arDataType dataType, int size );
  // Another way to share data. Here, the framework manages the memory
  // blocks. This allows them to be resized.
  // Add an internally-allocated transfer field. These can be resized on the
  // master and the slaves will automatically follow suit.
  bool addInternalTransferField( std::string fieldName,
                                 arDataType dataType,
                                 int size );
  bool setInternalTransferFieldSize( std::string fieldName,
                                     arDataType dataType,
                                     int newSize );
  // Get a pointer to either an externally- or an internally-stored
  // transfer field. Note that the internally stored fields are resizable.
  void* getTransferField( std::string fieldName,
                          arDataType dataType,
                          int& size );

  // A final way to share data from the master to the slaves. In this case,
  // the programmer registers arFrameworkObjects with the framework's
  // internal arMasterSlaveDataRouter. An identical set of arFrameworkObjects
  // is created by each of the master and slave instances. The
  // arMasterSlaveDataRouter routes messages between them.
  bool registerFrameworkObject( arFrameworkObject* object ){
    return _dataRouter.registerFrameworkObject( object );
  }

  void setPlayTransform( void );

  // repurposed for arGUI
  // void draw( void );  // HEAVILY deprecated; use drawGraphicsDatabase.

  void drawGraphicsDatabase( void );

  // tiny functions that only appear in the .h

  // If set to true (the default), the framework will swap the graphics
  // buffers itself. Otherwise, the application will do so. This is
  // really a bit of a hack.
  void internalBufferSwap( bool state ){ _internalBufferSwap = state; }
  void loadNavMatrix(void ) { arMatrix4 temp = ar_getNavInvMatrix();
                              glMultMatrixf( temp.v ); }

  /// msec since the first I/O poll (not quite start of the program).
  double getTime( void ) const { return _time; }
  /// How many msec it took to compute/draw the last frame.
  double getLastFrameTime( void ) const { return _lastFrameTime; }
  bool getMaster( void ) const { return _master; }
  bool getConnected( void ) const { return _master || _stateClientConnected; }
  int getNumberSlavesConnected( void ) const;
  bool soundActive( void ) const { return _soundActive; }
  bool inputActive( void ) const { return _inputActive; }

  // Shared random numbers.  Set the seed on the master,
  // then make sure a data exchange occurs, then generate random numbers
  // by calling randUniformFloat() identically on all machines.
  void setRandomSeed( const long newSeed );
  bool randUniformFloat( float& value );

  // is this really a good thing to let the user access???
  arGUIWindowManager* getWindowManager( void ) { return _wm; }

  // technically, this function isn't necessary, one could just call
  // arMasterSlaveFramework::getWindowManager()->getGraphicsWindow() though
  // this function is a /tad/ more convenient
  arGraphicsWindow* getGraphicsWindow( const int windowID );
  void returnGraphicsWindow( const int windowID );

 protected:
  arDataServer*        _stateServer;        // used only by master
  arDataClient         _stateClient;
  arGraphicsDatabase   _graphicsDatabase;
  // arGraphicsScreen     _defaultScreen;
  // arVRCamera           _defaultCamera;
  arSpeakerObject      _speakerObject;

  // Variables pertaining to the data transfer process.
  arTemplateDictionary    _transferLanguage;
  arDataTemplate          _transferTemplate;
  arStructuredData*       _transferData;
  arTransferFieldData     _transferFieldData;
  arTransferFieldData     _internalTransferFieldData;
  arMasterSlaveDataRouter _dataRouter;

  // unsigned int         _glutDisplayMode;
  bool                 _userInitCalled;
  bool                 _parametersLoaded;

  // arGraphicsWindow     _graphicsWindow;
  arGUIWindowManager* _wm;
  // std::vector<int> _windows;
  arGUIXMLParser* _guiXMLParser;

  // need to store the networks on which we'll try to connect to services
  // as well as the name of the service we'll be using
  std::string _serviceName;
  std::string _serviceNameBarrier;
  std::string _networks;

  /// Callbacks.
  bool (*_startCallback)( arMasterSlaveFramework&, arSZGClient& );
  void (*_preExchange)( arMasterSlaveFramework& );
  void (*_postExchange)( arMasterSlaveFramework& );
  void (*_windowInitCallback)( arMasterSlaveFramework& );
  void (*_drawCallback)( arMasterSlaveFramework& fw, arGraphicsWindow& win, arViewport& vp );
  void (*_oldDrawCallback)( arMasterSlaveFramework& );
  void (*_disconnectDrawCallback)( arMasterSlaveFramework& );
  void (*_playCallback)( arMasterSlaveFramework& );
  void (*_windowEventCallback)( arMasterSlaveFramework&, arGUIWindowInfo* );
  void (*_windowStartGLCallback)( arMasterSlaveFramework&, arGUIWindowInfo* );
  void (*_cleanup)( arMasterSlaveFramework& );
  void (*_userMessageCallback)( arMasterSlaveFramework&, const std::string& );
  void (*_overlay)( arMasterSlaveFramework& );
  void (*_keyboardCallback)( arMasterSlaveFramework&, unsigned char key, int x, int y );
  void (*_arGUIKeyboardCallback)( arMasterSlaveFramework&, arGUIKeyInfo* );
  void (*_mouseCallback)( arMasterSlaveFramework&, arGUIMouseInfo* );

  bool  _internalBufferSwap;
  bool  _framerateThrottle;

  arBarrierServer* _barrierServer;  //< used only by master
  arBarrierClient* _barrierClient;  //< used only by slave
  arSignalObject   _swapSignal;

  // While we don't want to modify _master after start(), sometimes we
  // need to set it after the constructor.  So it's not declared const.
  bool    _master;
  bool    _stateClientConnected;	  //< Used only if !_master.
  bool    _inputActive;	            //< Set by master's _start().
  bool    _soundActive;	            //< Set by master's _start().
  ARchar* _inBuffer;
  ARint   _inBufferSize;
  int     _numSlavesConnected;      //< updated only once/frame, before preExchange

  bool _harmonyInUse;
  int _harmonyReady;

  // Input-event stuff
  arInputEventQueue _eventQueue;
  arMutex _eventLock;

  // time variables
  ar_timeval _startTime;
  double     _time;
  double     _lastFrameTime;
  double     _lastComputeTime;
  double     _lastSyncTime;
  bool       _firstTimePoll;

  int   _randSeedSet;
  long  _randomSeed;
  long  _newSeed;
  long  _numRandCalls;
  float _lastRandVal;
  int   _randSynchError;
  int   _firstTransfer;

  std::string _texturePath;
  char   _textPath[ 256 ];          //< \todo fixed size buffer
  char   _inputIP[ 256 ];           //< \todo fixed size buffer

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
  arVector3 _noDrawFillColor;

  // Information about the various threads that are unique to the
  // arMasterSlaveFramework (some info about the status of thread types
  // shared with other frameworks is in arSZGAppFramework.h)
  bool _connectionThreadRunning;
  bool _useWindowing;

  // Used in "standalone" mode
  std::string      _standaloneControlMode;
  arInputSimulator  _simulator;
  arInputSimulator* _simPtr;
  arFramerateGraph _framerateGraph;
  bool             _showPerformance;

  // Storage for the brokered port on which the master will
  // offer the state data.
  int  _masterPort[ 1 ];

  // In standalone mode, we produce sound locally
  arSoundClient* _soundClient;

  // It turns out that reloading parameters must occur in the main event
  // thread. This is because we allow the window manager to be single
  // threaded... and, in this case, all window manager calls should occur
  // in one thread only.
  bool _requestReload;

  // small utility functions
  void _setMaster( bool );
  void _stop( void );
  bool _sync( void );

  // graphics utility functions
  // void _createGLUTWindow( void );
  void _createWindowing( bool useWindowing );
  void _handleScreenshot( bool stereo );

  // data transfer functions
  bool _sendData( void );
  bool _getData( void );
  void _pollInputData( void );
  void _packInputData( void );
  void _unpackInputData( void );
  void _eventCallback( arInputEvent& event );

  bool _sendSlaveReadyMessage();
  
  // functions pertaining to initing/starting services
  bool _determineMaster( std::stringstream& initResponse );
  bool _initStandaloneObjects( void );
  bool _startStandaloneObjects( void );
  bool _initMasterObjects( std::stringstream& initResponse );
  bool _initSlaveObjects( std::stringstream& initResponse );
  bool _startMasterObjects( std::stringstream& startResponse );
  bool _startSlaveObjects( std::stringstream& startResponse );
  bool _startObjects( void );
  // bool _startStandalone( bool );
  bool _start( bool );
  bool _startrespond( const std::string& s );

  // systems level functions
  bool _loadParameters( void );
  void _messageTask( void );
  void _connectionTask( void );
  int _getNumberSlavesExpected();

  // draw-related utility functions
  void _drawWindow( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow );
};

#endif
