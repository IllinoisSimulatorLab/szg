//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arGUIComponentFramework.h"
#include "arEventUtilities.h"
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

void ar_guiComponentFrameworkMessageTask( void* p ) {
  ((arGUIComponentFramework*) p)->_messageTask();
}

void ar_guiComponentFrameworkWindowEventFunction( arGUIWindowInfo* windowInfo ) {
  if ( windowInfo && windowInfo->getUserData() ) {
    ((arGUIComponentFramework*) windowInfo->getUserData())->onWindowEvent( windowInfo );
  }
}

void ar_guiComponentFrameworkWindowInitGLFunction( arGUIWindowInfo* windowInfo ) {
  if ( windowInfo && windowInfo->getUserData() ) {
    ((arGUIComponentFramework*) windowInfo->getUserData())->onWindowStartGL( windowInfo );
  }
}

void ar_guiComponentFrameworkKeyboardFunction( arGUIKeyInfo* keyInfo ) {
  if ( !keyInfo || !keyInfo->getUserData() )
    return;

  arGUIComponentFramework* fw = (arGUIComponentFramework*) keyInfo->getUserData();
  if ( fw->stopping() ) {
    // Shutting down.  Ignore keystrokes.
    return;
  }

  if ( keyInfo->getState() == AR_KEY_DOWN ) {
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
      case AR_VK_t:
        ar_log_critical() << "arGUIComponentFramework frame time = " <<
          fw->_lastFrameTime << " msec\n";
        break;
    }
  }

  // Forward keyboard events to the keyboard callback, if it's defined.
  fw->onKey( keyInfo );
}

void ar_guiComponentFrameworkMouseFunction( arGUIMouseInfo* mouseInfo ) {
  if ( !mouseInfo || !mouseInfo->getUserData() )
    return;

  arGUIComponentFramework* fw = (arGUIComponentFramework*) mouseInfo->getUserData();
  fw->onMouse( mouseInfo );
}

//***********************************************************************
// arGraphicsWindow callback classes
//***********************************************************************
class arGUIComponentWindowInitCallback : public arWindowInitCallback {
  public:
    arGUIComponentWindowInitCallback( arGUIComponentFramework* fw ) :
       _framework( fw ) {}
    ~arGUIComponentWindowInitCallback( void ) {}
    void operator()( arGraphicsWindow& );
  private:
    arGUIComponentFramework* _framework;
};

