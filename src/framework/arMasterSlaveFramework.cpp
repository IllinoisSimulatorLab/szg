//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#include "arSharedLib.h"
#include "arEventUtilities.h"
#include "arPForthFilter.h"
#include "arVRConstants.h"
#include "arGUIWindow.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arLogStream.h"
#include "arInputSimulatorFactory.h"

#ifndef AR_USE_WIN_32
#include <sys/types.h>
#include <signal.h>
#endif

//***********************************************************************
// arGUI callbacks
//***********************************************************************

void ar_masterSlaveFrameworkMessageTask( void* p ) {
  ((arMasterSlaveFramework*) p)->_messageTask();
}

void ar_masterSlaveFrameworkConnectionTask( void* p ) {
  ((arMasterSlaveFramework*) p)->_connectionTask();
}

void ar_masterSlaveFrameworkWindowEventFunction( arGUIWindowInfo* windowInfo ) {
  if( windowInfo && windowInfo->getUserData() ) {
    ((arMasterSlaveFramework*) windowInfo->getUserData())->onWindowEvent( windowInfo );
  }
}

void ar_masterSlaveFrameworkWindowInitGLFunction( arGUIWindowInfo* windowInfo ) {
  if( windowInfo && windowInfo->getUserData() ) {
    ((arMasterSlaveFramework*) windowInfo->getUserData())->onWindowStartGL( windowInfo );
  }
}

void ar_masterSlaveFrameworkKeyboardFunction( arGUIKeyInfo* keyInfo ) {
  if( !keyInfo || !keyInfo->getUserData() )
    return;

  arMasterSlaveFramework* fw = (arMasterSlaveFramework*) keyInfo->getUserData();
  if( fw->stopping() ) {
    // Shutting down.  Ignore keystrokes.
    return;
  }

  if( keyInfo->getState() == AR_KEY_DOWN ) {
    switch( keyInfo->getKey() ) {
      case AR_VK_ESC:
	// Don't block until the display thread is done.
	// Block on everything else.
	// Call exit() in the display thread rather than here.
	fw->stop( false );
	break;
      case AR_VK_f:
        fw->_wm->fullscreenWindow( keyInfo->getWindowID() );
	break;
      case AR_VK_F:
        fw->_wm->resizeWindow( keyInfo->getWindowID(), 600, 600 );
	break;
      case AR_VK_P:
        fw->_showPerformance = !fw->_showPerformance;
	break;
      case AR_VK_S:
        fw->_showSimulator = !fw->_showSimulator;
	break;
      case AR_VK_t:
        ar_log_critical() << "arMasterSlaveFramework frame time = " << fw->_lastFrameTime << " msec\n";
	break;
    }

    if( fw->_standalone &&
        fw->_standaloneControlMode == "simulator" ) {
      // Forward keyboard events to the interface too.
      fw->_simPtr->keyboard( keyInfo->getKey(), 1, 0, 0 );
    }
  }

  if( fw->getMaster() ) {
    // Forward keyboard events to the keyboard callback, if it's defined.
    fw->onKey( keyInfo );
  }
}

void ar_masterSlaveFrameworkMouseFunction( arGUIMouseInfo* mouseInfo ) {
  if( !mouseInfo || !mouseInfo->getUserData() )
    return;

  arMasterSlaveFramework* fw = (arMasterSlaveFramework*) mouseInfo->getUserData();
  if( fw->_standalone &&
      fw->_standaloneControlMode == "simulator" ) {
    if( mouseInfo->getState() == AR_MOUSE_DOWN || mouseInfo->getState() == AR_MOUSE_UP ) {
      const int whichButton =
        ( mouseInfo->getButton() == AR_LBUTTON ) ? 0 :
        ( mouseInfo->getButton() == AR_MBUTTON ) ? 1 :
        ( mouseInfo->getButton() == AR_RBUTTON ) ? 2 : 0; // Default to left button.
      const int whichState =
        ( mouseInfo->getState() == AR_MOUSE_DOWN ) ? 1 : 0;

      fw->_simPtr->mouseButton( whichButton, whichState, mouseInfo->getPosX(), mouseInfo->getPosY() );
    }
    else {
      fw->_simPtr->mousePosition( mouseInfo->getPosX(), mouseInfo->getPosY() );
    }
  }

  if( fw->getMaster() ) {
    fw->onMouse( mouseInfo );
  }
}

//***********************************************************************
// arGraphicsWindow callback classes
//***********************************************************************
class arMasterSlaveWindowInitCallback : public arWindowInitCallback {
  public:
    arMasterSlaveWindowInitCallback( arMasterSlaveFramework* fw ) :
       _framework( fw ) {}
    ~arMasterSlaveWindowInitCallback( void ) {}
    void operator()( arGraphicsWindow& );
  private:
    arMasterSlaveFramework* _framework;
};

void arMasterSlaveWindowInitCallback::operator()( arGraphicsWindow& ) {
  if( _framework ) {
    _framework->onWindowInit();
  }
}

//****************************************************************************
// Drawing the master/slave framework is complex. At the top level, the arGUI
// window manager (_wm) draws all of the application's arGUI windows, each of
// which is a seperate OS window. Each of these arGUI windows has an
// associated arGraphicsWindow, which contains enough information to draw
// the OpenGL inside the arGUI window. The arGraphicsWindow, in turn, holds
// a list of arViewports. The application draw callback that issues the OpenGL
// commands is called for each viewport (this occurs inside the
// arGraphicsWindow code).
//
// How are the callbacks registered?
// When the "graphics display" is parsed, the _wm creates a collection
// of arGUI windows (each of which has an arGraphicsWindow) and installs
// callbacks in them. The arGUI window
// has a draw callback (to occur once per window draw) and its
// arGraphicsWindow has a draw callback (to occur once per contained viewport
// and actually issue the OpenGL). Both of these callbacks are implemented
// as operators on an arGUIRenderCallback (which is a subclass of
// arRenderCallback). The arGUI window callback takes an arGUIWindowInfo*
// parameter and the arGraphicsWindow callback takes arGraphicsWindow
// and arViewport parameters (these give it enough information to perform
// view frustum culling, LOD, and other tricks).
//
// What is the call sequence?
// 1. From the event loop, the window manager requests that all (arGUI)
//    windows draw.
// 2. Each window (possibly in its own display thread) then calls
//    its arGUI render callback. In a master/slave app, this is actually
//    a call to arMasterSlaveRenderCallback::operator( arGUIWindowInfo* ).
// 3. arMasterSlaveFramework::_drawWindow(...) is called. It is responsible for
//    drawing the whole window and then blocking until all OpenGL commands
//    have written to the frame buffer.
//    It calls the arGraphicsWindow draw. It also draws various overlays
//    (like the performance graph or the inputsimulator).
// 4. Inside the arGraphicsWindow draw, for each owned viewport, the graphics window's
//    render callback is called. In an m/s app, this is a call to
//    arMasterSlaveRenderCallback::operator( arGraphicsWindow&, arViewport& ).
// 5. The arMasterSlaveRenderCallback calls the framework's (virtual) onDraw method.
// 6. If the onDraw method hasn't been over-ridden, then the application's
//    installed draw callback function is used.
//****************************************************************************

class arMasterSlaveRenderCallback : public arGUIRenderCallback {
  public:
    arMasterSlaveRenderCallback( arMasterSlaveFramework* fw ) :
      _framework( fw ) {}
    ~arMasterSlaveRenderCallback( void ) {}

    void operator()( arGraphicsWindow&, arViewport& );
    void operator()( arGUIWindowInfo* ) { }
    void operator()( arGUIWindowInfo* windowInfo,
                     arGraphicsWindow* graphicsWindow );

  private:
    arMasterSlaveFramework* _framework;
};

void arMasterSlaveRenderCallback::operator()( arGraphicsWindow& win, arViewport& vp ) {
  if( _framework ) {
    _framework->onDraw( win, vp );
  }
}

void arMasterSlaveRenderCallback::operator()( arGUIWindowInfo* windowInfo,
                                              arGraphicsWindow* graphicsWindow ) {
  if(_framework) {
    _framework->_drawWindow( windowInfo, graphicsWindow );
  }
}

//***********************************************************************
// arMasterSlaveFramework public methods
//***********************************************************************

