//********************************************************
// Syzygy is licensed under the BSD license v2
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
#include "arMasterSlaveDataRouter.h"
#include "arGUIInfo.h"
#include "arGUIXMLParser.h"
#include "arFrameworkCalling.h"

#include <vector>

// Helper for arMasterSlaveFramework.

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

typedef map<string, arTransferFieldDescriptor > arTransferFieldData;

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

  /**
   * @brief   Initializes the framework. Does NOT start the event loop.
   *
   * Pass in argc and argv from main().
   *
   * @return  true/false indicating success/failure. If false (usually
   * indicating the the szgserver specified in szg_<user>.conf--by
   * dlogin--could not be found), app must exit.
   */
  bool init( int& argc, char** argv );

  /**
   * @brief  Starts the application event loop.
   *
   * Calls the user-supplied startCallback or onStart() method,
   * creates windows, and starts the event loop. Note that onStart()
   * is called <i>before</i> window creation, so no OpenGL initialization
   * may happen there.
   *
   * @return false on failure to start, otherwise never returns. 
   */
  bool start();

  /**
   * @brief   Variant of start() that optionally does not create windows or start event loop.
   * @param[in]   useWindowing  bool, whether or not the framework should create windows.
   * @param[in]   useEventLoop  bool, whether or not the framework should start/manage the event loop.
   *
   * Calls the user-supplied onStart() method/start callback. This happens <i>before</i> window
   * creation, so no OpenGL initialization may happen there.
   *
   * Advanced: If useEventLoop is false, caller should run the event loop either
   * coarsely by calling loopQuantum() or finely using functions it calls, preDraw(), postDraw(), etc.
   *
   * @return false on failure to start <i>or</i> if useEventLoop is false,
   * otherwise never returns. 
   */
  bool start(bool useWindowing, bool useEventLoop);

  // Shutdown for many arMasterSlaveFramework services.
  // Optionally block until the display thread exits.
  void stop( bool blockUntilDisplayExit );

  bool createWindows(bool useWindowing);
  void loopQuantum();
  void exitFunction();
  virtual void preDraw( void );
  // Different than onDraw (and the corresponding draw callbacks).
  // Essentially causes the window manager to draw all the windows.
  void draw( int windowID = -1 );
  virtual void postDraw( void );
  void swap( int windowID = -1 );

  // Another layer of indirection to promote object-orientedness.
  // We MUST have callbacks to subclass this object in Python.
  // At each point the callbacks were formerly invoked, the code instead calls
  // these virtual (hence overrideable) methods, which in turn invoke the callbacks.
  // This allows subclassing instead of callbacks.
  // (If you override these, the corresponding callbacks are of course ignored).

  // Convenient options for arSZGClient.
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
  virtual void onUserMessage( const int messageID, const string& messageBody );
  virtual void onOverlay( void );
  virtual void onKey( unsigned char key, int x, int y );
  virtual void onKey( arGUIKeyInfo* );
  virtual void onMouse( arGUIMouseInfo* );

  // Set the callbacks
  void setStartCallback( bool (*startCallback)( arMasterSlaveFramework& fw, arSZGClient& ) );
  void setWindowStartGLCallback( void (*windowStartGL)( arMasterSlaveFramework&, arGUIWindowInfo* ) );
  void setPreExchangeCallback( void (*preExchange)( arMasterSlaveFramework& ) );
  void setPostExchangeCallback( void (*postExchange)( arMasterSlaveFramework& ) );
  void setWindowCallback( void (*windowCallback)( arMasterSlaveFramework& ) );
  // New-style draw callback.
  void setDrawCallback( void (*draw)( arMasterSlaveFramework&, arGraphicsWindow&, arViewport& ) );
  // Old-style draw callback.
  void setDrawCallback( void (*draw)( arMasterSlaveFramework& ) );
  void setDisconnectDrawCallback( void (*disConnDraw)( arMasterSlaveFramework& ) );
  void setPlayCallback( void (*play)( arMasterSlaveFramework& ) );
  void setWindowEventCallback( void (*windowEvent)( arMasterSlaveFramework&, arGUIWindowInfo* ) );
  void setExitCallback( void (*cleanup)( arMasterSlaveFramework& ) );
  void setUserMessageCallback( void (*userMessageCallback)( arMasterSlaveFramework&,
        const int messageID, const std::string& messageBody ) );
  void setUserMessageCallback( void (*userMessageCallback)( arMasterSlaveFramework&,
        const std::string& messageBody ) );
  void setOverlayCallback( void (*overlay)( arMasterSlaveFramework& ) );
  // Old-style keyboard callback.
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&, unsigned char, int, int ) );
  // New-style keyboard callback.
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&, arGUIKeyInfo* ) );
  void setMouseCallback( void (*mouse)( arMasterSlaveFramework&, arGUIMouseInfo* ) );
  virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );

  void setDataBundlePath( const string& bundlePathName,
                          const string& bundleSubDirectory );
  void loadNavMatrix(void );
  void setPlayTransform( void );
  void drawGraphicsDatabase( void );
  void usePredeterminedHarmony();
  int getNumberSlavesExpected();
  bool allSlavesReady();
  // How many slaves are currently connected?
  int getNumberSlavesConnected( void ) const;

  // How many slaves were involved in the last rendering sync?
  int getNumberSlavesSynced( void );

  bool sendMasterMessage( const string& messageBody, const string& messageType="user" );

  // Three ways to share data from the master to the slaves.

  // 1: simplest way.  Register global variables
  // with the framework (the transfer fields). Every frame,
  // the master dumps its values to the slaves.
  // Each field has a fixed size.
  bool addTransferField( const string& fieldName, void* data,
                         arDataType dataType, unsigned size );

  // 2: the framework manages the memory blocks, so they can resize.
  // Add an internally-allocated transfer field.
  // Resize them on the master, and the slaves automatically follow.
  bool addInternalTransferField( const string& fieldName,
                                 arDataType dataType,
                                 unsigned size );
  bool setInternalTransferFieldSize( const string& fieldName,
                                     arDataType dataType,
                                     unsigned newSize );
  // Get a pointer to an externally- or internally-stored
  // transfer field.  Internally stored fields are resizable.
  void* getTransferField( const string& fieldName,
                          arDataType dataType,
                          int& size );

  // 3: register arFrameworkObjects with the framework's
  // internal arMasterSlaveDataRouter. An identical set of arFrameworkObjects
  // is created by each of the master and slave instances. The
  // arMasterSlaveDataRouter routes messages between them.
  bool registerFrameworkObject( arFrameworkObject* object ) {
    return _dataRouter.registerFrameworkObject( object );
  }

  // Tiny functions that only appear in the .h

  // msec since the first I/O poll (not quite start of the program).
  double getTime( void ) const { return _time; }

  // msec taken to compute/draw the last frame.
  double getLastFrameTime( void ) const { return _lastFrameTime; }

  // Is this instance the master?
  bool getMaster( void ) const { return _master; }

  // Is this instance connected to the master?
  bool getConnected( void ) const { return _master || _stateClientConnected; }

  // Is sound enabled?
  bool soundActive( void ) const { return _soundActive; }

  // Is the input device enabled?
  bool inputActive( void ) const { return _inputActive; }

  // Shared random numbers.  Set the seed on the master,
  // then make sure a data exchange occurs, then generate random numbers
  // by calling randUniformFloat() identically on all machines.
  void setRandomSeed( const long newSeed );
  bool randUniformFloat( float& value );

  // Add entries to the data bundle path (used to locate texture maps by
  // szgrender for scene-graph apps in cluster mode and by SoundRender to
  // locate sounds for both types of apps in cluster mode).
  virtual void addDataBundlePathMap(const string& bundlePathName,
                          const string& bundlePath);

  std::vector< arGUIXMLWindowConstruct* >* getGUIXMLWindowConstructs() {
    return _guiXMLParser->getWindowingConstruct()->getWindowConstructs();
  }
  
 protected:
  // Objects that provide services.
  // Must be pointers, so languages can initialize.  Really?

  // Used only by master.
  arBarrierServer*     _barrierServer;
  arDataServer*        _stateServer;

  // Used only by slaves.
  arBarrierClient*     _barrierClient;
  arDataClient         _stateClient;
  arGraphicsDatabase   _graphicsDatabase;

  // In standalone mode, we produce sound locally
  arSoundClient* _soundClient;

  // Threads launched by the framework.
  arThread _connectionThread;

  // Misc. variables follow.
  // Holds the head position for the spatialized sound API.
  arSpeakerObject      _speakerObject;
  // sound navigation matrix ID
  int _soundNavMatrixID;
  // Need to store the networks on which we'll try to connect to services
  // as well as the name of the service we'll be using.
  std::string _serviceName;
  std::string _serviceNameBarrier;
  std::string _networks;
  // Used by (for instance) by the arBarrierServer to coordinate with the
  // arDataServer.
  arSignalObject _swapSignal;
  // While we don't want to modify _master after start(), sometimes we
  // need to set it after the constructor.  So it's not declared const.
  bool    _master;
  bool    _stateClientConnected;    // Used only if !_master.
  bool    _inputActive;                    // Set by master's start().
  bool    _soundActive;                    // Set by master's start().
  // Storage for the arDataServer/arDataClient's messaging.
  ARchar* _inBuffer;
  ARint   _inBufferSize;
  // Variables pertaining to predetermined harmony mode.
  int  _numSlavesConnected;      // Updated only once/frame, before preExchange.
  bool _harmonyInUse;
  int  _harmonyReady;
  int  _numSlavesSynced;      // Updated in postDraw()
  // For arGraphicsDatabase.
  std::string _texturePath;
  char   _textPath[ 256 ];          // bug: buffer overflow
  // For the input connection process.
  char   _inputIP[ 256 ];           // bug: buffer overflow
  // Information about the various threads that are unique to the
  // arMasterSlaveFramework (some info about the status of thread types
  // shared with other frameworks is in arSZGAppFramework.h)
  bool _connectionThreadRunning;
  bool _useWindowing;
  // Storage for the brokered port on which the master will
  // offer the state data.
  int  _masterPort[ 1 ];

  // Data transfer.
  arTemplateDictionary    _transferLanguage;
  arDataTemplate          _transferTemplate;
  arStructuredData*       _transferData;
  arTransferFieldData     _transferFieldData;
  arTransferFieldData     _internalTransferFieldData;
  arMasterSlaveDataRouter _dataRouter;

  // Callbacks.
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
  void (*_userMessageCallback)( arMasterSlaveFramework&,
      const int messageID, const std::string& messageBody );
  void (*_oldUserMessageCallback)( arMasterSlaveFramework&,
      const std::string& messageBody );
  void (*_overlay)( arMasterSlaveFramework& );
  void (*_keyboardCallback)( arMasterSlaveFramework&, unsigned char key, int x, int y );
  void (*_arGUIKeyboardCallback)( arMasterSlaveFramework&, arGUIKeyInfo* );
  void (*_mouseCallback)( arMasterSlaveFramework&, arGUIMouseInfo* );

  arInputEventQueue _inputEventQueue;

  // Time
  ar_timeval _startTime;
  double     _time;
  double     _lastFrameTime; // msec
  double     _lastComputeTime; // usec
  double     _lastSyncTime; // usec
  bool       _firstTimePoll;

  // Shared random number functions, as might be used in predetermined harmony mode.
  int   _randSeedSet;
  long  _randomSeed;
  long  _newSeed;
  long  _numRandCalls;
  float _lastRandVal;
  int   _randSynchError;
  int   _firstTransfer;

  // Related to the "delay" message. Artificial slowdown.
  bool  _framerateThrottle;

  // Respond to screenshot message.
  int _screenshotMsg;  // -1 iff no message to respond to, that requested a screenshot.
  bool _screenshotJPG; // pnm or jpg
  int  _whichScreenshot;

  // Respond to pause message.
  bool           _pauseFlag;
  arLock         _pauseLock; // with _pauseVar
  arConditionVar _pauseVar;

  // Allow a different screen color (for lighting effects... i.e. when
  // taking photographs inside a CAVE-type environment it might
  // be desirable to have some of the walls that aren't pictured display
  // another color. (in response to the color message)
  arVector3 _noDrawFillColor;

  // Reloading parameters must occur in the main event
  // thread, because we let the window manager be single
  // threaded, and because, in this case, all window manager calls
  // must occur in only one thread (in response to the reload message).
  int _requestReloadMsg;

  // Utilities.
  void _setMaster( bool );
  void _stop( void );
  bool _sync( void );

  // Graphics utilities.
  bool _createWindowing( bool useWindowing );
  void _handleScreenshot( bool stereo, int w, int h );

  // Data transfer.
  bool _sendData( void );
  bool _getData( void );
  void _pollInputData( void );
  void _packInputData( void );
  void _unpackInputData( void );

  // Initing/starting services.
  bool _determineMaster( void );
  bool _initStandaloneObjects( void );
  bool _startStandaloneObjects( void );
  bool _initMasterObjects( void );
  bool _initSlaveObjects( void );
  bool _startMasterObjects( void );
  bool _startInput( void );
  bool _startSlaveObjects( void );
  bool _startObjects( void );
  bool _start( bool useWindowing, bool useEventLoop );
  bool _startrespond( const std::string& s );

  // stop() when a callback has an error.
  void _stop(const char*, const arCallbackException&);

  // Systems level.
  bool _loadParameters( void );
  void _messageTask( void );
  void _connectionTask( void );

  void _processUserMessages();

  // Draw utility.
  void _drawWindow( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow );

 private:
  bool _addTransferField( const string&, void*, const arDataType, const int, arTransferFieldData&);
};

#endif
