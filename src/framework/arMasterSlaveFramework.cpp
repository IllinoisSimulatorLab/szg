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
        ar_log_critical() << "arMasterSlaveFramework frame time = " <<
	  fw->_lastFrameTime << " msec\n";
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

  // Sound nav. matrix ID
  _soundNavMatrixID(-1),

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

  // User messages.
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

  // when the default color is set like this, the app's geometry is displayed
  // instead of a default color
  _masterPort[ 0 ] = -1;

  // Performance graphs.
  const arVector3 white  (1,1,1);
  const arVector3 yellow (1,1,0);
  const arVector3 cyan   (0,1,1);
  _framerateGraph.addElement( "      fps", 250,   100, white  );
  _framerateGraph.addElement( " cpu usec", 250, 10000, yellow );
  _framerateGraph.addElement( "sync usec", 250, 10000, cyan   );
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
  arTransferFieldData::iterator iter;
  for( iter = _internalTransferFieldData.begin();
       iter != _internalTransferFieldData.end(); ++iter) {
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
  // Don't delete _screenObject, since we might not own it.
}

// Initialize the syzygy objects, but does not start any threads
bool arMasterSlaveFramework::init( int& argc, char** argv ) {
  _label = ar_stripExeName( string( argv[0] ) );
  
  // Connect to the szgserver.
  _SZGClient.simpleHandshaking( false );
  if (!_SZGClient.init( argc, argv )){
    return false;
  }
  
  if ( !_SZGClient ) {
    _standalone = true; // init() succeeded, but !_SZGClient says it's disconnected.
    _setMaster( true ); // Because standalone.
    
    // copypaste
    dgSetGraphicsDatabase( &_graphicsDatabase );
    dsSetSoundDatabase( &_soundServer );
    
    // Before _loadParameters, so the internal sound client,
    // created in _initStandaloneObjects, can be configured for standalone.
    if( !_initStandaloneObjects() ) {
      // Maybe a missing joystick driver or a pforth syntax error.
      return false;
    }

    if( !_loadParameters() ) {
      ar_log_warning() << "failed to load parameters while standalone.\n";
    }
    _parametersLoaded = true;

    // Don't start the message-receiving thread yet (because there's no
    // way yet to operate in a distributed fashion).
    // Don't initialize the master's objects, since they'll be unused.
    return true;
  }

  // Connected to szgserver.  Not standalone.
  
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
  // in trigger mode, we use the arAppLauncher object
  // to query info about the virtual computer, which requires the arSZGClient
  // be registered with the arAppLauncher

  // Give _launcher info about the virtual computer.
  (void)_launcher.setSZGClient( &_SZGClient );
  
  if( _SZGClient.getMode( "default" ) == "trigger" ) {
    // We are the trigger node.
    
    // launch components as part of a virtual computer
    _vircompExecution = true;

    const string vircomp = _SZGClient.getVirtualComputer();
    const string defaultMode = _SZGClient.getMode( "default" );
    ar_log_remark() << "virtual computer '" << vircomp <<
      "', default mode '" << defaultMode << "'.\n";

    _launcher.setAppType( "distapp" );

    // Restore "-szg log=DEBUG".
    // todo: also for scenegraph framework. (in arAppLauncher.cpp??)
    // todo: also for other -szg args?  Save them when _SZGClient.init parses them?
    char szLogLevel[80] = "log="; // more than long enough, for internally generated string
    if (!ar_log().logLevelDefault()) {
      // Play fast and loose with extending argv[], since it was longer before.
      argv[argc++] = "-szg";
      argv[argc++] = strncat(szLogLevel, ar_log().logLevel().c_str(), 79);
    }

    // The render program is the (stripped) EXE name, plus parameters.
    {
      char* exeBuf = new char[ _label.length() + 1 ];
      ar_stringToBuffer( _label, exeBuf, _label.length() + 1 );
      char* const exeSave = *argv;
      *argv = exeBuf;
      _launcher.setRenderProgram( ar_packParameters( argc, argv ) );
      *argv = exeSave;
      delete exeBuf;
    }
    
    // Reorganize the virtual computer.
    if( !_launcher.launchApp() ) {
      ar_log_warning() << "failed to launch on virtual computer " << vircomp << ".\n";
      goto fail;
    }
    
    // Wait for the message (render nodes do this in the message task).
    if( !_SZGClient.sendInitResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }
    
    // This application instance, the trigger, only launches other instances.
    ar_log_remark() << "trigger launched app's components.\n";
    
    if( !_SZGClient.sendStartResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }

    (void)_launcher.waitForKill();
    exit(0);
  }
  
  // We're not the trigger node.
  
  // So apps can query the virtual computer's attributes, 
  // those attributes are not restricted to the master.
  // So setParameters() is called. 
  // But we allow setting a bogus virtual computer via "-szg".
  // So it's not fatal if setParamters() fails, i.e. this should launch:
  //   my_program -szg virtual=bogus
  if( _SZGClient.getVirtualComputer() != "NULL" && !_launcher.setParameters() ) {
    ar_log_warning() << "invalid virtual computer definition.\n";
  }
  
  // Launch the message thread, so dkill works even if init() or start() fail.
  // But launch after the trigger code above, lest we catch messages in both
  // waitForKill() AND _messageThread.
  if(!_messageThread.beginThread( ar_masterSlaveFrameworkMessageTask, this )){
    ar_log_warning() << "failed to start message thread.\n";
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
  _pauseLock.lock();
    _exitProgram = true;
    _pauseFlag   = false;
    _pauseVar.signal();
  _pauseLock.unlock();
  
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
  
  arSleepBackoff a(50,100, 1.1);
  while( _connectionThreadRunning ||
         ( _useWindowing && _displayThreadRunning && blockUntilDisplayExit ) ||
         ( _useExternalThread && _externalThreadRunning ) ) {
    a.sleep();
  }
  
  ar_log_remark() << "stopped.\n";
  _stopped = true;
}

bool arMasterSlaveFramework::createWindows( bool useWindowing ) {
  std::vector< arGUIXMLWindowConstruct* >* windowConstructs =
    _guiXMLParser->getWindowingConstruct()->getWindowConstructs();
  
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
  if( _wm->createWindows( _guiXMLParser->getWindowingConstruct(), useWindowing ) < 0 ) {
    ar_log_warning() << "failed to create windows.\n";
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

  // Process events from keyboard, window manager, etc.
  _wm->processWindowEvents();
}

void arMasterSlaveFramework::exitFunction(){
  // Wildcat framelock is only deactivated if this makes sense;
  // and if so, do so in the display thread and before exiting.
  _wm->deactivateFramelock();
  _wm->deleteAllWindows();

  // Guaranteed to get here--user-defined cleanup will be called at the right time
  onCleanup();

  // Now it's safe for other threads, like message thread, to exit.
  _displayThreadRunning = false;

  // If stop() is called from the arGUI keyboard function,
  // exit immediately (_blockUntilDisplayExit will be false).
  // Otherwise, wait for the message thread to exit.
  arSleepBackoff a(70, 120, 1.1);
  while (_blockUntilDisplayExit)
    a.sleep();
}

// The sequence of events that should occur before the window is drawn.
// Public, so apps can create custom event loops.
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
  _pauseLock.lock();
  while( _pauseFlag ) {
    _pauseVar.wait( _pauseLock );
  }
  _pauseLock.unlock();
  
  // the pause might have been triggered by shutdown
  if (stopping())
    return;
  
  _processUserMessages();
  
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
          ar_log_warning() << "_startInput() failed.\n";
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
  
  // Frame rate bug: computed in _pollInputData, useless for slaves.
  _framerateGraph.getElement( "      fps" )->pushNewValue(1000. / _lastFrameTime);
  _framerateGraph.getElement( " cpu usec" )->pushNewValue(_lastComputeTime);
  _framerateGraph.getElement( "sync usec" )->pushNewValue(_lastSyncTime);
  
  // Get performance metrics.
  _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
}

// Public, so apps can make custom event loops.
void arMasterSlaveFramework::draw( int windowID ){
  if( stopping() || !_wm )
    return;

  if( windowID < 0 )
    _wm->drawAllWindows( true );
  else
    _wm->drawWindow( windowID, true );
}

// What happens after the window is drawn, but before synchronizing.
void arMasterSlaveFramework::postDraw( void ){
  if (stopping())
    return;
  
  const ar_timeval postDrawStart = ar_time();

  // For testing sync.
  if( _framerateThrottle ) {
    ar_usleep( 200000 );
  }

  if( !_standalone && !_sync() ) {
    ar_log_warning() << "sync failed in postDraw().\n";
  }
  _lastSyncTime = ar_difftime( ar_time(), postDrawStart );
}

// Public, so apps can make custom event loops.
void arMasterSlaveFramework::swap( int windowID ) {
  if (stopping() || !_wm)
    return;
  
  if( windowID >= 0 )
    _wm->swapWindowBuffer( windowID, true );
  else
    _wm->swapAllWindowBuffers(true);
}

bool arMasterSlaveFramework::onStart( arSZGClient& SZGClient ) {
  if( _startCallback && !_startCallback( *this, SZGClient ) ) {
    ar_log_warning() << "user-defined start callback failed.\n";
    return false;
  }

  return true;
}

void arMasterSlaveFramework::_stop(const char* name, const arMSCallbackException& exc) {
  ar_log_error() << "arMasterSlaveFramework " << name << " callback:\n\t" <<
    exc.message << "\n";
  stop(false);
}

void arMasterSlaveFramework::onPreExchange( void ) {
  if ( _preExchange ) {
    try {
      _preExchange( *this );
    } catch (arMSCallbackException exc) {
      _stop("preExchange", exc);
      stop(false);
    }
  }
}

void arMasterSlaveFramework::onPostExchange( void ) {
  if( _postExchange ) {
    try {
      _postExchange( *this );
    } catch (arMSCallbackException exc) {
      _stop("postExchange", exc);
    }
  }
}

void arMasterSlaveFramework::onWindowInit( void ) {
  if( _windowInitCallback ) {
    try {
      _windowInitCallback( *this );
    } catch (arMSCallbackException exc) {
      _stop("windowInit", exc);
    }
  }
  else {
    ar_defaultWindowInitCallback();
  }
}

// Only events that are actually generated by the GUI itself actually reach
// here, not for instance events that we post to the window manager ourselves.
void arMasterSlaveFramework::onWindowEvent( arGUIWindowInfo* wI ) {
  if( wI ) {
    if (_windowEventCallback ) {
      try {
        _windowEventCallback( *this, wI );
      } catch (arMSCallbackException exc) {
	_stop("windowEvent", exc);
      }
    } else if ( wI->getUserData() ) {
      // default window event handler, at least to handle a resizing event
      // (should we be handling window close events as well?)
      arMasterSlaveFramework* fw = (arMasterSlaveFramework*) wI->getUserData();

      if( !fw ) {
        return;
      }

      switch( wI->getState() ) {
        case AR_WINDOW_FULLSCREEN:
        default:
          break;
        case AR_WINDOW_RESIZE:
          fw->_wm->setWindowViewport(
	    wI->getWindowID(), 0, 0, wI->getSizeX(), wI->getSizeY() );
          break;

        case AR_WINDOW_CLOSE:
        // We will only get here if someone clicks the window close decoration.
        // This is NOT reached if we use the arGUIWindowManager's delete
        // method.
          fw->stop( false );
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
      _stop("windowStartGL", exc);
    }
  }
}

// Yes, this is really the application-provided draw function. It is
// called once per viewport of each arGraphicsWindow (arGUIWindow).
// It is a virtual method that issues the user-defined draw callback.
void arMasterSlaveFramework::onDraw( arGraphicsWindow& win, arViewport& vp ) {
  if( (!_oldDrawCallback) && (!_drawCallback) ) {
    ar_log_warning() << "forgot to setDrawCallback().\n";
    return;
  }

  if (_drawCallback) {
    try {
      _drawCallback( *this, win, vp );
    } catch (arMSCallbackException exc) {
      _stop("draw", exc);
    }
    return;
  }

  try {
    _oldDrawCallback( *this );
  } catch (arMSCallbackException exc) {
    _stop("draw", exc);
  }
}

void arMasterSlaveFramework::onDisconnectDraw( void ) {
  if( _disconnectDrawCallback ) {
    try {
      _disconnectDrawCallback( *this );
    } catch (arMSCallbackException exc) {
      _stop("disconnectDraw", exc);
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
  setPlayTransform();
  if( _playCallback ) {
    try {
      _playCallback( *this );
    } catch (arMSCallbackException exc) {
      _stop("play", exc);
    }
  }
}

void arMasterSlaveFramework::onCleanup( void ) {
  if( _cleanup ) {
    try {
      _cleanup( *this );
    } catch (arMSCallbackException exc) {
      _stop("cleanup", exc);
    }
  }
}

void arMasterSlaveFramework::onUserMessage( const int messageID, const string& messageBody ) {
  if (_userMessageCallback) {
    try {
      _userMessageCallback( *this, messageID, messageBody );
    } catch (arMSCallbackException exc) {
      _stop("userMessage", exc);
    }
  } else if (_oldUserMessageCallback) {
    try {
      _oldUserMessageCallback( *this, messageBody );
    } catch (arMSCallbackException exc) {
      _stop("userMessage", exc);
    }
  }
}

void arMasterSlaveFramework::onUserMessage( const string& messageBody ) {
  if( _oldUserMessageCallback ) {
    try {
      _oldUserMessageCallback( *this, messageBody );
    } catch (arMSCallbackException exc) {
      _stop("userMessage", exc);
    }
  }
}

void arMasterSlaveFramework::onOverlay( void ) {
  if( _overlay ) {
    try {
      _overlay( *this );
    } catch (arMSCallbackException exc) {
      _stop("overlay", exc);
    }
  }
}

void arMasterSlaveFramework::onKey( arGUIKeyInfo* keyInfo ) {
  if( !keyInfo ) {
    return;
  }

  // Prefer newer to legacy type of keyboard callback.
  if( _arGUIKeyboardCallback ) {
    try {
      _arGUIKeyboardCallback( *this, keyInfo );
    } catch (arMSCallbackException exc) {
      _stop("keyboard", exc);
    }
  } else if( keyInfo->getState() == AR_KEY_DOWN ) {
    // Legacy behavior: expect only key presses, not releases.
    onKey( keyInfo->getKey(), 0, 0 );
  }
}

void arMasterSlaveFramework::onKey( unsigned char key, int x, int y) {
  if( _keyboardCallback ) {
    try {
      _keyboardCallback( *this, key, x, y );
    } catch (arMSCallbackException exc) {
      _stop("keyboard", exc);
    }
  }
}

void arMasterSlaveFramework::onMouse( arGUIMouseInfo* mouseInfo ) {
  if( mouseInfo && _mouseCallback ) {
    try {
      _mouseCallback( *this, mouseInfo );
    } catch (arMSCallbackException exc) {
      _stop("mouse", exc);
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
// Otherwise the _drawSetUp method is called. This callback
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
  ar_log_remark() << "set draw callback.\n";
  _drawCallback = draw;
}

void arMasterSlaveFramework::setDrawCallback( void (*draw)( arMasterSlaveFramework& ) ) {
  ar_log_remark() << "set old-style draw callback.\n";
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
  ( void (*userMessageCallback)(arMasterSlaveFramework&,
                                const int messageID,
                                const string& messageBody )){
  _userMessageCallback = userMessageCallback;
  _oldUserMessageCallback = NULL;
}
void arMasterSlaveFramework::setUserMessageCallback
  ( void (*userMessageCallback)(arMasterSlaveFramework&,
                                const string& messageBody )){
  _oldUserMessageCallback = userMessageCallback;
  _userMessageCallback = NULL;
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
  ar_log_remark() << "set event queue callback.\n";
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

void arMasterSlaveFramework::loadNavMatrix(void ) {
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  glMultMatrixf( navMatrix.v );
  if (getMaster()) {
    if (_soundNavMatrixID != -1){
      dsTransform( _soundNavMatrixID, navMatrix );
    }
  }
}

void arMasterSlaveFramework::setPlayTransform( void ) {
  if( soundActive() ) {
    if (getStandalone()) {
      (void) _speakerObject.loadMatrices( _inputState->getMatrix(0), _soundServer.getMode() );
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
    ar_log_warning() << "ignoring usePredeterminedHarmony() after start callback.\n";
    return;
  }

  ar_log_remark() <<
    "using predetermined harmony.  Don't compute in pre-exchange, which slaves ignore.\n";
  _harmonyInUse = true;
}

int arMasterSlaveFramework::getNumberSlavesExpected() {
  if (getStandalone())
    return 0;

  if (!getMaster()) {
    ar_log_warning() << "slave ignoring getNumberSlavesExpected().\n";
    return 0;
  }

  static int totalSlaves = -1; // todo: member not static
  if (totalSlaves == -1) {
    totalSlaves = getAppLauncher()->getNumberScreens() - 1;
    ar_log_remark() << "expects " << totalSlaves << " slaves.\n";
  }
  return totalSlaves;
}

bool arMasterSlaveFramework::allSlavesReady() {
  return _harmonyReady || (_numSlavesConnected >= getNumberSlavesExpected());
}

int arMasterSlaveFramework::getNumberSlavesConnected( void ) const {
  if( !getMaster() ) {
    ar_log_warning() << "slave ignoring getNumberSlavesConnected().\n";
    return -1;
  }
  
  return _numSlavesConnected;
}

bool arMasterSlaveFramework::sendMasterMessage( const string& messageBody ) {
  const string lockName = _launcher.getMasterName();
  int processID;
  if (_SZGClient.getLock( lockName, processID )) {
    // nobody was holding the lock, i.e. no master present
    _SZGClient.releaseLock( lockName );
    ar_log_error() << "sendMasterMessage() got no master process ID.\n";
    return false;
  }
  const int iResponseMatch = _SZGClient.sendMessage( "user", messageBody, processID, false );
  if (iResponseMatch == -1) {
    ar_log_error() << "sendMasterMessage() failed to send message.\n";
    return false;
  }
  return true;
}


bool arMasterSlaveFramework::_addTransferField( const string& fieldName,
						void* data,
						const arDataType dataType,
						const int size,
						arTransferFieldData& fields) {
  if( _startCalled ) {
    ar_log_warning() << "ignoring addTransferField() after start().\n";
    return false;
  }

  if( size <= 0 ) {
    ar_log_warning() << "ignoring addTransferField() with negative size " << size << ".\n";
    return false;
  }

  if( !data ) {
    ar_log_warning() << "addTransferField() ignoring NULL data.\n";
    return false;
  }

  const string realName("USER_" + fieldName);
  if( _transferTemplate.getAttributeID( realName ) >= 0 ) {
    ar_log_warning() << "ignoring duplicate-name addTransferField().\n";
    return false;
  }

  _transferTemplate.addAttribute( realName, dataType );
  const arTransferFieldDescriptor descriptor( dataType, data, size );
  fields.insert( arTransferFieldData::value_type( realName, descriptor ) );
  return true;
}

bool arMasterSlaveFramework::addTransferField( const string& fieldName,
                                               void* data,
                                               const arDataType dataType,
                                               const unsigned size ) {
  return _addTransferField(fieldName, data, dataType, size, _transferFieldData);
}

bool arMasterSlaveFramework::addInternalTransferField( const string& fieldName,
                                                       const arDataType dataType,
						       const unsigned size ) {
  
  return _addTransferField(fieldName, ar_allocateBuffer(dataType, size), dataType,
    size, _internalTransferFieldData);
}

bool arMasterSlaveFramework::setInternalTransferFieldSize( const string& fieldName,
                                                           arDataType dataType, unsigned newSize ) {
  if (!getMaster()) {
    ar_log_warning() << "slave ignoring setInternalTransferFieldSize().\n";
    return false;
  }

  const string realName("USER_" + fieldName);
  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if ( iter == _internalTransferFieldData.end() ) {
     ar_log_warning() << "no internal transfer field '" << fieldName << "'.\n";
    return false;
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    ar_log_warning() << "type '" << arDataTypeName( dataType ) <<
      "' for internal transfer field '" << fieldName << "' should be '" <<
      arDataTypeName( p.type ) << "'.\n";
    return false;
  }

  if ( int(newSize) == p.size )
    return true;

  ar_deallocateBuffer( p.data );
  p.data = ar_allocateBuffer(  p.type, newSize );
  if( !p.data ) {
    ar_log_warning() << "failed to resize field '" << fieldName << "'.\n";
    return false;
  }

  p.size = int(newSize);
  return true;
}

void* arMasterSlaveFramework::getTransferField( const string& fieldName,
                                                arDataType dataType, int& size ) {
  const string realName("USER_" + fieldName);
  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if( iter == _internalTransferFieldData.end() ) {
    iter = _transferFieldData.find( realName );
    if( iter == _transferFieldData.end() ) {
      ar_log_warning() << "no transfer field '" << fieldName << "'.\n";
      return NULL;
    }
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    ar_log_warning() << "type '"
                   << arDataTypeName( dataType ) << "' for transfer field '"
                   << fieldName << "' should be '" << arDataTypeName( p.type ) << "'.\n";
    return NULL;
  }

  size = p.size; // int not unsigned, to permit uninitialized value -1.
  return p.data;
}

//********************************************************************************************
// Random number functions, useful for predetermined harmony.

void arMasterSlaveFramework::setRandomSeed( const long newSeed ) {
  if (!_master)
    return;

  if ( newSeed==0 ) {
    ar_log_warning() << "replacing illegal random seed value 0 with -1.\n";
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

  if ( _randSynchError & 1 ) {
    ar_log_warning() << 
      ": hosts have unequal numbers of randUniformFloat() calls.\n";
  }

  if (_randSynchError & 2 ) {
    ar_log_warning() << "hosts have divergent random number seeds.\n";
  }

  if ( _randSynchError & 4 ) {
    ar_log_warning() << "hosts have divergent random number values.\n";
  }

  const bool ok = _randSynchError == 0;
  _randSynchError = 0;
  return ok;
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

// Take a screenshot if it's been requested.
// copypaste with arGraphicsClient::takeScreenshot
void arMasterSlaveFramework::_handleScreenshot( bool stereo ) {
  if (!_screenshotFlag)
    return;

  _screenshotFlag = false;
  const string screenshotName =
    "screenshot" + ar_intToString(_whichScreenshot++) + ".jpg";
  char* buf = new char[ _screenshotWidth * _screenshotHeight * 3];
  glReadBuffer( stereo ? GL_FRONT_LEFT : GL_FRONT );
  glReadPixels( _screenshotStartX, _screenshotStartY,
		_screenshotWidth, _screenshotHeight,
		GL_RGB, GL_UNSIGNED_BYTE, buf );

  // todo: constructor next two lines
  arTexture texture;
  texture.setPixels( buf, _screenshotWidth, _screenshotHeight );
  delete [] buf;
  if( !texture.writeJPEG( screenshotName.c_str(),_dataPath ) ) {
    ar_log_warning() << "failed to write screenshot.\n";
  }
}

//************************************************************************
// Functions for data transfer
//************************************************************************

bool arMasterSlaveFramework::_sendData( void ) {
  if( !_master ) {
    ar_log_warning() << "slave ignoring _sendData.\n";
    return false;
  }

  // Pack user data.
  arTransferFieldData::iterator i;
  for( i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      ar_log_warning() << "aborting _sendData with nil data.\n";
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata, i->second.type, i->second.size ) ) {
      ar_log_warning() << "problem in _sendData.\n";
    }
  }

  // Pack internally-stored user data.
  for( i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      ar_log_warning() << "aborting _sendData with nil data #2.\n";
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata,
                                i->second.type, i->second.size ) ) {
      ar_log_warning() << "problem in _sendData #2.\n";
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

  _wm->activateFramelock();

  // Activate pending connections in orderly fashion. Queue all connection attempts.
  if( _barrierServer->checkWaitingSockets() ) {
    // The barrierServer (the synchronization engine) will now mark
    // the corresponding connections to the stateServer (the data engine)
    // as ready to receive. ("passive sockets" correspond to
    // acceptConnectionNoSend in the connection thread)
    _barrierServer->activatePassiveSockets( _stateServer );
  }

  // All connection attempts queued up during the previous
  // frame have been fulfilled. The number of "active" connections is how many
  // slaves to whom _stateServer->sendData() will send data.
  _numSlavesConnected = _stateServer ? _stateServer->getNumberConnected() : -1;

  if (!_stateServer) {
    ar_log_warning() << "no state server.\n";
    return false;
  }

  // Send data to any receivers.
  if( _stateServer->getNumberConnectedActive() > 0 &&
      !_stateServer->sendData( _transferData ) ){
    ar_log_remark() << "state server failed to send data.\n";
    return false;
  }

  return true;
}

bool arMasterSlaveFramework::_getData( void ) {
  if( _master ) {
    ar_log_warning() << "master ignoring _getData.\n";
    return false;
  }

  if( !_stateClientConnected ){
    // Not connected to master, so there's no data to get.
    return true;
  }

  // Connected, so request activation from the master.
  // Block until activated.
  if( !_barrierClient->checkActivation() ) {
    _barrierClient->requestActivation();
  }

  _wm->activateFramelock();

  // Read data, since we will be receiving data from the master.
  if( !_stateClient.getData( _inBuffer,_inBufferSize ) ) {
    ar_log_warning() << "state client got no data.\n";
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
         ar_log_warning() << "out of memory "
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
    ar_log_warning() << "ignoring _pollInputData() before start().\n";
    return;
  }

  if( !_master ) {
    ar_log_warning() << "slave ignoring _pollInputData.\n";
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
    ar_log_warning() << "problem in _packInputData.\n";
  }

  if( !ar_saveEventQueueToStructuredData( &_inputEventQueue, _transferData ) ) {
    ar_log_warning() << "failed to pack input event queue.\n";
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
    ar_log_warning() << "failed to unpack input event queue.\n";
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
// functions pertaining to user messages
//************************************************************************
void arMasterSlaveFramework::_processUserMessages() {
  arGuard guard( _userMessageLock );
  std::deque< arUserMessageInfo >::const_iterator iter;
  for (iter = _userMessageQueue.begin(); iter != _userMessageQueue.end(); ++iter) {
    onUserMessage( iter->messageID, iter->messageBody );
  }
  _userMessageQueue.clear();
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
        ar_log_warning() << "component failed to be master.\n";
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

// Run the program by itself, without connecting to the distributed system.
bool arMasterSlaveFramework::_initStandaloneObjects( void ) {
  // Create the input node. NOTE: there are, so far, two different ways
  // to control a standalone master/slave application. An embedded
  // inputsimulator and a joystick interface
  _inputActive = true;
  _inputDevice = new arInputNode( true );
  _inputState = &_inputDevice->_inputState;

  _handleStandaloneInput();

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

  _soundActive = false;

  if (_soundNavMatrixID == -1) {
    _soundNavMatrixID = dsTransform(getNavNodeName(),"root",ar_identityMatrix());
  }
  return true;
}

// Lousy factoring. Should combine start and init?
bool arMasterSlaveFramework::_startStandaloneObjects( void ) {
  _soundServer.start();
  _soundActive = true;
  return true;
}

bool arMasterSlaveFramework::_initMasterObjects() {
  _barrierServer = new arBarrierServer();
  if( !_barrierServer ) {
    ar_log_warning() << "master failed to construct barrier server.\n";
    return false;
  }

  if(!_barrierServer->init(_serviceNameBarrier, "graphics", _SZGClient)) {
    ar_log_warning() << "failed to init barrier server.\n";
  }

  _barrierServer->registerLocal();
  _barrierServer->setSignalObject( &_swapSignal );

  // Try to init _inputDevice.
  _inputActive = false;
  _inputDevice = new arInputNode( true );

  if( !_inputDevice ) {
    ar_log_warning() << "master failed to construct input device.\n";
    return false;
  }

  _inputState = &_inputDevice->_inputState;
  _inputDevice->addInputSource( &_netInputSource, false );
  if (!_netInputSource.setSlot(0)) {
    ar_log_warning() << "failed to set slot 0.\n";
LAbort:
    delete _inputDevice;
    _inputDevice = NULL;
    return false;
  }

  if( !_inputDevice->init( _SZGClient ) ) {
    ar_log_warning() << "master failed to init input device.\n";
    goto LAbort;
  }

  _soundActive = false;
  if( !_soundServer.init( _SZGClient ) ) {
    ar_log_warning() << "failed to init audio.\n";
    return false;
  }

  ar_log_remark() << "initialized master's objects.\n";
  return true;
}

// Starts the objects needed by the master.
bool arMasterSlaveFramework::_startMasterObjects() {
  // Start the master's service.
  _stateServer = new arDataServer( 1000 );

  if( !_stateServer ) {
    ar_log_warning() << "master failed to construct state server.\n";
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
    ar_log_warning() << "failed to listen on port "
		               << _masterPort[ 0 ] << ".\n";

    _SZGClient.requestNewPorts( _serviceName, "graphics", 1 ,_masterPort );
    _stateServer->setPort( _masterPort[ 0 ] );
  }

  if( tries >= 10 ) {
    // failed to bind to the ports
    return false;
  }

  if( !_SZGClient.confirmPorts( _serviceName, "graphics", 1, _masterPort ) ) {
    ar_log_warning() << "failed to confirm brokered port.\n";
    return false;
  }

  if( !_barrierServer->start() ) {
    ar_log_warning() << "failed to start barrier server.\n";
    return false;
  }

  if (!_harmonyInUse) {
    if (!_startInput()) {
      ar_log_warning() << "failed to start input device.\n";
      return false;
    }
  }
  
  _installFilters();

  if( !_soundServer.start() ) {
    ar_log_warning() << "failed to start audio.\n";
    return false;
  }

  if (_soundNavMatrixID == -1) {
    _soundNavMatrixID = dsTransform(getNavNodeName(),"root",ar_identityMatrix());
  }
    
  _soundActive = true;

  ar_log_remark() << "started master's objects.\n";
  return true;
}

bool arMasterSlaveFramework::_startInput() {
  if( !_inputDevice->start() ) {
    ar_log_warning() << "failed to start input device.\n";

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
     ar_log_warning() << "slave failed to construct barrier client.\n";
    return false;
  }

  _barrierClient->setNetworks( _networks );
  _barrierClient->setServiceName( _serviceNameBarrier );

  if( !_barrierClient->init( _SZGClient ) ) {
    ar_log_warning() << "slave failed to start barrier client.\n";
    return false;
  }

  _inputState = new arInputState();

  if( !_inputState ) {
    ar_log_warning() << "slave failed to construct input state.\n";
    return false;
  }
  // of course, the state client should not have weird delays on small
  // packets, as would be the case in the Win32 TCP/IP stack without
  // doing the following
  _stateClient.smallPacketOptimize( true );

  ar_log_remark() << "initialized slaves' objects.\n";
  return true;
}

// Starts the objects needed by the slaves
bool arMasterSlaveFramework::_startSlaveObjects() {
  // the barrier client is the only object to start
  if( !_barrierClient->start() ) {
    ar_log_warning() << "barrier client failed to start.\n";
    return false;
  }

  ar_log_remark() << "started slaves' objects.\n";
  return true;
}

bool arMasterSlaveFramework::_startObjects( void ){
  // THIS IS GUARANTEED TO BEGIN AFTER THE USER-DEFINED INIT!

  // Create the language.
  _transferLanguage.add( &_transferTemplate );
  _transferData = new arStructuredData( &_transferTemplate );

  if( !_transferData ) {
    ar_log_warning() << "master failed to construct _transferData.\n";
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
    ar_log_warning() << "failed to start connection thread.\n";
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
      ar_log_warning() << "can't start() before init().\n";
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
    int graphicsID = -1;
    if( !_SZGClient.getLock( screenLock, graphicsID ) ) {
      return _startrespond( "failed to get screen resource held by component " +
	ar_intToString(graphicsID) + ".\n(dkill that component to proceed.)" );
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
  ar_log_warning() << "" << s << ar_endl;
  
  if( !_SZGClient.sendStartResponse( false ) ) {
    cerr << _label << ": maybe szgserver died.\n";
  }
  
  return false;
}

//**************************************************************************
// Other system-level functions
//**************************************************************************

bool arMasterSlaveFramework::_loadParameters( void ) {

  ar_log_remark() << "reloading parameters.\n";

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
    ar_log_warning() << "display " << whichDisplay << "/name undefined, using default.\n";
  } else {
    ar_log_remark() << "displaying on " << whichDisplay <<
      " : " << displayName << ".\n";
  }

  // arTexture::_loadIntoOpenGL() complains and aborts
  // if its texture's dimensions are not powers of two.
  // SZG_RENDER/allow_texture_not_pow2 overrides this.
  const bool textureAllowNotPow2 =
    _SZGClient.getAttribute( "SZG_RENDER", "allow_texture_not_pow2" )==string("true");
  ar_setTextureAllowNotPowOf2( textureAllowNotPow2 );

  _guiXMLParser->setConfig( _SZGClient.getGlobalAttribute( displayName ) );
  if( _guiXMLParser->parse() < 0 ) {
    // already complained
    return false;
  }
  // copypaste with graphics/arGraphicsClient.cpp, end

  if( getMaster() ) {
    _head.configure( _SZGClient );
  }

  if( _standalone && (_standaloneControlMode == "simulator") ) {
    _simPtr->configure( _SZGClient );
  }

  if( !_speakerObject.configure( _SZGClient ) ) {
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
  // todo: cleanly shutdown both this and arSZGClient.
  string messageType, messageBody;

  while( !stopping() ) {
    int messageID = _SZGClient.receiveMessage( &messageType, &messageBody );
    if (!messageID) {
      // szgserver has *hard-shutdown* _SZGClient.

      // 5-line copypaste from case "quit" below.
      // Block until the display thread finishes.
      stop( true );
      if( !_useExternalThread ) {
        exit(0);
      }

      // External thread will exit().  End message task, since it's disconnected.
      return;
    }

    if( messageType == "quit" ) {
      _SZGClient.messageResponse( messageID, getLabel()+" quitting" );
      // Bring everything to an orderly halt, lest crashing windows apps
      // bring up a dialog box that must be clicked.

      // Block until the display thread finishes.
      stop( true );
      if( !_useExternalThread ) {
        exit( 0 );
      }
      // External thread will exit().
    }
    else if (messageType=="log") {
      if (ar_setLogLevel( messageBody )) {
        ar_log_critical() << "set log level to " << messageBody << ar_endl;
        _SZGClient.messageResponse( messageID, getLabel()+" set log level to "+messageBody );
      } else {
        ar_log_error() << "ignoring unrecognized loglevel '" << messageBody << "'.\n";
        _SZGClient.messageResponse( messageID, "ERROR: "+getLabel()+
            " ignoring unrecognized loglevel '"+messageBody+"'." );
      }
    }
    else if ( messageType== "performance" ) {
      _showPerformance = messageBody == "on";
      if (_showPerformance) {
        _SZGClient.messageResponse( messageID, getLabel()+" showing performance graph" );
      } else {
        _SZGClient.messageResponse( messageID, getLabel()+" hiding performance graph" );
      }
    }
    else if ( messageType == "reload" ) {
      // Hack: set _requestReload here, to make the
      // event loop reload over there and then reset _requestReload.
      // Side effect: only the last of several reload messages, when
      // sent in quick succession, will work.
      _requestReload = true;
      _SZGClient.messageResponse( messageID, getLabel()+" reloading rendering parameters." );
    }
    else if ( messageType == "user" ) {
      _appendUserMessage( messageID, messageBody );
    }
    else if ( messageType == "color" ) {
      if ( messageBody == "NULL" || messageBody == "off") {
        _noDrawFillColor = arVector3( -1.0f, -1.0f, -1.0f );
      } else {
        float tmp[ 3 ];
        ar_parseFloatString( messageBody, tmp, 3 );
        // todo: error checking
        memcpy( _noDrawFillColor.v, tmp, 3 * sizeof( AR_FLOAT ) );
      }
    }
    else if( messageType == "screenshot" ) {
      // copypaste with graphics/szgrender.cpp
      if ( _dataPath == "NULL" ) {
        ar_log_warning() << "screenshot failed: undefined SZG_DATA/path.\n";
        _SZGClient.messageResponse( messageID, "ERROR: "+getLabel()+
            " screenshot failed: undefined SZG_DATA/path." );
      } else {
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
        _SZGClient.messageResponse( messageID, getLabel()+" took screenshot." );
      }
    }
    else if (( messageType == "demo" )||(messageType == "fixedhead")) {
      bool useFixedHead(messageBody=="on");
      setFixedHeadMode(useFixedHead);
      if (useFixedHead) {
        _SZGClient.messageResponse( messageID, getLabel()+" enabled fixed-head mode." );
      } else {
        _SZGClient.messageResponse( messageID, getLabel()+" disabled fixed-head mode." );
      }
    }

    //*********************************************************
    // There's quite a bit of copy-pasting between the
    // messages accepted by szgrender and here... how can we
    // reuse messaging functionality?????
    //*********************************************************
    if( messageType == "delay" ) {
      _framerateThrottle = messageBody == "on";
      if (_framerateThrottle) {
        _SZGClient.messageResponse( messageID, getLabel()+" enabled frame-rate throttle." );
      } else {
        _SZGClient.messageResponse( messageID, getLabel()+" disabled frame-rate throttle." );
      }
    }
    else if ( messageType == "pause" ) {
      if ( messageBody == "on" ) {
        _SZGClient.messageResponse( messageID, getLabel()+" pausing." );
        arGuard dummy(_pauseLock);
        if ( !stopping() )
          _pauseFlag = true;
      }
      else if( messageBody == "off" ) {
        _SZGClient.messageResponse( messageID, getLabel()+" continueing." );
        arGuard dummy(_pauseLock);
        _pauseFlag = false;
        _pauseVar.signal();
      }
      else {
        ar_log_warning() << 
            " ignoring unexpected pause arg '" << messageBody << "'.\n";
        _SZGClient.messageResponse( messageID, "ERROR: "+getLabel()+
            " ignoring unexpected pause arg '"+messageBody+"'." );
      }
    }

  }
}

// Handle connections.
void arMasterSlaveFramework::_connectionTask( void ) {
  _connectionThreadRunning = true;
  if( _master ) {

    while( !stopping() ) {
      // todo: nonblocking connection accept.
      // Workaround: pretend connection thread isn't running.
      _connectionThreadRunning = false;
      arSocket* theSocket = _stateServer->acceptConnectionNoSend();
      _connectionThreadRunning = true;

      if (stopping())
        break;

      if( !theSocket || _stateServer->getNumberConnected() <= 0 ) {
        // Something bad happened.  Don't retry infinitely.
        _exitProgram = true;
	break;
      }

      // getNumberConnected before ar_log, so ar_log's output doesn't fragment.
      const int n = _stateServer->getNumberConnected();
      ar_log_remark() << "master got slave #" << n << ".\n";
    }

  }
  else {
    // slave

    while( !stopping() ) {
      arSleepBackoff a(40, 100, 1.1);
      while( !_barrierClient->checkConnection() && !stopping() ) {
	a.sleep();
      }
      a.reset();
      // Barrier is connected.

      if (stopping())
        break;

      // hack, since discoverService currently blocks
      // and arSZGClient has no shutdown mechanism
      // (this _fooThreadRunning = false; discoverService; _foo=true hack
      // is also in arBarrierClient::_connectionTask.)
      _connectionThreadRunning = false;
      const arPhleetAddress result(_SZGClient.discoverService(_serviceName, _networks, true));
      if (stopping())
        break;
      _connectionThreadRunning = true;

      ar_log_remark() << "slave connecting to "
	              << result.address << ":" << result.portIDs[0] << ".\n";

      if( !result.valid ||
          !_stateClient.dialUpFallThrough( result.address, result.portIDs[ 0 ] ) ) {
        ar_log_warning() << "failed to broker; retrying.\n";
        continue;
      }

      // Bond the appropriate data channel to this sync channel.
      _barrierClient->setBondedSocketID( _stateClient.getSocketIDRemote() );
      _stateClientConnected = true;

      ar_log_remark() << "slave connected.\n";
      while( _stateClientConnected && !stopping() ) {
        ar_usleep( 200000 );
      }

      if (stopping())
        break;

      ar_log_remark() << "slave disconnected.\n";
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
    ar_log_warning() << "_drawWindow got NULL pointer.\n";
    return;
  }

  int currentWinID = windowInfo->getWindowID();
  if( !_wm->windowExists( currentWinID ) ) {
    ar_log_warning() << "_drawWindow: no arGraphicsWindow with ID " <<
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