void arGUIComponentWindowInitCallback::operator()( arGraphicsWindow& ) {
  if ( _framework ) {
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
// The call sequence is:
// 1. From the event loop, the window manager requests that all (arGUI)
//    windows draw.
// 2. Each window (possibly in its own display thread) then calls
//    its arGUI render callback. In a master/slave app, this is actually
//    a call to arGUIComponentRenderCallback::operator( arGUIWindowInfo* ).
// 3. arGUIComponentFramework::_drawWindow() is called. It draws
//    the whole window and then blocks until all OpenGL commands
//    have written to the frame buffer.
//    It calls the arGraphicsWindow draw. It also draws overlays
//    like the performance graph and the inputsimulator.
// 4. Inside the arGraphicsWindow draw, for each owned viewport, the graphics window's
//    render callback is called. In an m/s app, this is a call to
//    arGUIComponentRenderCallback::operator( arGraphicsWindow&, arViewport& ).
// 5. The arGUIComponentRenderCallback calls the framework's (virtual) onDraw method.
// 6. If the onDraw method hasn't been over-ridden, then the application's
//    installed draw callback function is used.
//****************************************************************************

class arGUIComponentRenderCallback : public arGUIRenderCallback {
  public:
    arGUIComponentRenderCallback( arGUIComponentFramework* fw ) :
      _framework( fw ) {}
    ~arGUIComponentRenderCallback( void ) {}

    void operator()( arGraphicsWindow&, arViewport& );
    void operator()( arGUIWindowInfo* ) { }
    void operator()( arGUIWindowInfo* windowInfo,
                     arGraphicsWindow* graphicsWindow );

  private:
    arGUIComponentFramework* _framework;
};

void arGUIComponentRenderCallback::operator()( arGraphicsWindow& win, arViewport& vp ) {
  if ( _framework ) {
    _framework->onDraw( win, vp );
  }
}

void arGUIComponentRenderCallback::operator()( arGUIWindowInfo* windowInfo,
                                              arGraphicsWindow* graphicsWindow ) {
  if (_framework) {
    _framework->_drawWindow( windowInfo, graphicsWindow );
  }
}

//***********************************************************************
// arGUIComponentFramework public methods
//***********************************************************************

arGUIComponentFramework::arGUIComponentFramework( void ):
  // Miscellany.
  _networks( "NULL" ),
  _connectionThreadRunning( false ),
  _useWindowing( false ),

  // Callbacks.
  _startCallback( NULL ),
  _preExchange( NULL ),
  _postExchange( NULL ),
  _windowInitCallback( NULL ),
  _drawCallback( NULL ),
  _windowEventCallback( NULL ),
  _windowStartGLCallback( NULL ),
  _cleanup( NULL ),
  _userMessageCallback( NULL ),
  _overlay( NULL ),
  _keyboardCallback( NULL ),
  _arGUIKeyboardCallback( NULL ),
  _mouseCallback( NULL ),

  // User messages.
  _noDrawFillColor( -1.0f, -1.0f, -1.0f ),
  _requestReload( false ) {

  // when the default color is set like this, the app's geometry is displayed
  // instead of a default color
  _masterPort[ 0 ] = -1;

  _showPerformance = false;

  // By default, the window manager is single-threaded.
  _wm = new arGUIWindowManager( ar_guiComponentFrameworkWindowEventFunction,
                                ar_guiComponentFrameworkKeyboardFunction,
                                ar_guiComponentFrameworkMouseFunction,
                                ar_guiComponentFrameworkWindowInitGLFunction,
                                false );

  // as a replacement for the __globalFramework pointer,
  // each window has a user data pointer set.
  _wm->setUserData( this );

  _guiXMLParser = new arGUIXMLParser( &_SZGClient );
}

// \bug memory leak for several pointer members
arGUIComponentFramework::~arGUIComponentFramework( void ) {
  delete _wm;
  delete _guiXMLParser;
}

// Initialize the syzygy objects, but does not start any threads
bool arGUIComponentFramework::init( int& argc, char** argv ) {
  int i;
  string currDir;

  if (!_okToInit(argv[0]))
    return false;

  // Connect to the szgserver.
  _SZGClient.simpleHandshaking( false );
  if (!_SZGClient.init( argc, argv )) {
    return false;
  }

  ar_log_debug() << ar_versionInfo() << ar_versionString();

  // Performance graphs.
  const arVector3 white  (1, 1, 1);
  const arVector3 yellow (1, 1, 0);
  const arVector3 cyan   (0, 1, 1);
  _framerateGraph.addElement( "      fps", 250,   100, white  );
  _framerateGraph.addElement( " cpu usec", 250, 10000, yellow );

  if ( !_SZGClient ) {
    if (!ar_getWorkingDirectory( currDir )) {
      ar_log_critical() << "Failed to get working directory.\n";
    } else {
      ar_log_critical() << "Directory: " << currDir << ar_endl;
    }
    
    if ( !_loadParameters() ) {
      ar_log_error() << "failed to load parameters while standalone.\n";
    }
    _initCalled = true;

    // Don't start the message-receiving thread yet (because there's no
    // way yet to operate in a distributed fashion).
    // Don't initialize the master's objects, since they'll be unused.
    return true;
  } else {
    for (i=0; i<argc; ++i) {
      if (!strcmp( argv[i], "-szgtype" )) {
         _SZGClient.initResponse() << "guicomponent\n";
         _SZGClient.sendInitResponse( true );
         exit(0);
      }
    }
  }

  if (!ar_getWorkingDirectory( currDir )) {
    ar_log_critical() << "Failed to get working directory.\n";
  } else {
    ar_log_critical() << "Directory: " << currDir << ar_endl;
  }

  // Connected to szgserver.  Not standalone.
  _framerateGraph.addElement( "sync usec", 250, 10000, cyan   );

  // Load the parameters before executing any user callbacks,
  // because this determines if we're master or slave).
  if ( !_loadParameters() ) {
    goto fail;
  }

  // In trigger mode, get virtual computer info from the arAppLauncher object.
  // That requires that the arSZGClient be registered with the arAppLauncher.
  //
  // If we launch the other executables, this function might not return.

  // Tell _launcher about the virtual computer.
  (void)_launcher.setSZGClient( &_SZGClient );

  // We're not the trigger node.

  // So apps can query the virtual computer's attributes,
  // those attributes are not restricted to the master.
  // So setParameters() is called.
  // But we allow setting a bogus virtual computer via "-szg".
  // So it's not fatal if setParamters() fails, i.e. this should launch:
  //   my_program -szg virtual=bogus
  if ( _SZGClient.getVirtualComputer() != "NULL" && !_launcher.setParameters() ) {
    ar_log_error() << "invalid virtual computer definition.\n";
  }

  // Launch the message thread, so dkill works even if init() or start() fail.
  // But launch after the trigger code above, lest we catch messages in both
  // waitForKill() AND _messageThread.
  if (!_messageThread.beginThread( ar_guiComponentFrameworkMessageTask, this )) {
    ar_log_error() << "failed to start message thread.\n";
    goto fail;
  }

  _initCalled = true;
  if ( !_SZGClient.sendInitResponse( true ) ) {
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
void arGUIComponentFramework::stop( bool blockUntilDisplayExit ) {
  _blockUntilDisplayExit = blockUntilDisplayExit;

  // To avoid a race condition, set _exitProgram within this lock.
  _pauseLock.lock("arGUIComponentFramework::stop");
    _exitProgram = true;
    _pauseFlag   = false;
    _pauseVar.signal();
  _pauseLock.unlock();

  arSleepBackoff a(50, 100, 1.1);
  while ( _connectionThreadRunning ||
         ( _useWindowing && _displayThreadRunning && blockUntilDisplayExit ) ||
         ( _useExternalThread && _externalThreadRunning ) ) {
    a.sleep();
  }

  ar_log_remark() << "stopped.\n";
  _stopped = true;
}

bool arGUIComponentFramework::createWindows( bool useWindowing ) {
  std::vector< arGUIXMLWindowConstruct* >* windowConstructs =
    _guiXMLParser->getWindowingConstruct()->getWindowConstructs();

  if ( !windowConstructs ) {
    // print error?
    return false;
  }

  // Populate callbacks for the gui and graphics windows,
  // and the head for any vr cameras.
  std::vector< arGUIXMLWindowConstruct*>::iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); itr++ ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arGUIComponentWindowInitCallback( this ) );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arGUIComponentRenderCallback( this ) );
    (*itr)->setGUIDrawCallback( new arGUIComponentRenderCallback( this ) );

    std::vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    std::vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if ( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &_head );
      }
    }
  }

  // Create the windows if "using windowing", else just create placeholders.
  if ( _wm->createWindows( _guiXMLParser->getWindowingConstruct(), useWindowing ) < 0 ) {
    ar_log_error() << "failed to create windows.\n";
#ifdef AR_USE_DARWIN
    ar_log_error() << "  (Check that X11 is running.)\n";
#endif
    return false;
  }

  _wm->setAllTitles( _label, false );
  return true;
}