arMasterSlaveFramework::arMasterSlaveFramework( void ):
  arSZGAppFramework(),

  // Services.
  _barrierServer( NULL ),
  _stateServer( NULL ),
  _barrierClient( NULL ),
  _soundClient( NULL ),

  // Miscellany.
  _serviceName( "NULL" ),
  _serviceNameBarrier( "NULL" ),
  _networks( "NULL" ),
  _master( true ),
  _stateClientConnected( false ),
  _inputActive( false ),
  _soundActive( false ),
  _inBuffer( NULL ),
  _inBufferSize( -1 ),
  _numSlavesConnected( 0 ),
  _harmonyInUse(false),
  _harmonyReady(0),
  _connectionThreadRunning( false ),
  _useWindowing( false ),

  // Data.
  _transferTemplate( "data" ),
  _transferData( NULL ),

  // Callbacks.
  _startCallback( NULL ),
  _preExchange( NULL ),
  _postExchange( NULL ),
  _windowInitCallback( NULL ),
  _drawCallback( NULL ),
  _oldDrawCallback( NULL ),
  _disconnectDrawCallback( NULL ),
  _playCallback( NULL ),
  _windowEventCallback( NULL ),
  _windowStartGLCallback( NULL ),
  _cleanup( NULL ),
  _userMessageCallback( NULL ),
  _overlay( NULL ),
  _keyboardCallback( NULL ),
  _arGUIKeyboardCallback( NULL ),
  _mouseCallback( NULL ),

  // Time.
  _time( 0.0 ),
  _lastFrameTime( 0.1 ),
  _firstTimePoll( true ),

  // Random numbers.
  _randSeedSet( 1 ),
  _randomSeed( (long) -1 ),
  _newSeed( (long) -1 ),
  _numRandCalls( (long) 0 ),
  _lastRandVal( 0.0 ),
  _randSynchError( 0 ),
  _firstTransfer( 1 ),

  // For user messages.
  _framerateThrottle( false ),
  _screenshotFlag( false ),
  _screenshotStartX( 0 ),
  _screenshotStartY( 0 ),
  _screenshotWidth( 640 ),
  _screenshotHeight( 480 ),
  _whichScreenshot( 0 ),
  _pauseFlag( false ),
  _noDrawFillColor( -1.0f, -1.0f, -1.0f ),
  _requestReload( false ) {

  // Where input events are buffered for transfer to slaves.
  _callbackFilter.saveEventQueue( true );

  // also need to add fields for our default-shared data
  _transferTemplate.addAttribute( "harmony_ready",   AR_INT );
  _transferTemplate.addAttribute( "time",            AR_DOUBLE );
  _transferTemplate.addAttribute( "lastFrameTime",   AR_DOUBLE );
  _transferTemplate.addAttribute( "navMatrix",       AR_FLOAT  );
  _transferTemplate.addAttribute( "signature",       AR_INT    );
  _transferTemplate.addAttribute( "types",           AR_INT    );
  _transferTemplate.addAttribute( "indices",         AR_INT    );
  _transferTemplate.addAttribute( "matrices",        AR_FLOAT  );
  _transferTemplate.addAttribute( "buttons",         AR_INT    );
  _transferTemplate.addAttribute( "axes",            AR_FLOAT  );
  _transferTemplate.addAttribute( "eye_spacing",     AR_FLOAT  );
  _transferTemplate.addAttribute( "mid_eye_offset",  AR_FLOAT  );
  _transferTemplate.addAttribute( "eye_direction",   AR_FLOAT  );
  _transferTemplate.addAttribute( "head_matrix",     AR_FLOAT  );
  _transferTemplate.addAttribute( "near_clip",       AR_FLOAT  );
  _transferTemplate.addAttribute( "far_clip",        AR_FLOAT  );
  _transferTemplate.addAttribute( "unit_conversion", AR_FLOAT  );
  _transferTemplate.addAttribute( "fixed_head_mode", AR_INT    );
  _transferTemplate.addAttribute( "szg_data_router", AR_CHAR   );
  _transferTemplate.addAttribute( "randSeedSet",     AR_INT    );
  _transferTemplate.addAttribute( "randSeed",        AR_LONG   );
  _transferTemplate.addAttribute( "numRandCalls",    AR_LONG   );
  _transferTemplate.addAttribute( "randVal",         AR_FLOAT  );

  ar_mutex_init( &_pauseLock );

  // when the default color is set like this, the app's geometry is displayed
  // instead of a default color
  _masterPort[ 0 ] = -1;

  // Initialize the performance graph.
  const arVector3 white  (1,1,1);
  const arVector3 yellow (1,1,0);
  const arVector3 cyan   (0,1,1);
  _framerateGraph.addElement( "framerate", 300, 100, white );
  _framerateGraph.addElement( "compute", 300, 100, yellow );
  _framerateGraph.addElement( "sync", 300, 100, cyan );
  _showPerformance = false;

  // _defaultCamera.setHead( &_head );
  // _graphicsWindow.setInitCallback( new arMasterSlaveWindowInitCallback( *this ) );
  // By default, the window manager is single-threaded.
  _wm = new arGUIWindowManager( ar_masterSlaveFrameworkWindowEventFunction,
                                ar_masterSlaveFrameworkKeyboardFunction,
                                ar_masterSlaveFrameworkMouseFunction,
                                ar_masterSlaveFrameworkWindowInitGLFunction,
                                false );

  // as a replacement for the __globalFramework pointer,
  // each window has a user data pointer set.
  _wm->setUserData( this );

  _guiXMLParser = new arGUIXMLParser( &_SZGClient );
}

// \bug memory leak for several pointer members
arMasterSlaveFramework::~arMasterSlaveFramework( void ) {
  // DO NOT DELETE _screenObject in here. WE MIGHT NOT OWN IT.

  arTransferFieldData::iterator iter;
  for( iter = _internalTransferFieldData.begin();
       iter != _internalTransferFieldData.end(); iter++) {
    if( iter->second.data ) {
      ar_deallocateBuffer( iter->second.data );
    }
  }

  _internalTransferFieldData.clear();

  if( !_master && _inputState ) {
    delete _inputState;
  }

  delete _wm;
  delete _guiXMLParser;
}

// Initializes the syzygy objects, but does not start any threads
bool arMasterSlaveFramework::init( int& argc, char** argv ) {
  _label = ar_stripExeName( string( argv[ 0 ] ) );
  
  // Connect to the szgserver.
  _SZGClient.simpleHandshaking( false );
  if (!_SZGClient.init( argc, argv )){
    return false;
  }
  
  if ( !_SZGClient ) {
    _standalone = true; // init() succeeded, but !_SZGClient says it's disconnected.
    _setMaster( true ); // Because standalone.
    
    // HACK!!!!! There's cutting-and-pasting here...
    dgSetGraphicsDatabase( &_graphicsDatabase );
    dsSetSoundDatabase( &_soundServer );
    
    // Before _loadParameters, so the internal sound client,
    // created in _initStandaloneObjects, can be configured for standalone.
    if( !_initStandaloneObjects() ) {
      // Standalone objects can fail to init,
      // e.g a missing joystick driver or a pforth syntax error.
      return false;
    }
    
    if( !_loadParameters() ) {
      ar_log_warning() << _label << " failed to load parameters while standalone.\n";
    }
    
    _parametersLoaded = true;
    
    // Don't start the message-receiving thread yet (because there's no
    // way yet to operate in a distributed fashion).
    // Don't initialize the master's objects, since they'll be unused.
    return true;
  }
  
  // Initialize a few things.
  dgSetGraphicsDatabase( &_graphicsDatabase );
  dsSetSoundDatabase( &_soundServer );
  
  // Load the parameters before executing any user callbacks
  // (since this determines if we're master or slave).
  if( !_loadParameters() ) {
    goto fail;
  }
  
  // Figure out whether we should launch the other executables.
  // If so, under certain circumstances, this function may not return.
  // NOTE: regardless of whether or not we'll be launching from here
  // in trigger mode, we want to be able to use the arAppLauncher object
  // to query info about the virtual computer, which requires the arSZGClient
  // be registered with the arAppLauncher
  (void)_launcher.setSZGClient( &_SZGClient );
  
  if( _SZGClient.getMode( "default" ) == "trigger" ) {
    // if we are the trigger node, we launch the rest of the application
    // components... and then WAIT for the exit.
    string vircomp = _SZGClient.getVirtualComputer();
    
    // we are, in fact, executing as part of a virtual computer
    _vircompExecution = true;
    
    const string defaultMode = _SZGClient.getMode( "default" );
    
    ar_log_remark() << _label << " virtual computer '" << vircomp <<
      "', default mode '" << defaultMode << "'.\n";
    
    _launcher.setAppType( "distapp" );
    
    // The render program is the (stripped) EXE name, plus parameters.
    {
      char* exeBuf = new char[ _label.length() + 1 ];
      ar_stringToBuffer( _label, exeBuf, _label.length() + 1 );
      char* temp = argv[ 0 ];
      argv[ 0 ] = exeBuf;
      
      _launcher.setRenderProgram( ar_packParameters( argc, argv ) );
      
      argv[ 0 ] = temp;
      delete exeBuf;
    }
    
    // Reorganize the virtual computer.
    if( !_launcher.launchApp() ) {
      ar_log_warning() << _label << " failed to launch on virtual computer "
                     << vircomp << ".\n";
      goto fail;
    }
    
    // Wait for the message (render nodes do this in the message task).
    if( !_SZGClient.sendInitResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }
    
    // there's no more starting to do... since this application instance
    // is just used to launch other instances
    ar_log_remark() << _label << " trigger launched components.\n";
    
    if( !_SZGClient.sendStartResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }
    (void)_launcher.waitForKill();
    exit( 0 );
  }
  
  // Mode isn't trigger.
  
  // So apps can query the virtual computer's attributes, 
  // those attributes are not restricted to the master.
  // So setParameters() is called. 
  // But we allow setting a bogus virtual computer via "-szg".
  // So it's not fatal if setParamters() fails, i.e. this should launch:
  //   my_program -szg virtual=bogus
  if( _SZGClient.getVirtualComputer() != "NULL" &&
      !_launcher.setParameters() ) {
    ar_log_warning() << _label << ": invalid virtual computer definition.\n";
  }
  
  // Launch the message thread, so dkill works even if init() or start() fail.
  // But launch after the trigger code above, lest we catch messages both
  // in waitForKill() AND in the message thread.
  if(!_messageThread.beginThread( ar_masterSlaveFrameworkMessageTask, this )){
    ar_log_warning() << _label << " failed to start message thread.\n";
    goto fail;
  }
  
  if( !_determineMaster() ) {
    goto fail;
  }
  
  // init the objects, either master or slave
  if( getMaster() ) {
    if( !_initMasterObjects() ) {
      goto fail;
    }
  }
  else {
    if( !_initSlaveObjects() ) {
fail:
      if( !_SZGClient.sendInitResponse( false ) ) {
        cerr << _label << ": maybe szgserver died.\n";
      }
      return false;
    }
  }
  _parametersLoaded = true;
  if( !_SZGClient.sendInitResponse( true ) ) {
    cerr << _label << ": maybe szgserver died.\n";
  }
  return true;
}

// Begins halting many significant parts of the object and blocks until
// they have, indeed, halted. The parameter will be false if we have been
// called from the arGUI keyboard function (that way be will not wait for
// the display thread to finish, resulting in a deadlock, since the
// keyboard function is called from that thread). The parameter will be
// true, on the other hand, if we are calling from the message-receiving
// thread (which is different from the display thread, and there the
// stop(...) should not return until the display thread is done.
// THIS DOES NOT HALT EVERYTHING YET! JUST THE
// STUFF THAT SEEMS TO CAUSE SEGFAULTS OR OTHER PROBLEMS ON EXIT.
void arMasterSlaveFramework::stop( bool blockUntilDisplayExit ) {
  _blockUntilDisplayExit = blockUntilDisplayExit;
  if( _vircompExecution)  {
    // arAppLauncher object piggy-backing on a renderer execution
    _launcher.killApp();
  }

  // To avoid a race condition, set _exitProgram within this lock.
  ar_mutex_lock( &_pauseLock );
  _exitProgram = true;
  _pauseFlag   = false;
  _pauseVar.signal();
  ar_mutex_unlock( &_pauseLock );
  
  // We can be a master with no distribution.
  if( getMaster() ) {
    if( !_standalone ) {
      _barrierServer->stop();
      _soundServer.stop();
    }
  }
  else {
    _barrierClient->stop();
  }
  
  while( _connectionThreadRunning ||
         ( _useWindowing && _displayThreadRunning && blockUntilDisplayExit ) ||
         ( _useExternalThread && _externalThreadRunning ) ) {
    ar_usleep( 100000 );
  }
  
  ar_log_remark() << _label << " done.\n";
  _stopped = true;
}

