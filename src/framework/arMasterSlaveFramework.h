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
  
  // Initializes the various objects but does not start the event loop.
  bool init( int&, char** );
  // Starts services, windowing, and an internal event loop. On success, does not return.
  bool start( void );
  // Starts services but not windowing. Returns and allows the user to control the event loop.
  bool startWithoutWindowing( void );
  // Starts services, windowing. Returns and allows the user to control the event loop,
  // either as a whole (via loopQuantum()) or in a fine-grained way via preDraw(), postDraw(), etc.
  bool startWithoutEventLoop( void );
  // Shut-down for much (BUT NOT ALL YET) of the arMasterSlaveFramework services.
  // if the parameter is set to true, we will block until the display thread
  // exits
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
  // Now those of us who prefer sub-classing to callbacks can do so.
  // (If you override these, the corresponding callbacks are of course
  // ignored).

  // Not really necessary to pass SZGClient, but convenient.
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
  void setUserMessageCallback( void (*userMessageCallback)( arMasterSlaveFramework&, const std::string& messageBody ) );
  void setOverlayCallback( void (*overlay)( arMasterSlaveFramework& ) );
  // Old-style keyboard callback.
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&, unsigned char, int, int ) );
  // New-style keyboard callback.
  void setKeyboardCallback( void (*keyboard)( arMasterSlaveFramework&, arGUIKeyInfo* ) );
  void setMouseCallback( void (*mouse)( arMasterSlaveFramework&, arGUIMouseInfo* ) );
  virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );

  void setDataBundlePath( const string& bundlePathName,
                          const string& bundleSubDirectory );
  void loadNavMatrix(void ) { arMatrix4 temp = ar_getNavInvMatrix();
			      glMultMatrixf( temp.v ); }
  void setPlayTransform( void );
  void drawGraphicsDatabase( void );
  void usePredeterminedHarmony();
  int getNumberSlavesExpected();
  bool allSlavesReady();
  // How many slaves are currently connected?
  int getNumberSlavesConnected( void ) const;
  

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

  // Tiny functions that only appear in the .h

  /// msec since the first I/O poll (not quite start of the program).
  double getTime( void ) const { return _time; }
  /// How many msec it took to compute/draw the last frame.
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

 protected:
  // Objects that provide the various services.
  // Used only by master.
  arDataServer*        _stateServer;    
  // Used only by slaves.
  arDataClient         _stateClient;
  arGraphicsDatabase   _graphicsDatabase;
  // Used only by master.
  arBarrierServer*     _barrierServer;  
  // Used only by slave.
  arBarrierClient*     _barrierClient;  
  // In standalone mode, we produce sound locally
  arSoundClient* _soundClient;
  
  // Threads launched by the framework.
  arThread _connectionThread;
  
  // Misc. variables follow.
  // Holds the head position for the spatialized sound API.
  arSpeakerObject      _speakerObject;
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
  bool    _inputActive;	            // Set by master's _start().
  bool    _soundActive;	            // Set by master's _start().
  // Storage for the arDataServer/arDataClient's messaging.
  ARchar* _inBuffer;
  ARint   _inBufferSize;
  // Variables pertaining to predetermined harmony mode.
  int  _numSlavesConnected;      // Updated only once/frame, before preExchange.
  bool _harmonyInUse;
  int  _harmonyReady;
  // For arGraphicsDatabase.
  std::string _texturePath;
  char   _textPath[ 256 ];          //< \todo fixed size buffer
  // For the input connection process.
  char   _inputIP[ 256 ];           //< \todo fixed size buffer
  // Information about the various threads that are unique to the
  // arMasterSlaveFramework (some info about the status of thread types
  // shared with other frameworks is in arSZGAppFramework.h)
  bool _connectionThreadRunning;
  bool _useWindowing;
  // Storage for the brokered port on which the master will
  // offer the state data.
  int  _masterPort[ 1 ];

  // Variables pertaining to the data transfer process.
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
  void (*_userMessageCallback)( arMasterSlaveFramework&, const std::string& );
  void (*_overlay)( arMasterSlaveFramework& );
  void (*_keyboardCallback)( arMasterSlaveFramework&, unsigned char key, int x, int y );
  void (*_arGUIKeyboardCallback)( arMasterSlaveFramework&, arGUIKeyInfo* );
  void (*_mouseCallback)( arMasterSlaveFramework&, arGUIMouseInfo* );

  // Input-event information.
  arInputEventQueue _inputEventQueue;

  // Time variables
  ar_timeval _startTime;
  double     _time;
  double     _lastFrameTime;
  double     _lastComputeTime;
  double     _lastSyncTime;
  bool       _firstTimePoll;

  // Shared random number functions, as might be used in predetermined harmony mode.
  int   _randSeedSet;
  long  _randomSeed;
  long  _newSeed;
  long  _numRandCalls;
  float _lastRandVal;
  int   _randSynchError;
  int   _firstTransfer;
  
  // Variable related to the "delay" message. Allows us to artificially slow down
  // the framerate.
  bool  _framerateThrottle;

  // Variables pertaining to the screenshot process. (in response to message)
  bool _screenshotFlag;
  int  _screenshotStartX;
  int  _screenshotStartY;
  int  _screenshotWidth;
  int  _screenshotHeight;
  int  _whichScreenshot;

  // Pausing the visualization. (in response to message)
  bool           _pauseFlag;
  arMutex        _pauseLock;
  arConditionVar _pauseVar;

  // Allow a different screen color (for lighting effects... i.e. when
  // taking photographs inside a CAVE-type environment it might
  // be desirable to have some of the walls that aren't pictured display
  // another color. (in response to the color message)
  arVector3 _noDrawFillColor;
  
  // It turns out that reloading parameters must occur in the main event
  // thread. This is because we allow the window manager to be single
  // threaded... and, in this case, all window manager calls should occur
  // in one thread only. (in response to the reload message)
  bool _requestReload;

  // Small utility functions.
  void _setMaster( bool );
  void _stop( void );
  bool _sync( void );

  // Graphics utility functions.
  bool _createWindowing( bool useWindowing );
  void _handleScreenshot( bool stereo );

  // Data transfer functions.
  bool _sendData( void );
  bool _getData( void );
  void _pollInputData( void );
  void _packInputData( void );
  void _unpackInputData( void );
  
  // Functions pertaining to initing/starting services.
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

  // Systems level functions.
  bool _loadParameters( void );
  void _messageTask( void );
  void _connectionTask( void );

  // Draw-related utility functions.
  void _drawWindow( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow );
};

#endif