void arGUIComponentFramework::loopQuantum() {
  // Exchange data. Connect slaves to master and activate framelock.
  preDraw();
  draw();
  sync();

  // Synchronize.
  swap();

  // Process events from keyboard, window manager, etc.
  _wm->processWindowEvents();
}

void arGUIComponentFramework::exitFunction() {
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
void arGUIComponentFramework::preDraw( void ) {
  if (stopping())
    return;

  // Reload parameters in this thread,
  // since the arGUIWindowManager might be single threaded,
  // in which case all calls to it must be in that single thread.

  if (_requestReload) {
    (void) _loadParameters();
    // Assume reasonably that the windows will be recreated.
    (void) createWindows(_useWindowing);
    _requestReload = false;
  }

  const ar_timeval preDrawStart = ar_time();

  // Catch pause/ un-pause requests.
  _pauseLock.lock("arGUIComponentFramework::preDraw");
  while ( _pauseFlag ) {
    _pauseVar.wait( _pauseLock );
  }
  _pauseLock.unlock();

  // the pause might have been triggered by shutdown
  if (stopping())
    return;

  _processUserMessages();

  // Do this before the pre-exchange callback.
  // The user might want to use current input data via
  // arGUIComponentFramework::getButton() or some other method in that callback.
  // Frame rate bug: computed in _pollInputData, useless for slaves.
  _framerateGraph.getElement( "      fps" )->pushNewValue(1000. / _lastFrameTime);
  _framerateGraph.getElement( " cpu usec" )->pushNewValue(_lastComputeTime);
  if (!_standalone)
    _framerateGraph.getElement( "sync usec" )->pushNewValue(_lastSyncTime);

  // Get performance metrics.
  _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
}

// Public, so apps can make custom event loops.
void arGUIComponentFramework::draw( int windowID ) {
  if ( stopping() || !_wm )
    return;

  if ( windowID < 0 )
    _wm->drawAllWindows( true );
  else
    _wm->drawWindow( windowID, true );
}

// After the window is drawn, and before synchronizing.
void arGUIComponentFramework::sync( void ) {
  if ( stopping() || _standalone )
    return;

  const ar_timeval syncStart = ar_time();
  if ( _framerateThrottle ) {
    // Test sync.
    ar_usleep( 200000 );
  }

  if ( !_sync() ) {
    ar_log_error() << "_sync failed in sync().\n";
  }
  _lastSyncTime = ar_difftime( ar_time(), syncStart );
}

// Public, so apps can make custom event loops.
void arGUIComponentFramework::swap( int windowID ) {
  if (stopping() || !_wm)
    return;

  if ( windowID >= 0 )
    _wm->swapWindowBuffer( windowID, true );
  else
    _wm->swapAllWindowBuffers(true);
}

bool arGUIComponentFramework::onStart( arSZGClient& SZGClient ) {
  if ( _startCallback && !_startCallback( *this, SZGClient ) ) {
    ar_log_error() << "user-defined start callback failed.\n";
    return false;
  }

  return true;
}

void arGUIComponentFramework::_stop(const char* name, const arCallbackException& exc) {
  ar_log_error() << "arGUIComponentFramework " << name << " callback:\n\t" <<
    exc.message << "\n";
  stop(false);
}

void arGUIComponentFramework::onWindowInit( void ) {
  if ( _windowInitCallback ) {
    try {
      _windowInitCallback( *this );
    } catch (arCallbackException exc) {
      _stop("windowInit", exc);
    }
  }
  else {
    ar_defaultWindowInitCallback();
  }
}

// Only events that are actually generated by the GUI itself actually reach
// here, not for instance events that we post to the window manager ourselves.
void arGUIComponentFramework::onWindowEvent( arGUIWindowInfo* wI ) {
  if ( wI ) {
    if (_windowEventCallback ) {
      try {
        _windowEventCallback( *this, wI );
      } catch (arCallbackException exc) {
        _stop("windowEvent", exc);
      }
    } else if ( wI->getUserData() ) {
      // default window event handler, at least to handle a resizing event
      // (should we be handling window close events as well?)
      arGUIComponentFramework* fw = (arGUIComponentFramework*) wI->getUserData();

      if ( !fw ) {
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

void arGUIComponentFramework::onWindowStartGL( arGUIWindowInfo* windowInfo ) {
  if ( windowInfo && _windowStartGLCallback ) {
    try {
      _windowStartGLCallback( *this, windowInfo );
    } catch (arCallbackException exc) {
      _stop("windowStartGL", exc);
    }
  }
}

// Yes, this is really the application-provided draw function. It is
// called once per viewport of each arGraphicsWindow (arGUIWindow).
// It is a virtual method that issues the user-defined draw callback.
void arGUIComponentFramework::onDraw( arGraphicsWindow& win, arViewport& vp ) {
  if ( (!_oldDrawCallback) && (!_drawCallback) ) {
    ar_log_error() << "forgot to setDrawCallback().\n";
    return;
  }

  if (_drawCallback) {
    try {
      _drawCallback( *this, win, vp );
    } catch (arCallbackException exc) {
      _stop("draw", exc);
    }
  }
}

void arGUIComponentFramework::onCleanup( void ) {
  if ( _cleanup ) {
    try {
      _cleanup( *this );
    } catch (arCallbackException exc) {
//      _stop("cleanup", exc);
    }
  }
}

void arGUIComponentFramework::onUserMessage( const int messageID, const string& messageBody ) {
  if (_userMessageCallback) {
    try {
      _userMessageCallback( *this, messageID, messageBody );
    } catch (arCallbackException exc) {
      _stop("userMessage", exc);
    }
  }
}

void arGUIComponentFramework::onOverlay( void ) {
  if ( _overlay ) {
    try {
      _overlay( *this );
    } catch (arCallbackException exc) {
      _stop("overlay", exc);
    }
  }
}

void arGUIComponentFramework::onKey( arGUIKeyInfo* keyInfo ) {
  if ( !keyInfo ) {
    return;
  }

  // Prefer newer to legacy type of keyboard callback.
  if ( _arGUIKeyboardCallback ) {
    try {
      _arGUIKeyboardCallback( *this, keyInfo );
    } catch (arCallbackException exc) {
      _stop("keyboard", exc);
    }
  } else if ( keyInfo->getState() == AR_KEY_DOWN ) {
    // Legacy behavior: expect only key presses, not releases.
    onKey( keyInfo->getKey(), 0, 0 );
  }
}

void arGUIComponentFramework::onKey( unsigned char key, int x, int y) {
  if ( _keyboardCallback ) {
    try {
      _keyboardCallback( *this, key, x, y );
    } catch (arCallbackException exc) {
      _stop("keyboard", exc);
    }
  }
}

void arGUIComponentFramework::onMouse( arGUIMouseInfo* mouseInfo ) {
  if ( mouseInfo && _mouseCallback ) {
    try {
      _mouseCallback( *this, mouseInfo );
    } catch (arCallbackException exc) {
      _stop("mouse", exc);
    }
  }
}

void arGUIComponentFramework::setStartCallback
  ( bool (*startCallback)( arGUIComponentFramework&, arSZGClient& ) ) {
  _startCallback = startCallback;
}

// The window callback is called once per window, per frame, if set.
// Otherwise the _drawSetUp method is called. This callback
// is needed since we may want several different views in a single window,
// with a draw callback filling each. This is especially useful for passive
// stereo from a single box, but will not work if the draw callback includes
// code that clears the entire buffer. Hence such code needs to moved
// into this sort of callback.
void arGUIComponentFramework::setWindowCallback
  ( void (*windowCallback)( arGUIComponentFramework& ) ) {
  _windowInitCallback = windowCallback;
}

void arGUIComponentFramework::setWindowEventCallback
  ( void (*windowEvent)( arGUIComponentFramework&, arGUIWindowInfo* ) ) {
  _windowEventCallback = windowEvent;
}

void arGUIComponentFramework::setWindowStartGLCallback
  ( void (*windowStartGL)( arGUIComponentFramework&, arGUIWindowInfo* ) ) {
  _windowStartGLCallback = windowStartGL;
}

void arGUIComponentFramework::setDrawCallback(
              void (*draw)( arGUIComponentFramework&, arGraphicsWindow&, arViewport& ) ) {
  ar_log_remark() << "set draw callback.\n";
  _drawCallback = draw;
}

void arGUIComponentFramework::setExitCallback
  ( void (*cleanup)( arGUIComponentFramework& ) ) {
  _cleanup = cleanup;
}

// Syzygy messages currently consist of two strings, the first being
// a type and the second being a value. The user can send messages
// to the arGUIComponentFramework and the application can trap them
// using this callback. A message w/ type "user" and value "foo" will
// be passed into this callback, if set, with "foo" going into the string.
void arGUIComponentFramework::setUserMessageCallback
  ( void (*userMessageCallback)(arGUIComponentFramework&,
                                const int messageID,
                                const string& messageBody )) {
  _userMessageCallback = userMessageCallback;
}

// In general, the graphics window can be a complicated sequence of
// viewports (as is necessary for simulating a CAVE or Cube). Sometimes
// the application just wants to draw something once, which is the
// purpose of this callback.
void arGUIComponentFramework::setOverlayCallback
  ( void (*overlay)( arGUIComponentFramework& ) ) {
  _overlay = overlay;
}

// A master instance will also take keyboard input via this callback,
// if defined.
void arGUIComponentFramework::setKeyboardCallback
  ( void (*keyboard)( arGUIComponentFramework&, unsigned char, int, int ) ) {
  _keyboardCallback = keyboard;
}

void arGUIComponentFramework::setKeyboardCallback
  ( void (*keyboard)( arGUIComponentFramework&, arGUIKeyInfo* ) ) {
  _arGUIKeyboardCallback = keyboard;
}

void arGUIComponentFramework::setMouseCallback
  ( void (*mouse)( arGUIComponentFramework&, arGUIMouseInfo* ) ) {
  _mouseCallback = mouse;
}

//************************************************************************
// functions pertaining to user messages
//************************************************************************
void arGUIComponentFramework::_processUserMessages() {
  arGuard _( _userMessageLock, "arGUIComponentFramework::_processUserMessages" );
  std::deque< arUserMessageInfo >::const_iterator iter;
  for (iter = _userMessageQueue.begin(); iter != _userMessageQueue.end(); ++iter) {
    onUserMessage( iter->messageID, iter->messageBody );
  }
  _userMessageQueue.clear();
}

//************************************************************************
// functions pertaining to starting the application
//************************************************************************

bool arGUIComponentFramework::_startSlaveObjects() {
  if ( !_barrierClient->start() ) {
    ar_log_error() << "slave failed to start barrier client.\n";
    return false;
  }

  ar_log_debug() << "slave's objects started.\n";
  return true;
}

bool arGUIComponentFramework::start() {
  return start(true, true);
}

bool arGUIComponentFramework::start( bool useWindowing, bool useEventLoop ) {
  if (!_okToStart())
    return false;

  _useWindowing = useWindowing;
  if ( !_standalone ) {
    // Get this lock only after an app launches;
    // not in init() which is called even on the trigger instance,
    // but here, which only render instances reach.
    const string screenLock =
      _SZGClient.getComputerName() + "/" + _SZGClient.getMode( "graphics" );
    int graphicsID = -1;
    if ( !_SZGClient.getLock( screenLock, graphicsID ) ) {
      return _startrespond( "failed to get screen resource held by component " +
        ar_intToString(graphicsID) + ".\n(dkill that component to proceed.)" );
    }
  }

  // User-defined init.  After _startDetermineMaster(),
  // so _startCallback knows if this instance is master or slave.
  if ( !onStart( _SZGClient ) ) {
    return _SZGClient &&
      _startrespond( "arGUIComponentFramework start callback failed." );
  }

  if (!createWindows(_useWindowing)) {
    return false;
  }

  _startCalled = true;
  if (_SZGClient) {
    if (!_SZGClient.sendStartResponse( true ) ) {
      cerr << _label << ": maybe szgserver died.\n";
    }
  } else {
    cout << _SZGClient.startResponse().str() << endl;
  }

  if ( !_standalone ) {
    _wm->findFramelock();
  }

  if (useEventLoop || _useWindowing) {
    // _displayThreadRunning coordinates shutdown with
    // program stop via ESC-press (arGUI keyboard callback)
    // or kill message (received in the message thread).
    _displayThreadRunning = true;
  }

  if ( useEventLoop ) {
    // Internal event loop.
    while ( !stopping() ) {
      loopQuantum();
    }
    // A kill point ("quit" message, click on window close button, hit ESC) called stop().
    exitFunction();
    exit(0);
  }

  // External event loop.  Caller should now repeatedly call e.g. loopQuantum().
  return true;
}

bool arGUIComponentFramework::_startrespond( const string& s ) {
  ar_log_error() << "" << s << ar_endl;

  if ( !_SZGClient.sendStartResponse( false ) ) {
    cerr << _label << ": maybe szgserver died.\n";
  }

  return false;
}

//**************************************************************************
// Other system-level functions
//**************************************************************************

bool arGUIComponentFramework::_loadParameters( void ) {
  ar_log_debug() << "reloading parameters.\n";

  // Set window-wide attributes based on the display name, like
  // stereo, window size, window position, framelock.

  _guiXMLParser->setDisplayName(_SZGClient.getDisplayName(_SZGClient.getMode("graphics")));
  if (!_guiXMLParser->parse()) {
    return false;
  }

  return true;
}


void arGUIComponentFramework::_messageTask( void ) {
  // todo: cleanly shutdown both this and arSZGClient.
  string messageType, messageBody;

  while ( !stopping() ) {
    const int messageID = _SZGClient.receiveMessage( &messageType, &messageBody );
    if (!messageID) {
      // szgserver has *hard-shutdown* _SZGClient.

      // 5-line copypaste from case "quit" below.
      // Block until the display thread finishes.
      stop( true );
      if ( !_useExternalThread ) {
        exit(0);
      }

      // External thread will exit().  End message task, since it's disconnected.
      return;
    }

    if ( messageType == "quit" ) {
      _SZGClient.messageResponse( messageID, getLabel()+" quitting" );
      // Halt everything gracefully, lest crashing windows apps
      // show a dialog box that must be clicked.

      // Block until the display thread finishes.
      stop( true );
      if ( !_useExternalThread ) {
        exit( 0 );
      }
      // External thread will exit().
    }
    else if (messageType=="log") {
      if (ar_setLogLevel( messageBody )) {
        _SZGClient.messageResponse( messageID, getLabel()+" set loglevel to "+messageBody );
      } else {
        _SZGClient.messageResponse( messageID, "ERROR: "+getLabel()+
            " ignoring unrecognized loglevel '"+messageBody+"'." );
      }
    }
    else if ( messageType== "performance" ) {
      _showPerformance = messageBody == "on";
      _SZGClient.messageResponse( messageID,
        getLabel() + (_showPerformance ? " show" : " hid") + "ing performance graph" );
    }
    else if ( messageType == "reload" ) {
      // Hack: set _requestReload here, to make the
      // event loop reload over there and then reset _requestReload.
      // Side effect: only the last of several reload messages, when
      // sent in quick succession, will work.
      _requestReload = true;
      _SZGClient.messageResponse( messageID, getLabel()+" reloading rendering parameters." );
    }
    else if ( messageType == "display_name" ) {
      _SZGClient.messageResponse( messageID, _SZGClient.getMode("graphics")  );
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
    else if ( messageType == "key" ) {
      string::const_iterator siter;
      for (siter = messageBody.begin(); siter != messageBody.end(); ++siter) {
        onKey( *siter, 0, 0 );
      }
    }

    //*********************************************************
    // There's quite a bit of copy-pasting between the
    // messages accepted by szgrender and here... how can we
    // reuse messaging functionality?????
    //*********************************************************
    else if ( messageType == "pause" ) {
      if ( messageBody == "on" ) {
        _SZGClient.messageResponse( messageID, getLabel()+" pausing." );
        arGuard _(_pauseLock, "arGUIComponentFramework::_messageTask pause on");
        if ( !stopping() )
          _pauseFlag = true;
      }
      else if ( messageBody == "off" ) {
        _SZGClient.messageResponse( messageID, getLabel()+" unpausing." );
        arGuard _(_pauseLock, "arGUIComponentFramework::_messageTask pause off");
        _pauseFlag = false;
        _pauseVar.signal();
      }
      else {
        ar_log_error() << " ignoring unexpected pause arg '" << messageBody << "'.\n";
        _SZGClient.messageResponse( messageID, "ERROR: "+getLabel()+
            " ignoring unexpected pause arg '"+messageBody+"'." );
      }
    }
  }
}

//**************************************************************************
// Functions directly pertaining to drawing
//**************************************************************************

// Display a whole arGUIWindow.
// The definition of arGUIComponentRenderCallback explains how arGUIWindow calls this.
void arGUIComponentFramework::_drawWindow( arGUIWindowInfo* windowInfo,
                                          arGraphicsWindow* graphicsWindow ) {
  if ( !windowInfo || !graphicsWindow ) {
    ar_log_error() << "_drawWindow ignoring NULL pointer.\n";
    return;
  }

  int currentWinID = windowInfo->getWindowID();
  if ( !_wm->windowExists( currentWinID ) ) {
    ar_log_error() << "_drawWindow: no arGraphicsWindow with ID " << currentWinID << ".\n";
    return;
  }

  // draw the window
  if (!stopping() ) {
    if ( _noDrawFillColor[ 0 ] == -1 ) {
      if ( getConnected() && !(_harmonyInUse && !_harmonyReady) ) {
        graphicsWindow->draw();

        if ( _wm->isFirstWindow( currentWinID ) ) {
          if ( _standalone && _standaloneControlMode == "simulator"
              && _showSimulator) {
            _simPtr->drawWithComposition();
          }

          if ( _showPerformance ) {
            _framerateGraph.drawWithComposition();
          }

          if ( getMaster() ) {
	    // Draw the application's overlay last, if such exists.
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