bool arMasterSlaveFramework::createWindows( bool useWindowing ) {
  std::vector< arGUIXMLWindowConstruct* >* windowConstructs = _guiXMLParser->getWindowingConstruct()->getWindowConstructs();
  
  if( !windowConstructs ) {
    // print error?
    return false;
  }
  
  // Populate callbacks for the gui and graphics windows,
  // and the head for any vr cameras.
  std::vector< arGUIXMLWindowConstruct*>::iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); itr++ ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arMasterSlaveWindowInitCallback( this ) );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arMasterSlaveRenderCallback( this ) );
    (*itr)->setGUIDrawCallback( new arMasterSlaveRenderCallback( this ) );
    
    std::vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    std::vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &_head );
      }
    }
  }
  
  // Create the windows if "using windowing", else just create placeholders.
  if( _wm->createWindows( _guiXMLParser->getWindowingConstruct(),
                          useWindowing ) < 0 ) {
    ar_log_warning() << _label << " failed to create windows.\n";
#ifdef AR_USE_DARWIN
    ar_log_warning() << "  (Check that X11 is running.)\n";
#endif	
    return false;
  }
  
  _wm->setAllTitles( _label, false );
  return true;
}

void arMasterSlaveFramework::loopQuantum(){
  // Exchange data. Connect slaves to master and activate framelock.
  preDraw();
  draw();
  postDraw();

  // Synchronize.
  swap();

  // Process keyboard events, events from the window manager, etc.
  _wm->processWindowEvents();
}

void arMasterSlaveFramework::exitFunction(){
  // Wildcat framelock is only deactivated if this makes sense;
  // and if so, do so in the display thread and before exiting.
  _wm->deactivateFramelock();

  // Framelock is now deactivated, so exit all windows.
  _wm->deleteAllWindows();
  // Guaranteed to get here--user-defined cleanup will be called
  // at the right time
  onCleanup();
  // If an exit is going to occur elsewehere (like in the message thread)
  // it can now happen safely at any time.
  _displayThreadRunning = false;
  // If stop(...) is called from the arGUI keyboard function, we
  // want to exit here. (_blockUntilDisplayExit will be false)
  // Otherwise, we want to wait here for the exit to occur in the
  // message thread.
  while( _blockUntilDisplayExit ){
    ar_usleep( 100000 );
  }
  // When this function has returned, we can exit.
}

// The sequence of events that should occur before the window is drawn.
// Made available as a public method so that applications can create custom
// event loops.
void arMasterSlaveFramework::preDraw( void ) {
  if (stopping())
    return;
  
  // Reload parameters in this thread,
  // since the arGUIWindowManager might be single threaded,
  // in which case all calls to it must be in that single thread.
  
  if (_requestReload){
    (void) _loadParameters();
    // Assume that recreating the windows will be OK. A reasonable assumption.
    (void) createWindows(_useWindowing);
    _requestReload = false;
  }
  
  const ar_timeval preDrawStart = ar_time();
  
  // Catch pause/ un-pause requests.
  ar_mutex_lock( &_pauseLock );
  while( _pauseFlag ) {
    _pauseVar.wait( &_pauseLock );
  }
  ar_mutex_unlock( &_pauseLock );
  
  // the pause might have been triggered by shutdown
  if (stopping())
    return;
  
  // Do this before the pre-exchange callback.
  // The user might want to use current input data via
  // arMasterSlaveFramework::getButton() or some other method in that callback.
  if( getMaster() ) {
    if (_harmonyInUse && !_harmonyReady) {
      _harmonyReady = allSlavesReady();
      if (_harmonyReady) {
        // All slaves are connected, so accept input in predetermined harmony.
        if (!_startInput()) {
          // What else should we do here?
          ar_log_warning() << _label << " _startInput() failed.\n";
        }
      }
    }
    
    _pollInputData();
    
    _inputEventQueue = _callbackFilter.getEventQueue();
    arInputEventQueue myQueue( _inputEventQueue );
    onProcessEventQueue( myQueue );
    
    if (!(_harmonyInUse && !_harmonyReady)) {
      onPreExchange();
    }
  }
  
  if( !_standalone ) {
    if( !( getMaster() ? _sendData() : _getData() ) ) {
      _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
      return;
    }
  }
  
  onPlay();
  
  if( getConnected() && !(_harmonyInUse && !_harmonyReady) ) {
    if (_harmonyInUse && !getMaster()) {
      // In pre-determined harmony mode, slaves get to process event queue.
      arInputEventQueue myQueue2( _inputEventQueue );
      onProcessEventQueue( myQueue2 );
    }
    onPostExchange();
  }
  
  if( _standalone && !_soundClient->silent()) {
    // Play the sounds locally.
    _soundClient->_cliSync.consume();
  }
  
  // Report current frametime to the framerate graph.
  // Bug: computed in _pollInputData, useless for slaves.
  arPerformanceElement* framerateElement = _framerateGraph.getElement( "framerate" );
  framerateElement->pushNewValue( 1000.0 / _lastFrameTime );
  
  // Compute time is in microseconds and the graph element is scaled to 100 ms.
  arPerformanceElement* computeElement   = _framerateGraph.getElement( "compute" );
  computeElement->pushNewValue( _lastComputeTime / 1000.0 );
  
  // Sync time is in microseconds and the graph element is scaled to 100 ms.
  arPerformanceElement* syncElement = _framerateGraph.getElement( "sync" );
  syncElement->pushNewValue(_lastSyncTime / 1000.0 );
  
  // Get performance metrics.
  _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
}

// Public method so that applications can make custom event loops.
void arMasterSlaveFramework::draw( int windowID ){
  if( stopping() || !_wm )
    return;

  if( windowID < 0 )
    _wm->drawAllWindows( true );
  else
    _wm->drawWindow( windowID, true );
}

// The sequence of events that should occur after the window is drawn,
// but before the synchronization is called.
void arMasterSlaveFramework::postDraw( void ){
  // if shutdown has been triggered, just return
  if (stopping())
    return;
  
  ar_timeval postDrawStart = ar_time();
  
  // sometimes, for testing of synchronization, we want to be able to throttle
  // down the framerate
  if( _framerateThrottle ) {
    ar_usleep( 200000 );
  }

  // Synchronize.
  if( !_standalone && !_sync() ) {
    ar_log_warning() << _label << ": sync failed.\n";
  }
  
  _lastSyncTime = ar_difftime( ar_time(), postDrawStart );
}

// Public method so that applications can make custom event loops.
void arMasterSlaveFramework::swap( int windowID ) {
  if (stopping() || !_wm)
    return;
  
  if( windowID < 0 )
    _wm->swapAllWindowBuffers(true);
  else
    _wm->swapWindowBuffer( windowID, true );
}

bool arMasterSlaveFramework::onStart( arSZGClient& SZGClient ) {
  if( _startCallback && !_startCallback( *this, SZGClient ) ) {
    ar_log_error() << _label << " user-defined start callback failed.\n";
    return false;
  }

  return true;
}

void arMasterSlaveFramework::onPreExchange( void ) {
  if ( _preExchange ) {
    try {
      _preExchange( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework preExchange callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onPostExchange( void ) {
  if( _postExchange ) {
    try {
      _postExchange( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework postExchange callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onWindowInit( void ) {
  if( _windowInitCallback ) {
    try {
      _windowInitCallback( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework windowInit callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
  else {
    ar_defaultWindowInitCallback();
  }
}

// Only events that are actually generated by the GUI itself actually reach
// here, not for instance events that we post to the window manager ourselves.
void arMasterSlaveFramework::onWindowEvent( arGUIWindowInfo* windowInfo ) {
  if( windowInfo ) {
    if (_windowEventCallback ) {
      try {
        _windowEventCallback( *this, windowInfo );
      } catch (arMSCallbackException exc) {
        ar_log_error() << "The following error occurred in the arMasterSlaveFramework windowEvent callback:\n\t"
             << exc.message << ar_endl;
        stop(false);
      }
    } else if ( windowInfo->getUserData() ) {
      // default window event handler, at least to handle a resizing event
      // (should we be handling window close events as well?)
      arMasterSlaveFramework* fw = (arMasterSlaveFramework*) windowInfo->getUserData();

      if( !fw ) {
        return;
      }

      switch( windowInfo->getState() ) {
        case AR_WINDOW_FULLSCREEN:
          break;
        case AR_WINDOW_RESIZE:
          fw->_wm->setWindowViewport( windowInfo->getWindowID(), 0, 0,
                                      windowInfo->getSizeX(), windowInfo->getSizeY() );
          break;

        case AR_WINDOW_CLOSE:
        // We will only get here if someone clicks the window close decoration.
        // This is NOT reached if we use the arGUIWindowManager's delete
        // method.
          fw->stop( false );
          break;

        default:
          break;
      }
    }
  }
}

void arMasterSlaveFramework::onWindowStartGL( arGUIWindowInfo* windowInfo ) {
  if( windowInfo && _windowStartGLCallback ) {
    try {
      _windowStartGLCallback( *this, windowInfo );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework windowStartGL callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

// Yes, this is really the application-provided draw function. It is
// called once per viewport of each arGraphicsWindow (arGUIWindow).
// It is a virtual method that issues the user-defined draw callback.
void arMasterSlaveFramework::onDraw( arGraphicsWindow& win, arViewport& vp ) {
  if( (!_oldDrawCallback) && (!_drawCallback) ) {
    ar_log_warning() << _label << " warning: forgot to setDrawCallback().\n";
    return;
  }

  if (_drawCallback) {
    try {
      _drawCallback( *this, win, vp );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework draw callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
    return;
  }

  try {
    _oldDrawCallback( *this );
  } catch (arMSCallbackException exc) {
    ar_log_error() << "The following error occurred in the arMasterSlaveFramework draw callback:\n\t"
         << exc.message << ar_endl;
    stop(false);
  }
}

void arMasterSlaveFramework::onDisconnectDraw( void ) {
  if( _disconnectDrawCallback ) {
    try {
      _disconnectDrawCallback( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework disconnectDraw callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
  else {
    // just draw a black background
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -1.0, 1.0, -1.0, 1.0, 0.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glPushAttrib( GL_ALL_ATTRIB_BITS );
    glDisable( GL_LIGHTING );
    glColor3f( 0.0f, 0.0f, 0.0f );
    glBegin( GL_QUADS );
    glVertex3f(  1.0f,  1.0f, -0.5f );
    glVertex3f( -1.0f,  1.0f, -0.5f );
    glVertex3f( -1.0f, -1.0f, -0.5f );
    glVertex3f(  1.0f, -1.0f, -0.5f );
    glEnd();
    glPopAttrib();
  }
}

void arMasterSlaveFramework::onPlay( void ) {
  if( _playCallback ) {
    setPlayTransform();
    try {
      _playCallback( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework play callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onCleanup( void ) {
  if( _cleanup ) {
    try {
      _cleanup( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework cleanup callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onUserMessage( const string& messageBody ) {
  if( _userMessageCallback ) {
    try {
      _userMessageCallback( *this, messageBody );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework userMessage callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onOverlay( void ) {
  if( _overlay ) {
    try {
      _overlay( *this );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework overlay callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onKey( arGUIKeyInfo* keyInfo ) {
  if( !keyInfo ) {
    return;
  }

  // If the 'newer' keyboard callback type is registered, use it instead of
  // the legacy version.
  if( _arGUIKeyboardCallback ) {
    try {
      _arGUIKeyboardCallback( *this, keyInfo );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework keyboard callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  } else if( keyInfo->getState() == AR_KEY_DOWN ) {
    // For legacy reasons, this call expects only key press
    // (not release) events.
    onKey( keyInfo->getKey(), 0, 0 );
  }
}

void arMasterSlaveFramework::onKey( unsigned char key, int x, int y) {
  if( _keyboardCallback ) {
    try {
      _keyboardCallback( *this, key, x, y );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework keyboard callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onMouse( arGUIMouseInfo* mouseInfo ) {
  if( mouseInfo && _mouseCallback ) {
    try {
      _mouseCallback( *this, mouseInfo );
    } catch (arMSCallbackException exc) {
      ar_log_error() << "The following error occurred in the arMasterSlaveFramework mouse callback:\n\t"
           << exc.message << ar_endl;
      stop(false);
    }
  }
}

void arMasterSlaveFramework::setStartCallback
  ( bool (*startCallback)( arMasterSlaveFramework&, arSZGClient& ) ) {
  _startCallback = startCallback;
}

void arMasterSlaveFramework::setPreExchangeCallback
  ( void (*preExchangeCallback)( arMasterSlaveFramework& ) ) {
  _preExchange = preExchangeCallback;
}

void arMasterSlaveFramework::setPostExchangeCallback
  ( void (*postExchange)( arMasterSlaveFramework& ) ) {
  _postExchange = postExchange;
}

// The window callback is called once per window, per frame, if set.
// If it is not set, the _drawSetUp method is called instead. This callback
// is needed since we may want several different views in a single window,
// with a draw callback filling each. This is especially useful for passive
// stereo from a single box, but will not work if the draw callback includes
// code that clears the entire buffer. Hence such code needs to moved
// into this sort of callback.
void arMasterSlaveFramework::setWindowCallback
  ( void (*windowCallback)( arMasterSlaveFramework& ) ) {
  _windowInitCallback = windowCallback;
}

void arMasterSlaveFramework::setWindowEventCallback
  ( void (*windowEvent)( arMasterSlaveFramework&, arGUIWindowInfo* ) ) {
  _windowEventCallback = windowEvent;
}

void arMasterSlaveFramework::setWindowStartGLCallback
  ( void (*windowStartGL)( arMasterSlaveFramework&, arGUIWindowInfo* ) ) {
  _windowStartGLCallback = windowStartGL;
}

void arMasterSlaveFramework::setDrawCallback(
              void (*draw)( arMasterSlaveFramework&, arGraphicsWindow&, arViewport& ) ) {
  ar_log_remark() << _label << " set draw callback.\n";
  _drawCallback = draw;
}

void arMasterSlaveFramework::setDrawCallback( void (*draw)( arMasterSlaveFramework& ) ) {
  ar_log_remark() << _label << " set old-style draw callback.\n";
  _oldDrawCallback = draw;
}

void arMasterSlaveFramework::setDisconnectDrawCallback( void (*disConnDraw)( arMasterSlaveFramework& ) ) {
  _disconnectDrawCallback = disConnDraw;
}

void arMasterSlaveFramework::setPlayCallback
  ( void (*play)( arMasterSlaveFramework& ) ) {
  _playCallback = play;
}

void arMasterSlaveFramework::setExitCallback
  ( void (*cleanup)( arMasterSlaveFramework& ) ) {
  _cleanup = cleanup;
}

// Syzygy messages currently consist of two strings, the first being
// a type and the second being a value. The user can send messages
// to the arMasterSlaveFramework and the application can trap them
// using this callback. A message w/ type "user" and value "foo" will
// be passed into this callback, if set, with "foo" going into the string.
void arMasterSlaveFramework::setUserMessageCallback
  ( void (*userMessageCallback)(arMasterSlaveFramework&, const string& )){
  _userMessageCallback = userMessageCallback;
}

// In general, the graphics window can be a complicated sequence of
// viewports (as is necessary for simulating a CAVE or Cube). Sometimes
// the application just wants to draw something once, which is the
// purpose of this callback.
void arMasterSlaveFramework::setOverlayCallback
  ( void (*overlay)( arMasterSlaveFramework& ) ) {
  _overlay = overlay;
}

// A master instance will also take keyboard input via this callback,
// if defined.
void arMasterSlaveFramework::setKeyboardCallback
  ( void (*keyboard)( arMasterSlaveFramework&, unsigned char, int, int ) ) {
  _keyboardCallback = keyboard;
}

void arMasterSlaveFramework::setKeyboardCallback
  ( void (*keyboard)( arMasterSlaveFramework&, arGUIKeyInfo* ) ) {
  _arGUIKeyboardCallback = keyboard;
}

void arMasterSlaveFramework::setMouseCallback
  ( void (*mouse)( arMasterSlaveFramework&, arGUIMouseInfo* ) ) {
  _mouseCallback = mouse;
}

void arMasterSlaveFramework::setEventQueueCallback( arFrameworkEventQueueCallback callback ) {
  _eventQueueCallback = callback;
  ar_log_remark() << _label << " set event queue callback.\n";
}

// The sound server should be able to find its files in the application
// directory. If this function is called between init() and start(),
// soundrender can find clips there. bundlePathName should
// be SZG_PYTHON or SZG_DATA. bundleSubDirectory is usually the app's name.
void arMasterSlaveFramework::setDataBundlePath( const string& bundlePathName,
                                                const string& bundleSubDirectory ) {
  _soundServer.setDataBundlePath( bundlePathName, bundleSubDirectory );
  if( _soundClient ) {
    // Standalone, so set up the internal sound client.
    _soundClient->setDataBundlePath( bundlePathName, bundleSubDirectory );
  }
}

void arMasterSlaveFramework::setPlayTransform( void ) {
  if( soundActive() ) {
    if (getStandalone()) {
      (void) _speakerObject.loadMatrices( _inputState->getMatrix(0) );
    } else {
      dsPlayer( _inputState->getMatrix(0), getHead()->getMidEyeOffset(), getUnitConversion() );
    }
  }
}

void arMasterSlaveFramework::drawGraphicsDatabase( void ){
  _graphicsDatabase.draw();
}

void arMasterSlaveFramework::usePredeterminedHarmony() {
  if (_startCalled) {
    ar_log_warning() << _label << " ignoring usePredeterminedHarmony() after start callback.\n";
    return;
  }

  ar_log_remark() << _label << " using predetermined harmony.  Don't compute in pre-exchange, since slaves don't call that.\n";
  _harmonyInUse = true;
}

int arMasterSlaveFramework::getNumberSlavesExpected() {
  static int totalSlaves = -1;
  if (getStandalone())
    return 0;

  if (!getMaster()) {
    ar_log_warning() << _label << "slave ignoring getNumberSlavesExpected().\n";
    return 0;
  }

  if (totalSlaves == -1) {
    totalSlaves = getAppLauncher()->getNumberScreens() - 1;
    ar_log_remark() << _label << " expects " << totalSlaves << " slaves.\n";
  }
  return totalSlaves;
}

bool arMasterSlaveFramework::allSlavesReady() {
  return _harmonyReady || (_numSlavesConnected >= getNumberSlavesExpected());
}

int arMasterSlaveFramework::getNumberSlavesConnected( void ) const {
  if( !getMaster() ) {
    ar_log_warning() << _label << " slave ignoring getNumberSlavesConnected().\n";
    return -1;
  }
  
  return _numSlavesConnected;
}

bool arMasterSlaveFramework::addTransferField( string fieldName,
                                               void* data,
                                               arDataType dataType,
                                               int size ) {
  if( _startCalled ) {
    ar_log_warning() << _label << " ignoring addTransferField() after start().\n";
    return false;
  }

  if( size <= 0 ) {
    ar_log_warning() << _label << " ignoring addTransferField() with size " << size << ".\n";
    return false;
  }

  if( !data ) {
    ar_log_warning() << _label << " addTransferField() ignoring NULL data.\n";
    return false;
  }

  const string realName("USER_" + fieldName);
  if( _transferTemplate.getAttributeID( realName ) >= 0 ) {
    ar_log_warning() << _label << " ignoring duplicate-name addTransferField().\n";
    return false;
  }

  _transferTemplate.addAttribute( realName, dataType );

  const arTransferFieldDescriptor descriptor( dataType, data, size );
  _transferFieldData.insert( arTransferFieldData::value_type( realName, descriptor ) );

  return true;
}

bool arMasterSlaveFramework::addInternalTransferField( string fieldName,
                                                       arDataType dataType, int size ) {
  if( _startCalled ) {
    ar_log_warning() << _label << " ignoring addTransferField() after start().\n";
    return false;
  }

  if( size <= 0 ) {
    ar_log_warning() << _label << " ignoring addTransferField() with size "
                     << size << ".\n";
    return false;
  }

  const string realName = "USER_" + fieldName;
  if( _transferTemplate.getAttributeID( realName ) >= 0 ) {
    ar_log_warning() << _label << " ignoring addTransferField() with duplicate name.\n";
    return false;
  }

  void* data = ar_allocateBuffer( dataType, size );
  if( !data ) {
    ar_log_warning() << _label << " addInternalTransferField out of memory.\n";
    return false;
  }

  _transferTemplate.addAttribute( realName, dataType );

  const arTransferFieldDescriptor descriptor( dataType, data, size );
  _internalTransferFieldData.insert( arTransferFieldData::value_type( realName,descriptor ) );

  return true;
}

// todo: decopypaste setInternalTransferFieldSize and getTransferField

bool arMasterSlaveFramework::setInternalTransferFieldSize( string fieldName,
                                                           arDataType dataType, int newSize ) {
  if (!getMaster()) {
    ar_log_warning() << _label << " slave ignoring setInternalTransferFieldSize().\n";
    return false;
  }

  const string realName("USER_" + fieldName);
  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if ( iter == _internalTransferFieldData.end() ) {
     ar_log_warning() << _label << ": no internal transfer field '" << fieldName << "'.\n";
    return false;
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    ar_log_warning() << _label << ": type '"
                   << arDataTypeName( dataType ) << "' for internal transfer field '"
                   << fieldName << "' should be '" << arDataTypeName( p.type ) << "'.\n";
    return false;
  }

  if ( newSize == p.size )
    return true;

  ar_deallocateBuffer( p.data );
  p.data = ar_allocateBuffer(  p.type, newSize );
  if( !p.data ) {
    ar_log_warning() << _label << " failed to resize field '" << fieldName << "'.\n";
    return false;
  }

  p.size = newSize;
  return true;
}

void* arMasterSlaveFramework::getTransferField( string fieldName,
                                                arDataType dataType, int& size ) {
  const string realName("USER_" + fieldName);

  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if( iter == _internalTransferFieldData.end() ) {
    iter = _transferFieldData.find( realName );
    if( iter == _transferFieldData.end() ) {
      ar_log_warning() << _label << ": no transfer field '" << fieldName << "'.\n";
      return NULL;
    }
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    ar_log_warning() << _label << ": type '"
                   << arDataTypeName( dataType ) << "' for transfer field '"
                   << fieldName << "' should be '" << arDataTypeName( p.type ) << "'.\n";
    return NULL;
  }

  size = p.size;
  return p.data;
}

//********************************************************************************************
// Random number functions, useful for predetermined harmony.

void arMasterSlaveFramework::setRandomSeed( const long newSeed ) {
  if (!_master)
    return;

  if ( newSeed==0 ) {
    ar_log_warning() << _label << " replacing illegal random seed value 0 with -1.\n";
    _newSeed = -1;
  }
  else {
    _newSeed = newSeed;
  }

  _randSeedSet = 1;
}

bool arMasterSlaveFramework::randUniformFloat( float& value ) {
  value = ar_randUniformFloat( &_randomSeed );
  _lastRandVal = value;
  ++_numRandCalls;

  if( _randSynchError & 1 ) {
    ar_log_warning() << _label << ": unequal numbers of calls to randUniformFloat() "
                     << "on different hosts.\n";
  }

  if(_randSynchError & 2 ) {
    ar_log_warning() << _label << ": random number seeds diverging "
                     << "on different hosts.\n";
  }

  if( _randSynchError & 4 ) {
    ar_log_warning() << _label << ": random number values diverging "
                     << "on different hosts.\n";
  }

  bool success = ( _randSynchError == 0 );
  _randSynchError = 0;

  return success;
}


//************************************************************************
// utility functions
//************************************************************************

void arMasterSlaveFramework::_setMaster( bool master ){
  _master = master;
}

bool arMasterSlaveFramework::_sync( void ){
  if( _master ) {
    // localSync() has a cpu-throttle in case no one is connected.
    // So don't call it if nobody's connected: this would throttle
    // the common case of only one application instance.

    if( _stateServer->getNumberConnectedActive() > 0 )
      _barrierServer->localSync();
    return true;
  }

  return !_stateClientConnected || _barrierClient->sync();
}

//************************************************************************
// utility functions for graphics
//************************************************************************

// Check and see if we are supposed to take a screenshot. If so, go
// ahead and take it!
void arMasterSlaveFramework::_handleScreenshot( bool stereo ) {
  if( _screenshotFlag ) {
    char numberBuffer[ 8 ];
    sprintf( numberBuffer,"%i", _whichScreenshot );
    const string screenshotName =
      string( "screenshot" ) + numberBuffer + string( ".jpg" );
    char* buf1 = new char[ _screenshotWidth * _screenshotHeight * 3 ];
    glReadBuffer( stereo ? GL_FRONT_LEFT : GL_FRONT );
    glReadPixels( _screenshotStartX, _screenshotStartY,
                  _screenshotWidth, _screenshotHeight,
                  GL_RGB, GL_UNSIGNED_BYTE, buf1 );

    arTexture texture;
    texture.setPixels( buf1, _screenshotWidth, _screenshotHeight );
    if( !texture.writeJPEG( screenshotName.c_str(),_dataPath ) ) {
      ar_log_warning() << _label << " failed to write screenshot.\n";
    }

    // the pixels are copied into arTexture's memory, so delete them.
    delete[] buf1;
    ++_whichScreenshot;
    _screenshotFlag = false;
  }
}

//************************************************************************
// Functions for data transfer
//************************************************************************

bool arMasterSlaveFramework::_sendData( void ) {
  if( !_master ) {
    ar_log_warning() << _label << " slave ignoring _sendData.\n";
    return false;
  }

  // Pack user data.
  arTransferFieldData::iterator i;
  for( i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      ar_log_warning() << _label << " aborting _sendData with nil data.\n";
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata,
                                i->second.type, i->second.size ) ) {
      ar_log_warning() << _label << " problem in _sendData.\n";
    }
  }

  // Pack internally-stored user data.
  for( i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      ar_log_warning() << _label << " aborting _sendData with nil data #2.\n";
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata,
                                i->second.type, i->second.size ) ) {
      ar_log_warning() << _label << " problem in _sendData #2.\n";
    }
  }

  // Pack the arMasterSlaveDataRouter's managed data
  _dataRouter.internalDumpState();
  int dataRouterDumpSize = -1;

  const char* dataRouterInfo = _dataRouter.getTransferBuffer( dataRouterDumpSize );
  _transferData->dataIn( "szg_data_router", dataRouterInfo,
                         AR_CHAR, dataRouterDumpSize );
  // Pack other data.
  _packInputData();

  // only activates it if necessary (and possible)
  // ar_activateWildcatFramelock();
  _wm->activateFramelock();

  // Activate pending connections. We queue all connection attempts.
  // They are "activated" in an orderly fashion at this precise point in
  // the code.
  if( _barrierServer->checkWaitingSockets() ) {
    // The barrierServer (the synchronization engine) will now mark
    // the corresponding connections to the stateServer (the data engine)
    // as ready to receive. ("passive sockets" correspond to
    // acceptConnectionNoSend in the connection thread)
    _barrierServer->activatePassiveSockets( _stateServer );
  }

  // Here, all connection attempts that were queued up during the previous
  // frame have been fulfilled. The number of "active" connections is the
  // number of slaves to whom we will be sending data in the next 
  // _stateServer->sendData() (right below).
  if( _stateServer ) {
    _numSlavesConnected = _stateServer->getNumberConnected();
  }
  else {
    _numSlavesConnected = -1;
  }

  // Send data, if there are any receivers.
  if( _stateServer->getNumberConnectedActive() > 0 &&
      !_stateServer->sendData( _transferData ) ){
    ar_log_remark() << _label << " state server failed to send data.\n";
    return false;
  }

  return true;
}

bool arMasterSlaveFramework::_getData( void ) {
  if( _master ) {
    ar_log_warning() << _label << " master ignoring _getData.\n";
    return false;
  }

  if( !_stateClientConnected ){
    // Not connected to master, so there's no data to get.
    return true;
  }

  // We are connected, so request activation from the master.
  // This call blocks until an activation occurs.
  if( !_barrierClient->checkActivation() ) {
    _barrierClient->requestActivation();
  }

  // only activates it if necessary (and possible)
  // ar_activateWildcatFramelock();
  _wm->activateFramelock();

  // Read data, since we will be receiving data from the master.
  if( !_stateClient.getData( _inBuffer,_inBufferSize ) ) {
    ar_log_warning() << _label << " state client got no data.\n";
    _stateClientConnected = false;

    // we need to disable framelock now, so that it can be appropriately
    // re-enabled upon reconnection to the master
    // wildcat framelock is only deactivated if this makes sense
    // ar_deactivateWildcatFramelock();
    _wm->deactivateFramelock();
    return false;
  }

  _transferData->unpack(_inBuffer);
  arTransferFieldData::iterator i;
  // unpack the user data
  for( i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i ) {
    void* pdata = i->second.data;
    if( pdata ) {
      _transferData->dataOut( i->first, pdata, i->second.type, i->second.size );
    }
  }
  // unpack internally-stored user data
  for( i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end(); ++i ) {
    int currSize = i->second.size;
    int numItems = _transferData->getDataDimension( i->first );
    if( numItems == 0 ) {
      ar_deallocateBuffer( i->second.data );
      i->second.data = NULL;
    }

    if( numItems != currSize ) {
      ar_deallocateBuffer( i->second.data );
      i->second.data = ar_allocateBuffer( i->second.type, numItems );
      if ( !i->second.data ) {
         ar_log_warning() << _label << " out of memory "
                        << "for transfer field '" <<  i->first << "'.\n";
        return false;
      }

      i->second.size = numItems;
    }

    if( i->second.data ){
      _transferData->dataOut( i->first, i->second.data,
                              i->second.type, i->second.size );
    }
  }

  // unpack the data intended for the data router
  _dataRouter.setRemoteStreamConfig( _stateClient.getRemoteStreamConfig() );
  _dataRouter.routeMessages( (char*) _transferData->getDataPtr( "szg_data_router", AR_CHAR ),
                             _transferData->getDataDimension( "szg_data_router" ) );

  // unpack the other data
  _unpackInputData();
  return true;
}

void arMasterSlaveFramework::_pollInputData( void ) {
  if( !_startCalled ) {
    ar_log_warning() << _label << " ignoring _pollInputData() before start().\n";
    return;
  }

  if( !_master ) {
    ar_log_warning() << _label << " slave ignoring _pollInputData.\n";
    return;
  }

  if( !_inputActive )
    return;

  if( _firstTimePoll ) {
    _firstTimePoll = false;
    _startTime = ar_time();
    _time = 0;
  }
  else {
    const double temp = ar_difftime( ar_time(), _startTime ) / 1000.0;
    _lastFrameTime = temp - _time; // in milliseconds
    // Set a 5 microsecond lower bound for low-resolution system clocks,
    // to avoid division by zero.
    if( _lastFrameTime < 0.005 ) {
      _lastFrameTime = 0.005;
    }
    _time = temp;
  }

  // If standalone, get the input events now.
  // AARGH!!!! This could be done more generally...
  if( _standalone && _standaloneControlMode == "simulator" ) {
    _simPtr->advance();
  }

  _inputState->updateLastButtons();
  _inputDevice->processBufferedEvents();

  _head.setMatrix( getMatrix( AR_VR_HEAD_MATRIX_ID, false ) );
}

void arMasterSlaveFramework::_packInputData( void ){
  if( _randSeedSet ) {
    _randomSeed = -labs( _newSeed );
  }

  const arMatrix4 navMatrix( ar_getNavMatrix() );
  if (!_transferData->dataIn( "time",            &_time,                 AR_DOUBLE, 1 ) ||
      !_transferData->dataIn( "lastFrameTime",   &_lastFrameTime,        AR_DOUBLE, 1 ) ||
      !_transferData->dataIn( "harmony_ready",   &_harmonyReady,         AR_INT,    1 ) ||
      !_transferData->dataIn( "navMatrix",       navMatrix.v,            AR_FLOAT, 16 ) ||
      !_transferData->dataIn( "randSeedSet",     &_randSeedSet,          AR_INT,    1 ) ||
      !_transferData->dataIn( "randSeed",        &_randomSeed,           AR_LONG,   1 ) ||
      !_transferData->dataIn( "numRandCalls",    &_numRandCalls,         AR_LONG,   1 ) ||
      !_transferData->dataIn( "randVal",         &_lastRandVal,          AR_FLOAT,  1 ) ||
      !_transferData->dataIn( "eye_spacing",     &_head._eyeSpacing,     AR_FLOAT,  1 ) ||
      !_transferData->dataIn( "mid_eye_offset",  _head._midEyeOffset.v,  AR_FLOAT,  3 ) ||
      !_transferData->dataIn( "eye_direction",   _head._eyeDirection.v,  AR_FLOAT,  3 ) ||
      !_transferData->dataIn( "head_matrix",     _head._matrix.v,        AR_FLOAT, 16 ) ||
      !_transferData->dataIn( "near_clip",       &_head._nearClip,       AR_FLOAT,  1 ) ||
      !_transferData->dataIn( "far_clip",        &_head._farClip,        AR_FLOAT,  1 ) ||
      !_transferData->dataIn( "unit_conversion", &_head._unitConversion, AR_FLOAT,  1 ) ||
      !_transferData->dataIn( "fixed_head_mode", &_head._fixedHeadMode,  AR_INT,    1 ) ) {
    ar_log_warning() << _label << ": problem in _packInputData.\n";
  }

  if( !ar_saveEventQueueToStructuredData( &_inputEventQueue, _transferData ) ) {
    ar_log_warning() << _label << " failed to pack input event queue.\n";
  }

  _numRandCalls = 0;
  _randSeedSet = 0;
  _firstTransfer = 0;
}

void arMasterSlaveFramework::_unpackInputData( void ){
  _transferData->dataOut( "time",            &_time,                 AR_DOUBLE, 1 );
  _transferData->dataOut( "lastFrameTime",   &_lastFrameTime,        AR_DOUBLE, 1 );
  _transferData->dataOut( "harmony_ready",   &_harmonyReady,         AR_INT,    1 );
  _transferData->dataOut( "eye_spacing",     &_head._eyeSpacing,     AR_FLOAT,  1 );
  _transferData->dataOut( "mid_eye_offset",  _head._midEyeOffset.v,  AR_FLOAT,  3 );
  _transferData->dataOut( "eye_direction",   _head._eyeDirection.v,  AR_FLOAT,  3 );
  _transferData->dataOut( "head_matrix",     _head._matrix.v,        AR_FLOAT, 16 );
  _transferData->dataOut( "near_clip",       &_head._nearClip,       AR_FLOAT,  1 );
  _transferData->dataOut( "far_clip",        &_head._farClip,        AR_FLOAT,  1 );
  _transferData->dataOut( "unit_conversion", &_head._unitConversion, AR_FLOAT,  1 );
  _transferData->dataOut( "fixed_head_mode", &_head._fixedHeadMode,  AR_INT,    1 );

  arMatrix4 navMatrix;
  _transferData->dataOut( "navMatrix", navMatrix.v, AR_FLOAT, 16 );
  ar_setNavMatrix( navMatrix );

  _inputEventQueue.clear();
  if (!ar_setEventQueueFromStructuredData( &_inputEventQueue, _transferData )) {
    ar_log_warning() << _label << " failed to unpack input event queue.\n";
  } else {
    _inputState->updateLastButtons();
    arInputEvent nextEvent(_inputEventQueue.popNextEvent());
    while (nextEvent) {
      _inputState->update(nextEvent);
      nextEvent = _inputEventQueue.popNextEvent();
    }
  }

  const long lastNumCalls = _numRandCalls;
  const long lastSeed = _randomSeed;
  _transferData->dataOut( "randSeedSet",  &_randSeedSet,  AR_INT,  1 );
  _transferData->dataOut( "randSeed",     &_randomSeed,   AR_LONG, 1 );
  _transferData->dataOut( "numRandCalls", &_numRandCalls, AR_LONG, 1 );

  float tempRandVal = 0.0f;
  _transferData->dataOut( "randVal", &tempRandVal, AR_FLOAT, 1 );
  _randSynchError = 0;

  if( !_firstTransfer ) {
    if( lastNumCalls != _numRandCalls ) {
      _randSynchError |= 1;
    }

    if( ( lastSeed != _randomSeed ) && !_randSeedSet ) {
      _randSynchError |= 2;
    }

    if( tempRandVal != _lastRandVal ) {
      _randSynchError |= 4;
    }
  }
  else {
//    _sendSlaveReadyMessage();
    _firstTransfer = 0;
  }

  _numRandCalls = 0;
}

//************************************************************************
// functions pertaining to starting the application
//************************************************************************

// Determines whether or not we are the master instance
bool arMasterSlaveFramework::_determineMaster() {
  // each master/slave application has it's own unique service,
  // since each has its own unique protocol
  _serviceName = _SZGClient.createComplexServiceName( "SZG_MASTER_" + _label );

  _serviceNameBarrier = _SZGClient.createComplexServiceName( "SZG_MASTER_" + _label + "_BARRIER" );

  // if running on a virtual computer, use that info to determine if we are
  // the master or not. otherwise, it's first come-first served in terms of
  // who the master is. NOTE: we allow the virtual computer to not define
  // a master... in which case it is more or less random who gets to be the
  // master.
  if( _SZGClient.getVirtualComputer() != "NULL" &&
      _launcher.getMasterName() != "NULL" ) {
    if( _launcher.isMaster() || _SZGClient.getMode( "default" ) == "master" ) {
      _setMaster( true );

      if( !_SZGClient.registerService( _serviceName, "graphics", 1, _masterPort ) ) {
        ar_log_warning() << _label << " component failed to be master.\n";
        return false;
      }
    }
    else {
      _setMaster( false );
    }
  }
  else {
    // we'll be the master if we are the first to register the service
    _setMaster( _SZGClient.registerService( _serviceName, "graphics", 1,
                                           _masterPort ) );
  }

  return true;
}

// Sometimes we may want to run the program by itself, without connecting
// to the distributed system.
bool arMasterSlaveFramework::_initStandaloneObjects( void ) {
  // Create the input node. NOTE: there are, so far, two different ways
  // to control a standalone master/slave application. An embedded
  // inputsimulator and a joystick interface
  _inputActive = true;
  _inputDevice = new arInputNode( true );
  _inputState = &_inputDevice->_inputState;

  // Which mode are we using? The simulator mode is the default.
  _standaloneControlMode = _SZGClient.getAttribute( "SZG_DEMO", "control_mode",
                                                    "|simulator|joystick|");
  // _simPtr defaults to &_simulator, a vanilla arInputSimulator instance.
  // If it has been set to something else in code, then we don't mess with it here.
  if (_simPtr == &_simulator) {
    arInputSimulatorFactory simFactory;
    arInputSimulator* simTemp = simFactory.createSimulator( _SZGClient );
    if (simTemp) {
      _simPtr = simTemp;
    }
  }
  if (_standaloneControlMode == "simulator") {
    _simPtr->registerInputNode( _inputDevice );
  }
  else {
    // the joystick is the only other option so far
    arSharedLib* joystickObject = new arSharedLib();
    string sharedLibLoadPath = _SZGClient.getAttribute( "SZG_EXEC", "path" );
    string pforthProgramName = _SZGClient.getAttribute( "SZG_PFORTH",
                                                        "program_names");
    string error;
    if( !joystickObject->createFactory( "arJoystickDriver", sharedLibLoadPath,
                                        "arInputSource", error ) ) {
      ar_log_warning() << error;
      return false;
    }

    arInputSource* driver = (arInputSource*) joystickObject->createObject();

    // The input node is not responsible for clean-up
    _inputDevice->addInputSource( driver, false );
    if( pforthProgramName == "NULL" ) {
      ar_log_remark() << _label << ": no pforth program for standalone joystick.\n";
    }
    else {
      string pforthProgram = _SZGClient.getGlobalAttribute( pforthProgramName );
      if( pforthProgram == "NULL" ) {
        ar_log_remark() << _label << ": no pforth program for '"
		        << pforthProgramName << "'.\n";
      }
      else {
        arPForthFilter* filter = new arPForthFilter();
        ar_PForthSetSZGClient( &_SZGClient );

        if( !filter->loadProgram( pforthProgram ) ) {
	  ar_log_remark() << _label <<
	    " failed to configure pforth filter with program '" << pforthProgram << "'.\n";
          return false;
        }

        // The input node is not responsible for clean-up
        _inputDevice->addFilter( filter, false );
      }
    }
  }

  // Ignore init()'s success.  (Failure might mean just that
  // no szgserver was found, vacuously true since standalone.)
  // But init() nevertheless, so arInputNode components also get init'ed.
  (void)_inputDevice->init( _SZGClient );
  _inputDevice->start();
  _installFilters();

  // Init sound here (not in
  // startStandaloneObjects), so it's before the user-defined init.
  // So that _soundClient calls
  // registerLocalConnection before any dsLoop etc happens.
  arSpeakerObject* speakerObject = new arSpeakerObject();

  //;;;; _soundClient not a pointer?  Like the other framework.
  // must be a pointer? fmod 3.7 needed it, lest the constructor conflict
  // with exes that themselves use fmod (or standalone).  fmod 4 maybe no longer so.
  _soundClient = new arSoundClient();
  _soundClient->setSpeakerObject( speakerObject );
  _soundClient->configure( &_SZGClient );
  _soundClient->_cliSync.registerLocalConnection( &_soundServer._syncServer );
  if (!_soundClient->init())
    ar_log_warning() << "silent.\n";

  // Sound is inactive for the time being
  _soundActive = false;

  // This always succeeds.
  return true;
}

// This is lousy factoring. I think it would be a good idea to eventually
// fold init and start in together.
bool arMasterSlaveFramework::_startStandaloneObjects( void ) {
  _soundServer.start();
  _soundActive = true;
  return true;
}

bool arMasterSlaveFramework::_initMasterObjects() {
  // attempt to initialize the barrier server
  _barrierServer = new arBarrierServer();
  if( !_barrierServer ) {
    ar_log_warning() << _label << " master failed to construct barrier server.\n";
    return false;
  }

  _barrierServer->setServiceName( _serviceNameBarrier );
  _barrierServer->setChannel( "graphics" );
  if( !_barrierServer->init( _SZGClient ) ) {
    ar_log_warning() << _label << " failed to init barrier server.\n";
  }

  _barrierServer->registerLocal();
  _barrierServer->setSignalObject( &_swapSignal );

  // Try to init _inputDevice.
  _inputActive = false;
  _inputDevice = new arInputNode( true );

  if( !_inputDevice ) {
    ar_log_warning() << _label << " master failed to construct input device.\n";
    return false;
  }

  _inputState = &_inputDevice->_inputState;
  _inputDevice->addInputSource( &_netInputSource, false );
  if (!_netInputSource.setSlot(0)) {
    ar_log_warning() << _label << " failed to set slot 0.\n";
LAbort:
    delete _inputDevice;
    _inputDevice = NULL;
    return false;
  }

  if( !_inputDevice->init( _SZGClient ) ) {
    ar_log_warning() << _label << " master failed to init input device.\n";
    goto LAbort;
  }

  _soundActive = false;
  if( !_soundServer.init( _SZGClient ) ) {
    ar_log_warning() << _label << " failed to init sound server.\n";
    return false;
  }

  ar_log_remark() << _label << " initialized master's objects.\n";
  return true;
}

// Starts the objects needed by the master.
bool arMasterSlaveFramework::_startMasterObjects() {
  // Start the master's service.
  _stateServer = new arDataServer( 1000 );

  if( !_stateServer ) {
    ar_log_warning() << _label << " master failed to construct state server.\n";
    return false;
  }

  _stateServer->smallPacketOptimize( true );
  _stateServer->setInterface( "INADDR_ANY" );

  // the _stateServer's initial ports were set in _determineMaster
  _stateServer->setPort( _masterPort[ 0 ] );

  // TODO TODO TODO TODO TODO TODO TODO
  // there is a very annoying duplication of connection code in many places!
  // (i.e. of this try-to-connect-to-brokered ports 10 times)
  // TODO TODO: copy this cleaned-up version (no "success" variable, no if/then)
  // TODO TODO: to the other ones
  int tries = 0;
  while( tries++ < 10 && !_stateServer->beginListening( &_transferLanguage ) ) {
    ar_log_warning() << _label << " failed to listen on port "
		               << _masterPort[ 0 ] << ".\n";

    _SZGClient.requestNewPorts( _serviceName, "graphics", 1 ,_masterPort );
    _stateServer->setPort( _masterPort[ 0 ] );
  }

  if( tries >= 10 ) {
    // failed to bind to the ports
    return false;
  }

  if( !_SZGClient.confirmPorts( _serviceName, "graphics", 1, _masterPort ) ) {
    ar_log_warning() << _label << " failed to confirm brokered port.\n";
    return false;
  }

  if( !_barrierServer->start() ) {
    ar_log_warning() << _label << " failed to start barrier server.\n";
    return false;
  }

  if (!_harmonyInUse) {
    if (!_startInput()) {
      ar_log_warning() << _label << " failed to start input device.\n";
      return false;
    }
  }
  
  _installFilters();

  // start the sound server
  if( !_soundServer.start() ) {
    ar_log_warning() << _label << " failed to start sound server.\n";
    return false;
  }

  _soundActive = true;

  ar_log_remark() << _label << " started master's objects.\n";
  return true;
}

bool arMasterSlaveFramework::_startInput() {
  if( !_inputDevice->start() ) {
    ar_log_warning() << _label << " failed to start input device.\n";

    delete _inputDevice;
    _inputDevice = NULL;

    return false;
  }
  _inputActive = true;
  return true;
}

bool arMasterSlaveFramework::_initSlaveObjects() {
  // slave instead of master
  // in this case, the component must know which networks on which it
  // should attempt to connect
  _networks = _SZGClient.getNetworks( "graphics" );
  _inBufferSize = 1000;
  _inBuffer = new ARchar[ _inBufferSize ];
  _barrierClient = new arBarrierClient;

  if ( !_barrierClient ) {
     ar_log_warning() << _label << "slave failed to construct barrier client.\n";
    return false;
  }

  _barrierClient->setNetworks( _networks );
  _barrierClient->setServiceName( _serviceNameBarrier );

  if( !_barrierClient->init( _SZGClient ) ) {
    ar_log_warning() << _label << " slave failed to start barrier client.\n";
    return false;
  }

  _inputState = new arInputState();

  if( !_inputState ) {
    ar_log_warning() << _label << "slave failed to construct input state.\n";
    return false;
  }
  // of course, the state client should not have weird delays on small
  // packets, as would be the case in the Win32 TCP/IP stack without
  // doing the following
  _stateClient.smallPacketOptimize( true );

  ar_log_remark() << _label << " initialized slaves' objects.\n";
  return true;
}

// Starts the objects needed by the slaves
bool arMasterSlaveFramework::_startSlaveObjects() {
  // the barrier client is the only object to start
  if( !_barrierClient->start() ) {
    ar_log_warning() << _label << " barrier client failed to start.\n";
    return false;
  }

  ar_log_remark() << _label << " started slaves' objects.\n";
  return true;
}

bool arMasterSlaveFramework::_startObjects( void ){
  // THIS IS GUARANTEED TO BEGIN AFTER THE USER-DEFINED INIT!

  // Create the language.
  _transferLanguage.add( &_transferTemplate );
  _transferData = new arStructuredData( &_transferTemplate );

  if( !_transferData ) {
    ar_log_warning() << _label << " master failed to construct _transferData.\n";
    return false;
  }

  // Start the arMasterSlaveDataRouter (this just creates the
  // language to be used by that device, using the registered
  // arFrameworkObjects). Always succeeds.
  (void) _dataRouter.start();

  // By now, we know if we are the master
  if( _master ) {
    if( !_startMasterObjects() ) {
      return false;
    }
  }
  else {
    if( !_startSlaveObjects() ) {
      return false;
    }
  }

  // Both master and slave need a connection thread
  if(!_connectionThread.beginThread(ar_masterSlaveFrameworkConnectionTask, this)) {
    ar_log_warning() << _label << " failed to start connection thread.\n";
    return false;
  }

  _graphicsDatabase.loadAlphabet( _textPath );
  _graphicsDatabase.setTexturePath( _texturePath );

  return true;
}

bool arMasterSlaveFramework::start() {
  return start(true, true);
}

bool arMasterSlaveFramework::start( bool useWindowing, bool useEventLoop ) {
  _useWindowing = useWindowing;
  if ( !_parametersLoaded ) {
    if ( _SZGClient ) {
      ar_log_warning() << _label << ": can't start() before init().\n";
      if( !_SZGClient.sendInitResponse( false ) ) {
        cerr << _label << ": maybe szgserver died.\n";
      }
    }
    return false;
  }

  if( !_standalone ) {
    // Get the screen resource.
    // Grab this lock only AFTER an app launching:
    // don't do this in init(), which is called even on the trigger
    // instance, but do it here, which is only reached on render instances.
    const string screenLock =
      _SZGClient.getComputerName() + "/" + _SZGClient.getMode( "graphics" );
    int graphicsID;
    if( !_SZGClient.getLock( screenLock, graphicsID ) ) {
      char buf[ 20 ];
      sprintf( buf, "%d", graphicsID );
      return _startrespond( "failed to get screen resource held by component " +
			    string( buf ) + ".\n(dkill that component to proceed.)" );
    }
  }

  // Do user-defined init.  After _startDetermineMaster(),
  // so _startCallback knows if this instance is master or slave.
  if( !onStart( _SZGClient ) ) {
    if( _SZGClient ) {
      return _startrespond( "arMasterSlaveFramework start callback failed." );
    }
    return false;
  }

  if (!createWindows(_useWindowing)){
    return false; 
  }

  if( _standalone ) {
    // copypaste from _startObjects
    _graphicsDatabase.loadAlphabet( _textPath );
    _graphicsDatabase.setTexturePath( _texturePath );
    _startStandaloneObjects();
  }
  else {
    if( !_startObjects() ) {
      return _startrespond( "Objects failed to start." );
    }
  }

  _startCalled = true;

  if(_SZGClient) {
    if (!_SZGClient.sendStartResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }
  } else {
    cout << _SZGClient.startResponse().str() << endl;
  }

  if( !_standalone ) {
    _wm->findFramelock();
  }
  
  if (useEventLoop || _useWindowing){
    // _displayThreadRunning is used to coordinate shutdown with
    // program stop via ESC-press (arGUI keyboard callback) or kill message
    // (received in the message thread).
    _displayThreadRunning = true;
  }

  if( useEventLoop ) {
    // Internal event loop.
    // NOTE: stop(...) must have been called at one of the three kill
    // points ("quit" message, clicking on window close button, pressing
    // ESC key) to get to here.
    while( !stopping() ){
      loopQuantum();
    }
    exitFunction();
    exit(0);
  }

  // External event loop.  Caller should now repeatedly call e.g. loopQuantum().
  return true;
}

bool arMasterSlaveFramework::_startrespond( const string& s ) {
  ar_log_warning() << _label << ": " << s << ar_endl;
  
  if( !_SZGClient.sendStartResponse( false ) ) {
    cerr << _label << ": maybe szgserver died.\n";
  }
  
  return false;
}

//**************************************************************************
// Other system-level functions
//**************************************************************************

bool arMasterSlaveFramework::_loadParameters( void ) {

  ar_log_remark() << _label << " reloading parameters.\n";

  // some things depend on the SZG_RENDER
  _texturePath = _SZGClient.getAttribute( "SZG_RENDER","texture_path" );
  string received( _SZGClient.getAttribute( "SZG_RENDER","text_path" ) );
  ar_stringToBuffer( ar_pathAddSlash( received ), _textPath, sizeof( _textPath ) );

  // Set window-wide attributes based on the display name, like
  // stereo, window size, window position, wildcat framelock.

  // copypaste with graphics/arGraphicsClient.cpp, start
  const string whichDisplay = _SZGClient.getMode( "graphics" );
  const string displayName = _SZGClient.getAttribute( whichDisplay, "name" );

  if (displayName == "NULL") {
    ar_log_warning() << _label << ": display " << whichDisplay << "/name undefined, using default.\n";
  } else {
    ar_log_remark() << _label << " displaying on " << whichDisplay <<
      " : " << displayName << ".\n";
  }

  // By default, arTexture::_loadIntoOpenGL() will abort and print an error message
  // if you attempt to load a texture whose dimensions are not powers of two. You
  // can override this behavior by setting this variable
  bool textureBlockNotPowOf2 = _SZGClient.getAttribute( "SZG_RENDER", "block_texture_not_pow2" )!=string("false");
  ar_setTextureBlockNotPowOf2( textureBlockNotPowOf2 );

  _guiXMLParser->setConfig( _SZGClient.getGlobalAttribute( displayName ) );
  if( _guiXMLParser->parse() < 0 ) {
    // already complained
    return false;
  }
  // copypaste with graphics/arGraphicsClient.cpp, end

  if( getMaster() ) {
    _head.configure( _SZGClient );
  }

  if( _standalone ) {
    _simPtr->configure( _SZGClient );
  }

  // Does nothing?
  if( !_speakerObject.configure( &_SZGClient ) ) {
    return false;
  }

  _loadNavParameters();

  const string szgExecPath = _SZGClient.getAttribute("SZG_EXEC","path"); // for dll's
  arGraphicsPluginNode::setSharedLibSearchPath( szgExecPath );
  
  // Ensure everybody gets the right bundle map, standalone or not.
  _dataPath = _SZGClient.getDataPath();
  _soundServer.addDataBundlePathMap( "SZG_DATA", _dataPath );
  if (_soundClient)
    _soundClient->addDataBundlePathMap( "SZG_DATA", _dataPath );

  const string pythonPath = _SZGClient.getDataPathPython();
  _soundServer.addDataBundlePathMap( "SZG_PYTHON", pythonPath );
  if (_soundClient)
    _soundClient->addDataBundlePathMap( "SZG_PYTHON", pythonPath );

  return true;
}

void arMasterSlaveFramework::_messageTask( void ) {
  // might be a good idea to shut this down cleanly... BUT currently
  // there's no way to shut the arSZGClient down cleanly.
  string messageType, messageBody;

  while( !stopping() ) {
    // NOTE: it is possible for receiveMessage to fail, precisely in the
    // case that the szgserver has *hard-shutdown* our client object.
    if( !_SZGClient.receiveMessage( &messageType, &messageBody ) ) {
      // NOTE: the shutdown procedure is the same as for the "quit" message.
      stop( true );

      // If we are, in fact, using an external thread of control (i.e. started
      // without the m/s framework windowing, then DO NOT exit here.
      // That will occur in the external thread of control.
      if( !_useExternalThread ) {
        exit( 0 );
      }

      // We DO NOT want to hit this again! (since things are DISCONNECTED)
      break;
    }

    if( messageType == "quit" ) {
      // At this point, we do our best to bring everything to an orderly halt.
      // This keeps some programs from seg-faulting on exit, which is bad news
      // on Win32 since it (using default registry settings) brings up a dialog
      // box that must be clicked!
      // NOTE: we block here until the display thread is finished.
      stop( true );

      // If we are, in fact, using an external thread of control (i.e. started
      // without the m/s framework windowing, then DO NOT exit here.
      // That will occur in the external thread of control.
      if( !_useExternalThread ) {
        exit( 0 );
      }
    }
    else if ( messageType== "performance" ) {
      if ( messageBody == "on" ) {
	      _showPerformance = true;
      }
      else {
        _showPerformance = false;
      }
    }
    else if ( messageType == "reload" ) {
      // Yes, this is a little bit bogus. By setting the variable here,
      // the event loop goes ahead and does the reload over there...
      // setting _requestReload back to false after it is done.
      // Consequently, if there are multiple reload messages, sent in quick
      // succession, some might fail.
      _requestReload = true;
    }
    else if ( messageType == "user" ) {
      onUserMessage( messageBody );
    }
    else if( messageType == "color" ) {
      if ( messageBody == "NULL" || messageBody == "off") {
        _noDrawFillColor = arVector3( -1.0f, -1.0f, -1.0f );
      }
      else {
	float tmp[ 3 ];
	ar_parseFloatString( messageBody, tmp, 3 );
	// todo: error checking
        memcpy( _noDrawFillColor.v, tmp, 3 * sizeof( AR_FLOAT ) );
      }
    }
    else if( messageType == "screenshot" ) {
      // copypaste with graphics/szgrender.cpp
      if ( _dataPath == "NULL" ) {
	ar_log_warning() << _label << " screenshot failed: undefined SZG_DATA/path.\n";
      }
      else{
        _screenshotFlag = true;
        if( messageBody != "NULL" ) {
          int tmp[ 4 ];
          ar_parseIntString( messageBody, tmp, 4 );
	        // todo: error checking
          _screenshotStartX = tmp[ 0 ];
      	  _screenshotStartY = tmp[ 1 ];
      	  _screenshotWidth  = tmp[ 2 ];
      	  _screenshotHeight = tmp[ 3 ];
	}
      }
    }
    else if( messageType == "demo" ) {
      setFixedHeadMode(messageBody=="on");
    }

    //*********************************************************
    // There's quite a bit of copy-pasting between the
    // messages accepted by szgrender and here... how can we
    // reuse messaging functionality?????
    //*********************************************************
    if( messageType == "delay" ) {
      _framerateThrottle = messageBody == "on";
        _framerateThrottle = true;
    }
    else if ( messageType == "pause" ) {
      if ( messageBody == "on" ) {
        ar_mutex_lock( &_pauseLock );
        if ( !stopping() )
	  _pauseFlag = true;
	ar_mutex_unlock(&_pauseLock);
      }
      else if( messageBody == "off" ) {
        ar_mutex_lock( &_pauseLock );
        _pauseFlag = false;
        _pauseVar.signal();
        ar_mutex_unlock( &_pauseLock );
      }
      else
        ar_log_warning() << _label <<
	  " ignoring unexpected pause arg '" << messageBody << "'.\n";
    }

  }
}

// Handle connections.
void arMasterSlaveFramework::_connectionTask( void ) {
  _connectionThreadRunning = true; // for shutdown

  if( _master ) {
    while( !stopping() ) {
      // TODO TODO TODO TODO TODO TODO
      // As a hack, since non-blocking connection accept has yet to be
      // implemented, pretend the connection thread isn't running
      // during the blocking call
      _connectionThreadRunning = false;
      arSocket* theSocket = _stateServer->acceptConnectionNoSend();
      _connectionThreadRunning = true;

      if (stopping())
        break;

      if( !theSocket || _stateServer->getNumberConnected() <= 0 ) {
        // something bad happened.  Don't keep trying infinitely.
        _exitProgram = true;
	break;
      }

      ar_log_remark() << _label << " slave connected to master";
      const int num = _stateServer->getNumberConnected();

      if ( num > 1 ) {
	ar_log_remark() << " (" << num << " in all)";
      }
      ar_log_remark() << ".\n";
    }
  }
  else {
    // slave
    while( !stopping() ) {
      // make sure barrier is connected
      while( !_barrierClient->checkConnection() && !stopping() ) {
        ar_usleep( 100000 );
      }

      if( stopping() )
        break;

      // hack, since discoverService is currently a blocking call
      // and the arSZGClient does not have a shutdown mechanism in place
      _connectionThreadRunning = false;
      const arPhleetAddress result =
        _SZGClient.discoverService( _serviceName, _networks, true );
      _connectionThreadRunning = true;

      if (stopping())
        break;

      ar_log_remark() << _label << " connecting to "
	              << result.address << ":" << result.portIDs[ 0 ] << ar_endl;

      if( !result.valid ||
          !_stateClient.dialUpFallThrough( result.address, result.portIDs[ 0 ] ) ) {
        ar_log_warning() << _label << " failed to broker; retrying.\n";
        continue;
      }

      // Bond the appropriate data channel to this sync channel.
      _barrierClient->setBondedSocketID( _stateClient.getSocketIDRemote() );
      _stateClientConnected = true;

      ar_log_remark() << _label << " slave connected to master.\n";
      while( _stateClientConnected && !stopping() ) {
        ar_usleep( 300000 );
      }

      if (stopping())
        break;

      ar_log_remark() << _label << " slave disconnected.\n";
      _stateClient.closeConnection();
    }
  }

  _connectionThreadRunning = false;
}

//**************************************************************************
// Functions directly pertaining to drawing
//**************************************************************************

// Display a whole arGUIWindow.
// The definition of arMasterSlaveRenderCallback explains how arGUIWindow calls this.
void arMasterSlaveFramework::_drawWindow( arGUIWindowInfo* windowInfo,
                                          arGraphicsWindow* graphicsWindow ) {
  if ( !windowInfo || !graphicsWindow ) {
    ar_log_warning() << _label << " _drawWindow got NULL pointer.\n";
    return;
  }

  int currentWinID = windowInfo->getWindowID();
  if( !_wm->windowExists( currentWinID ) ) {
    ar_log_warning() << _label << " _drawWindow: no arGraphicsWindow with ID " <<
      currentWinID << ".\n";
    return;
  }

  // draw the window
  if(!stopping() ) {
    if( _noDrawFillColor[ 0 ] == -1 ) {
      if( getConnected() && !(_harmonyInUse && !_harmonyReady) ) {
        graphicsWindow->draw();

        if ( _wm->isFirstWindow( currentWinID ) ) {
          if( _standalone && _standaloneControlMode == "simulator" 
              && _showSimulator) {
            _simPtr->drawWithComposition();
          }

          if( _showPerformance ){
            _framerateGraph.drawWithComposition();
          }

          // we draw the application's overlay last, if such exists. This is only
          // done on the master instance.
          if( getMaster() ) {
            onOverlay();
          }
        }
      } else {
        onDisconnectDraw();
      }
    } else {
      // only a colored background
      glMatrixMode( GL_PROJECTION );
      glLoadIdentity();
      glOrtho( -1.0, 1.0, -1.0, 1.0, 0.0, 1.0 );
      glMatrixMode( GL_MODELVIEW );
      glLoadIdentity();
      glPushAttrib( GL_ALL_ATTRIB_BITS );
      glDisable( GL_LIGHTING );
      glColor3fv( &_noDrawFillColor[ 0 ] );
      glBegin( GL_QUADS );
      glVertex3f(  1.0f,  1.0f, -0.5f );
      glVertex3f( -1.0f,  1.0f, -0.5f );
      glVertex3f( -1.0f, -1.0f, -0.5f );
      glVertex3f(  1.0f, -1.0f, -0.5f );
      glEnd();
      glPopAttrib();
    }
  }

  // Since some vendors' glFlush/glFinish are unreliable,
  // read a few pixels to force drawing to complete.
  char buffer[ 32 ];
  glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
  // Weirdly, with arGUI, the following seems necessary for "high quality"
  // synchronization. Maybe GLUT was implicitly posting one of these itself?
  // I'd expect the glReadPixels to have already done this.
  glFinish();

  // Take a screenshot if we should.  If there are multiple GUI windows,
  // only the "first" one is captured.
  if ( _wm->isFirstWindow( currentWinID ) )
    _handleScreenshot( _wm->isStereo( currentWinID ) );
}
