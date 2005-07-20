//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#ifndef AR_USE_WIN_32
  #include <sys/types.h>
  #include <signal.h>
#endif
#include "arSharedLib.h"
#include "arEventUtilities.h"
#include "arPForthFilter.h"
#include "arVRConstants.h"
#include "arGUIWindow.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"


/// to make the callbacks startable from GLUT, we need this
/// global variable to pass in an arMasterSlaveFramework parameter
// arMasterSlaveFramework* __globalFramework = NULL;

//***********************************************************************
// GLUT callbacks
//***********************************************************************

void ar_masterSlaveFrameworkMessageTask( void* p ) {
  ((arMasterSlaveFramework*) p)->_messageTask();
}

void ar_masterSlaveFrameworkConnectionTask( void* p ) {
  ((arMasterSlaveFramework*) p)->_connectionTask();
}

/*
void ar_masterSlaveFrameworkReshapeFunction(int width, int height){
  if (!__globalFramework)
    return;

  __globalFramework->_windowSizeX = width;
  __globalFramework->_windowSizeY = height;
  __globalFramework->onReshape( width, height );
}

void ar_masterSlaveFrameworkDisplayFunction(){
  if (__globalFramework)
    __globalFramework->_drawWindow();
}
*/

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
  if( !keyInfo || !keyInfo->getUserData() ) {
    return;
  }

  arMasterSlaveFramework* fw = (arMasterSlaveFramework*) keyInfo->getUserData();

  if( fw->_exitProgram ) {
    // do not process key strokes after we have begun shutdown
    return;
  }

  if( keyInfo->getState() == AR_KEY_DOWN ) {
    switch( keyInfo->getKey() ) {
      case AR_VK_ESC:
          // We do not block until the display thread is done... but we do
          // block on everything else.
          // NOTE: we do not exit(0) here. Instead, that occurs in the display
          // thread!
          fw->stop( false );
      break;
      case AR_VK_f:
        // glutFullScreen();
        fw->_wm->fullscreenWindow( keyInfo->getWindowID() );
      break;
      case AR_VK_F:
        // glutReshapeWindow(600,600);
        fw->_wm->resizeWindow( keyInfo->getWindowID(), 600, 600 );
      break;
      case AR_VK_P:
        fw->_showPerformance = !fw->_showPerformance;
      break;
      case AR_VK_t:
        std::cout << "Frame time = " << fw->_lastFrameTime << std::endl;
      break;
    }

    // in standalone mode, keyboard events should also go to the interface
    if( fw->_standalone &&
        fw->_standaloneControlMode == "simulator" ) {
      fw->_simulator.keyboard( keyInfo->getKey(), 1, 0, 0 );
    }
  }

  // finally, keyboard events should be forwarded to the keyboard callback
  // *if* we are the master and *if* the callback has been defined.
  if( fw->getMaster() ) {
    fw->onKey( keyInfo );
  }
}

void ar_masterSlaveFrameworkMouseFunction( arGUIMouseInfo* mouseInfo ) {
  if( !mouseInfo || !mouseInfo->getUserData() ) {
    return;
  }

  arMasterSlaveFramework* fw = (arMasterSlaveFramework*) mouseInfo->getUserData();

  if( fw->_standalone &&
      fw->_standaloneControlMode == "simulator" ) {
    if( mouseInfo->getState() == AR_MOUSE_DOWN || mouseInfo->getState() == AR_MOUSE_UP ) {
      int whichButton = ( mouseInfo->getButton() == AR_LBUTTON ) ? 0 :
                        ( mouseInfo->getButton() == AR_MBUTTON ) ? 1 :
                        ( mouseInfo->getButton() == AR_RBUTTON ) ? 2 : 0;
      int whichState = ( mouseInfo->getState() == AR_MOUSE_DOWN ) ? 1 : 0;

      fw->_simulator.mouseButton( whichButton, whichState, mouseInfo->getPosX(), mouseInfo->getPosY() );
    }
    else {
      fw->_simulator.mousePosition( mouseInfo->getPosX(), mouseInfo->getPosY() );
    }
  }

  if( fw->getMaster() ) {
    fw->onMouse( mouseInfo );
  }
}

/*
void ar_masterSlaveFrameworkButtonFunction(int button, int state,
                                           int x, int y){
  // in standalone mode, button events should go to the interface
  if (__globalFramework->_standalone &&
      __globalFramework->_standaloneControlMode == "simulator"){
    // Must translate from the GLUT constants
    const int whichButton =
    (button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
    const int whichState = (state == GLUT_DOWN) ? 1 : 0;
    __globalFramework->_simulator.mouseButton(whichButton, whichState, x, y);
  }
}

void ar_masterSlaveFrameworkMouseFunction(int x, int y){
  // in standalone mode, mouse motion events should go to the interface
  if (__globalFramework->_standalone &&
      __globalFramework->_standaloneControlMode == "simulator"){
    __globalFramework->_simulator.mousePosition(x,y);
  }
}

/// ESC: quit, 'f': full screen, 'F': 600x600 window
void ar_masterSlaveFrameworkKeyboardFunction(unsigned char key, int x, int y){
  if (__globalFramework){
    // do not process key strokes after we have begun shutdown
    if (__globalFramework->_exitProgram){
      return;
    }
  }
  switch (key){
  case 27:
    if (__globalFramework){
      // We do not block until the display thread is done... but we do
      // block on everything else.
      __globalFramework->stop(false);
    }
    // NOTE: we do not exit(0) here. Instead, that occurs in the display
    // thread!
    break;
  case 'f':
    glutFullScreen();
    break;
  case 'F':
    glutReshapeWindow(600,600);
    break;
  case 'P':
    __globalFramework->_showPerformance = !__globalFramework->_showPerformance;
    break;
  case 't':
    cout << "Frame time = " << __globalFramework->_lastFrameTime << "\n";
    break;
  }

  // in standalone mode, keyboard events should also go to the interface
  if (__globalFramework->_standalone &&
      __globalFramework->_standaloneControlMode == "simulator"){
    __globalFramework->_simulator.keyboard(key, 1, x, y);
  }

  // finally, keyboard events should be forwarded to the keyboard callback
  // *if* we are the master and *if* the callback has been defined.
  if (__globalFramework->getMaster()) {
    __globalFramework->onKey( key, x, y );
  }
}
*/

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
    void operator()( arGUIWindowInfo* windowInfo ) { }
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
  _stateServer( NULL ),
  _transferTemplate( "data" ),
  _transferData( NULL ),
  // _glutDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA ),
  _userInitCalled( false ),
  _parametersLoaded( false ),
  _wm( NULL ),
  _guiXMLParser( NULL ),
  _serviceName( "NULL" ),
  _serviceNameBarrier( "NULL" ),
  _networks( "NULL" ),
  _startCallback( NULL ),
  _preExchange( NULL ),
  _postExchange( NULL ),
  _windowInitCallback( NULL ),
  _drawCallback( NULL ),
  _oldDrawCallback( NULL ),
  _disconnectDrawCallback( NULL ),
  _playCallback( NULL ),
  // _reshape(NULL),
  _windowEventCallback( NULL ),
  _windowStartGLCallback( NULL ),
  _cleanup( NULL ),
  _userMessageCallback( NULL ),
  _overlay( NULL ),
  _keyboardCallback( NULL ),
  _arGUIKeyboardCallback( NULL ),
  _mouseCallback( NULL ),
  _connectCallback( NULL ),
  // _stereoMode( false ),
  _internalBufferSwap( true ),
  _framerateThrottle( false ),
  _barrierServer( NULL ),
  _barrierClient( NULL ),
  _master( true ),
  _stateClientConnected( false ),
  _inputActive( false ),
  _soundActive( false ),
  _inBuffer( NULL ),
  _inBufferSize( -1 ),
  _newSlaveConnected( false ),
  _numSlavesConnected( 0 ),
  _time( 0.0 ),
  _lastFrameTime( 0.1 ),
  _firstTimePoll( true ),
  _randSeedSet( 1 ),
  _randomSeed( (long) -1 ),
  _newSeed( (long) -1 ),
  _numRandCalls( (long) 0 ),
  _lastRandVal( 0.0 ),
  _randSynchError( 0 ),
  _firstTransfer( 1 ),
  _screenshotFlag( false ),
  _screenshotStartX( 0 ),
  _screenshotStartY( 0 ),
  _screenshotWidth( 640 ),
  _screenshotHeight( 480 ),
  _whichScreenshot( 0 ),
  _pauseFlag( false ),
  _noDrawFillColor( -1.0f, -1.0f, -1.0f ),
  _connectionThreadRunning( false ),
  _useWindowing( false ),
  _standaloneControlMode( "simulator" ),
  _showPerformance( false ),
  _soundClient( NULL ) {

  ar_mutex_init( &_connectFlagMutex );
  ar_mutex_init( &_windowingMutex );

  // also need to add fields for our default-shared data
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
  ar_mutex_init( &_eventLock );

  // when the default color is set like this, the app's geometry is displayed
  // instead of a default color
  _masterPort[ 0 ] = -1;

  // Also, let's initialize the performance graph.
  _framerateGraph.addElement( "framerate",
                              300, 100, arVector3( 1.0f, 1.0f, 1.0f ) );
  _framerateGraph.addElement( "compute",
                              300, 100, arVector3( 1.0f, 1.0f, 0.0f ) );
  _framerateGraph.addElement( "sync",
                              300, 100, arVector3( 0.0f, 1.0f, 1.0f ) );
  _showPerformance = false;

  // _defaultCamera.setHead( &_head );
  // _graphicsWindow.setInitCallback( new arMasterSlaveWindowInitCallback( *this ) );
  // By default, the window manager will operate in single-threaded mode.
  _wm = new arGUIWindowManager( ar_masterSlaveFrameworkWindowEventFunction,
                                ar_masterSlaveFrameworkKeyboardFunction,
                                ar_masterSlaveFrameworkMouseFunction,
                                ar_masterSlaveFrameworkWindowInitGLFunction,
                                false );

  // as a replacement for the global __globalFramework pointer, each window 
  // will have a user data pointer set.
  _wm->setUserData( this );

  _guiXMLParser = new arGUIXMLParser( &_SZGClient );
}

/// \bug memory leak for several pointer members
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

bool arMasterSlaveFramework::onStart( arSZGClient& SZGClient ) {
  if( _startCallback && !_startCallback( *this, SZGClient ) ) {
    std::cerr << _label << " error: user-defined start callback failed."
              << std::endl;
    return false;
  }

  return true;
}

void arMasterSlaveFramework::onPreExchange( void ) {
  if ( _preExchange ) {
    _preExchange( *this );
  }
}

void arMasterSlaveFramework::onPostExchange( void ) {
  if( _postExchange ) {
    _postExchange( *this );
  }
}

void arMasterSlaveFramework::onWindowInit( void ) {
  if( _windowInitCallback ) {
    _windowInitCallback( *this );
  }
  else {
    ar_defaultWindowInitCallback();
  }
}

void arMasterSlaveFramework::onWindowEvent( arGUIWindowInfo* windowInfo ) {
  if( windowInfo && _windowEventCallback ) {
    _windowEventCallback( *this, windowInfo );
  }
  else if( windowInfo && windowInfo->getUserData() ) {
    // default window event handler, at least to handle a resizing event
    // (should we be handling window close events as well?)
    arMasterSlaveFramework* fw = (arMasterSlaveFramework*) windowInfo->getUserData();

    if( !fw ) {
      return;
    }

    switch( windowInfo->getState() ) {
      case AR_WINDOW_RESIZE:
        fw->_wm->setWindowViewport( windowInfo->getWindowID(), 0, 0,
                                    windowInfo->getSizeX(), windowInfo->getSizeY() );
      break;

      case AR_WINDOW_CLOSE:
        fw->stop( false );
      break;

      default:
      break;
    }
  }
}

void arMasterSlaveFramework::onWindowStartGL( arGUIWindowInfo* windowInfo ) {
  if( windowInfo && _windowStartGLCallback ) {
    _windowStartGLCallback( *this, windowInfo );
  }
}

/// Yes, this is really the application-provided draw function. It is
/// called once per viewport of each arGraphicsWindow (arGUIWindow).
/// It is a virtual method that issues the user-defined draw callback.
void arMasterSlaveFramework::onDraw( arGraphicsWindow& win, arViewport& vp ) {
  if( (!_oldDrawCallback) && (!_drawCallback) ) {
    std::cerr << _label << " warning: forgot to setDrawCallback()." << std::endl;
    return;
  }

  if (_drawCallback) {
    _drawCallback( *this, win, vp );
    return;
  }

  _oldDrawCallback( *this );
}

void arMasterSlaveFramework::onDisconnectDraw( void ) {
  if( _disconnectDrawCallback ) {
    _disconnectDrawCallback( *this );
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
    _playCallback( *this );
  }
}

/*
void arMasterSlaveFramework::onReshape( int width, int height ) {
  if( _reshape ) {
    _reshape( *this, width, height );
  }
  else {
    glViewport( 0, 0, width, height );
  }
}
*/

void arMasterSlaveFramework::onCleanup( void ) {
  if( _cleanup ) {
    _cleanup( *this );
  }
}

void arMasterSlaveFramework::onUserMessage( const std::string& messageBody ) {
  if( _userMessageCallback ) {
    _userMessageCallback( *this, messageBody );
  }
}

void arMasterSlaveFramework::onOverlay( void ) {
  if( _overlay ) {
    _overlay( *this );
  }
}

void arMasterSlaveFramework::onKey( arGUIKeyInfo* keyInfo ) {
  if( !keyInfo ) {
    return;
  }

  // if the 'newer' keyboard callback type is registered, use it instead of
  // the legacy version
  if( _arGUIKeyboardCallback ) {
    _arGUIKeyboardCallback( *this, keyInfo );
  }
  else if( keyInfo->getState() == AR_KEY_DOWN ) {
    // for legacy reasons, this call expects only key press
    // (not release) events
    onKey( keyInfo->getKey(), 0, 0 );
  }
}

void arMasterSlaveFramework::onKey( unsigned char key, int x, int y) {
  if( _keyboardCallback ) {
    _keyboardCallback( *this, key, x, y );
  }
}

void arMasterSlaveFramework::onMouse( arGUIMouseInfo* mouseInfo ) {
  if( mouseInfo && _mouseCallback ) {
    _mouseCallback( *this, mouseInfo );
  }
}

void arMasterSlaveFramework::onSlaveConnected( int numConnected ) {
  if( _connectCallback ) {
    _connectCallback( *this, numConnected );
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

/// The window callback is called once per window, per frame, if set.
/// If it is not set, the _drawSetUp method is called instead. This callback
/// is needed since we may want several different views in a single window,
/// with a draw callback filling each. This is especially useful for passive
/// stereo from a single box, but will not work if the draw callback includes
/// code that clears the entire buffer. Hence such code needs to moved
/// into this sort of callback.
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
  _drawCallback = draw;
}

void arMasterSlaveFramework::setDrawCallback( void (*draw)( arMasterSlaveFramework& ) ) {
  _oldDrawCallback = draw;
}

void arMasterSlaveFramework::setPlayCallback
  ( void (*play)( arMasterSlaveFramework& ) ) {
  _playCallback = play;
}

/*
void arMasterSlaveFramework::setReshapeCallback
  ( void (*reshape)( arMasterSlaveFramework&, int, int ) ) {
  _reshape = reshape;
}
*/

void arMasterSlaveFramework::setExitCallback
  ( void (*cleanup)( arMasterSlaveFramework& ) ) {
  _cleanup = cleanup;
}

/// Syzygy messages currently consist of two strings, the first being
/// a type and the second being a value. The user can send messages
/// to the arMasterSlaveFramework and the application can trap them
/// using this callback. A message w/ type "user" and value "foo" will
/// be passed into this callback, if set, with "foo" going into the string.
void arMasterSlaveFramework::setUserMessageCallback
  ( void (*userMessageCallback)(arMasterSlaveFramework&, const std::string& )){
  _userMessageCallback = userMessageCallback;
}

/// In general, the graphics window can be a complicated sequence of
/// viewports (as is necessary for simulating a CAVE or Cube). Sometimes
/// the application just wants to draw something once, which is the
/// purpose of this callback.
void arMasterSlaveFramework::setOverlayCallback
  ( void (*overlay)( arMasterSlaveFramework& ) ) {
  _overlay = overlay;
}

/// A master instance will also take keyboard input via this callback,
/// if defined.
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

void arMasterSlaveFramework::setSlaveConnectedCallback
  ( void (*connectCallback)( arMasterSlaveFramework& fw, int numConnected ) ) {
  _connectCallback = connectCallback;
}

bool arMasterSlaveFramework::_startrespond( const std::string& s ) {
  _SZGClient.startResponse() << _label << " error: " << s << std::endl;

  if( !_SZGClient.sendStartResponse( false ) ) {
    std::cerr << _label << " error: maybe szgserver died." << std::endl;
  }

  return false;
}

/// Initializes the syzygy objects, but does not start any threads
bool arMasterSlaveFramework::init( int& argc, char** argv ) {
  std::cout << "HERE: " << argv[ 0 ] << std::endl;
  _label = std::string( argv[ 0 ] );
    std::cout << "HEREa" << std::endl;
  _label = ar_stripExeName( _label );

  std::cout << "HEREb" << std::endl;

  // Connect to the szgserver.
  _SZGClient.simpleHandshaking( false );
  _SZGClient.init( argc, argv );

  std::cout << "HERE 0" << std::endl;

  if ( !_SZGClient ) {
    // HACK!!! For right now, if the arSZGClient fails to init, we assume
    // that we should try to run everything in "standalone" mode....
    // SO... we don't really want to return false here.
    std::cout << _label << " remark: RUNNING IN STANDALONE MODE. "
	            << "NO DISTRIBUTION." << std::endl;

    _standalone = true;

    // Furthermore, in standalone mode, this instance MUST be the master!
    _setMaster( true );

    // HACK!!!!! There's cutting-and-pasting here...
    dgSetGraphicsDatabase( &_graphicsDatabase );
    dsSetSoundDatabase( &_soundServer );

    // This MUST come before _loadParameters, so that the internal sound
    // client can be configured for the standalone configuration.
    // (The internal arSoundClient is created in _initStandaloneObjects).
    if( !_initStandaloneObjects() ) {
      // NOTE: It is definitely possible for initialization of the standalone
      // objects to fail. For instance, what if we are unable to load
      // the joystick driver (which is a loadable module) or what if
      // the configuration of the pforth filter fails?
      return false;
    }

    if( !_loadParameters() ) {
      std::cerr << _label << " remark: COULD NOT LOAD PARAMETERS IN STANDALONE "
	              << "MODE." << std::endl;
    }

    _parametersLoaded = true;

    // We do not start the message-receiving thread yet (because there's no
    // way yet to operate in a distributed fashion). We also do not
    // initialize the master's objects... since they will be unused!
    return true;
  }

  // the init responses need to go to the client's initResponse stream
  std::stringstream& initResponse = _SZGClient.initResponse();

  // Initialize a few things.
  dgSetGraphicsDatabase( &_graphicsDatabase );
  dsSetSoundDatabase( &_soundServer );

  std::cout << "HERE 1" << std::endl;

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

  std::cout << "HERE 2" << std::endl;

  if( _SZGClient.getMode( "default" ) == "trigger" ) {
    // if we are the trigger node, we launch the rest of the application
    // components... and then WAIT for the exit.
    string vircomp = _SZGClient.getVirtualComputer();

    // we are, in fact, executing as part of a virtual computer
    _vircompExecution = true;

    const string defaultMode = _SZGClient.getMode( "default" );

    initResponse << _label << " remark: executing on virtual computer "
	               << vircomp << ",\n    with default mode "
                 << defaultMode << "." << std::endl;

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

    // Reorganizes the virtual computer.
    if( !_launcher.launchApp() ) {
      initResponse << _label << " error: failed to launch on virtual computer "
	                 << vircomp << "." << std::endl;
      goto fail;
    }

    // Wait for the message (render nodes do this in the message task).
    // we've suceeded in initing
    if( !_SZGClient.sendInitResponse( true ) ) {
      std::cerr << _label << " error: maybe szgserver died." << std::endl;
    }

    // there's no more starting to do... since this application instance
    // is just used as a launcher for other instances
    _SZGClient.startResponse() << _label << " trigger launched components." << std::endl;

    if( !_SZGClient.sendStartResponse( true ) ) {
      std::cerr << _label << " error: maybe szgserver died." << std::endl;
    }

    (void)_launcher.waitForKill();
    exit( 0 );
  }

  std::cout << "HERE 3" << std::endl;

  // Mode isn't trigger.

  // NOTE: sometimes user programs need to be able to determine
  // characteristics of the virtual computer. Hence, that information must be
  // available (to EVERYONE... and not just the master!)
  // Consequently, setParameters() should be called. However, it is
  // desirable to allow the user to set a BOGUS virtual computer via the
  // -szg command line flags. Consequently, it is merely a warning and
  // NOT a fatal error if setParameters() fails... i.e.
  // my_program -szg virtual=foo
  // should launch
  if( _SZGClient.getVirtualComputer() != "NULL" &&
      !_launcher.setParameters() ) {
    initResponse << _label << " warning: invalid virtual computer definition." << std::endl;
  }

  // Launch the message thread here, so dkill still works
  // even if init() or start() fail.
  // But launch after the trigger code above, lest we catch messages both
  // in the waitForKill() AND in the message thread.
  if( !_messageThread.beginThread( ar_masterSlaveFrameworkMessageTask, this ) ) {
    initResponse << _label << " error: failed to start message thread." << std::endl;
    goto fail;
  }

  if( !_determineMaster( initResponse ) ) {
    goto fail;
  }

  std::cout << "HERE 4" << std::endl;

  // init the objects, either master or slave
  if( getMaster() ) {
    if( !_initMasterObjects( initResponse ) ) {
      goto fail;
    }
  }
  else {
    if( !_initSlaveObjects( initResponse ) ) {
fail:
      if( !_SZGClient.sendInitResponse( false ) ) {
        std::cerr << _label << " error: maybe szgserver died." << std::endl;
      }

      return false;
    }
  }

  _parametersLoaded = true;

  if( !_SZGClient.sendInitResponse( true ) ) {
    std::cerr << _label << " error: maybe szgserver died." << std::endl;
  }

  return true;
}

/// Starts the application, using the GLUT event loop. NOTE: this call does
/// return if successful, since glutMainLoop() does not return.
bool arMasterSlaveFramework::start( void ) {
  // call _start and tell it that, yes, we are using GLUT.
  // AN ANNOYING HACK
  return _start( true );
}

/// Starts the application, but does not create a window or start the GLUT
/// event loop.
bool arMasterSlaveFramework::startWithoutWindowing( void ){
  // call _start and tell it that, no, we are not using GLUT
  // AN ANNOYING HACK
  return _start( false );
}

/// Begins halting many significant parts of the object and blocks until
/// they have, indeed, halted. The parameter will be false if we have been
/// called from the GLUT keyboard function (that way be will not wait for
/// the display thread to finish, resulting in a deadlock, since the
/// keyboard function is called from that thread). The parameter will be
/// true, on the other hand, if we are calling from the message-receiving
/// thread (which is different from the display thread, and there the
/// stop(...) should not return until the display thread is done.
/// THIS DOES NOT HALT EVERYTHING YET! JUST THE
/// STUFF THAT SEEMS TO CAUSE SEGFAULTS OR OTHER PROBLEMS ON EXIT.
void arMasterSlaveFramework::stop( bool blockUntilDisplayExit ) {

  _blockUntilDisplayExit = blockUntilDisplayExit;

  if( _vircompExecution)  {
    // arAppLauncher object piggy-backing on a renderer execution
    _launcher.killApp();
  }

  // to avoid an uncommon race condition setting the _exitProgram
  // flag must go within this lock
  ar_mutex_lock( &_pauseLock );
  _exitProgram = true;
  _pauseFlag   = false;
  _pauseVar.signal();
  ar_mutex_unlock( &_pauseLock );

  // recall that we can be a master AND not have any distribution occuring
  if( getMaster() ) {
    // it IS NOT valid to combine this conditional w/ the above because
    // of the else
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

  std::cout << _label << " remark: done." << std::endl;

  _stopped = true;
}

/// The sound server should be able to find its files in the application
/// directory. If this function is called between init(...) and start(...),
/// the sound render will be able to find clips there. bundlePathName should
/// be SZG_PYTHON or SZG_DATA and bundleSubDirectory will likely (but not necessarily)
/// be the name of the app.
void arMasterSlaveFramework::setDataBundlePath( const std::string& bundlePathName,
                                                const std::string& bundleSubDirectory ) {
  _soundServer.setDataBundlePath( bundlePathName, bundleSubDirectory );

  // For standalone mode, we also need to set-up the internal sound client.
  // Only in this case will the _soundClient ptr be non-NULL.
  if( _soundClient ) {
    _soundClient->setDataBundlePath( bundlePathName, bundleSubDirectory );
  }
}

/// public method so that applications can make custom event loops
void arMasterSlaveFramework::draw( int windowID ) {

  if( _exitProgram || !_wm ) {
    return;
  }

  if( windowID < 0 ) {
    _wm->drawAllWindows();
  }
  else {
    _wm->drawWindow( windowID );
  }
}

/// public method so that applications can make custom event loops
void arMasterSlaveFramework::swap( int windowID ) {

  if( _exitProgram || !_wm ) {
    return;
  }

  if( windowID < 0 ) {
    _wm->swapAllWindowBuffers();
  }
  else {
    _wm->swapWindowBuffer( windowID );
  }
}

/// The sequence of events that should occur before the window is drawn.
/// Made available as a public method so that applications can create custom
/// event loops.
void arMasterSlaveFramework::preDraw( void ) {
  // don't even start the function if we are, in fact, in shutdown mode
  if( _exitProgram ) {
    return;
  }

  // want to time this function...
  ar_timeval preDrawStart = ar_time();

  // Catch the pause/ un-pause requests.
  ar_mutex_lock( &_pauseLock );
  while( _pauseFlag ) {
    _pauseVar.wait( &_pauseLock );
  }
  ar_mutex_unlock( &_pauseLock );

  // the pause might have been triggered because shutdown has started
  if( _exitProgram ) {
    return;
  }

  // important that this occurs before the pre-exchange callback
  // the user might want to use current input data via
  // arMasterSlaveFramework::getButton() or some other method in that callback
  if( getMaster() ) {
    _pollInputData();

    if( _eventFilter ) {
      _eventFilter->processEventQueue();
    }

    ar_mutex_lock( &_connectFlagMutex );
    if( _newSlaveConnected ) {
      onSlaveConnected( _numSlavesConnected );
      _newSlaveConnected = false;
    }
    ar_mutex_unlock( &_connectFlagMutex );

    onPreExchange();
  }

  // might not be running in a distributed fashion
  if( !_standalone ) {
    if( !( getMaster() ? _sendData() : _getData() ) ) {
      _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
      return;
    }
  }

  onPlay();

  if( getConnected() ) {
    onPostExchange();
  }

  if( _standalone ) {
    // play the sounds locally
    _soundClient->_cliSync.consume();
  }

  // must let the framerate graph know about the current frametime.
  // NOTE: this is computed in _pollInputData... hmmm... doesn't make this
  // very useful for the slaves...
  arPerformanceElement* framerateElement = _framerateGraph.getElement( "framerate" );
  framerateElement->pushNewValue( 1000.0 / _lastFrameTime );

  // compute time is in microseconds and the graph element is scaled to 100 ms.
  arPerformanceElement* computeElement   = _framerateGraph.getElement( "compute" );
  computeElement->pushNewValue( _lastComputeTime / 1000.0 );

  // sync time is in microseconds and the graph element is scaled to 100 ms.
  arPerformanceElement* syncElement = _framerateGraph.getElement( "sync" );
  syncElement->pushNewValue(_lastSyncTime / 1000.0 );

  // must get performance metrics
  _lastComputeTime = ar_difftime( ar_time(), preDrawStart );
}


/// The sequence of events that should occur after the window is drawn,
/// but before the synchronization is called.
void arMasterSlaveFramework::postDraw( void ){
  // if shutdown has been triggered, just return
  if( _exitProgram ) {
    return;
  }

  ar_timeval postDrawStart = ar_time();

  // sometimes, for testing of synchronization, we want to be able to throttle
  // down the framerate
  if( _framerateThrottle ) {
    ar_usleep( 200000 );
  }
  else {
    // NEVER, NEVER, NEVER, NEVER PUT A SLEEP IN THE MAIN LOOP
    // CAN'T COUNT ON THIS BEING LESS THAN 10 MS ON SOME SYSTEMS!!!!!
    //ar_usleep(2000);
  }

  // the synchronization. NOTE: we DO NOT synchronize if we are standalone.
  if( !_standalone && !_sync() ) {
    std::cerr << _label << " warning: sync failed." << std::endl;
  }

  _lastSyncTime = ar_difftime( ar_time(), postDrawStart );
}

/*
arMatrix4 arMasterSlaveFramework::getProjectionMatrix(float eyeSign){
  _defaultCamera.setEyeSign( eyeSign );
  return _defaultCamera.getProjectionMatrix();
}

arMatrix4 arMasterSlaveFramework::getModelviewMatrix(float eyeSign){
  _defaultCamera.setEyeSign( eyeSign );
  return _defaultCamera.getModelviewMatrix();
}
*/

bool arMasterSlaveFramework::addTransferField( std::string fieldName, void* data,
                                               arDataType dataType, int size ) {
  if( _userInitCalled ) {
    std::cerr << _label << " warning: ignoring addTransferField() after init()." << std::endl;
    return false;
  }

  if( size <= 0 ) {
    std::cerr << _label << " warning: ignoring addTransferField() with size "
              << size << "." << std::endl;
    return false;
  }

  if( !data ) {
    std::cerr << _label
              << " warning: ignoring addTransferField() with NULL data ptr." << std::endl;
    return false;
  }

  const std::string realName = "USER_" + fieldName;
  if( _transferTemplate.getAttributeID( realName ) >= 0 ) {
    std::cerr << _label
              << " warning: ignoring addTransferField() with duplicate name." << std::endl;
    return false;
  }

  _transferTemplate.addAttribute( realName, dataType );

  const arTransferFieldDescriptor descriptor( dataType, data, size );
  _transferFieldData.insert( arTransferFieldData::value_type( realName, descriptor ) );

  return true;
}

bool arMasterSlaveFramework::addInternalTransferField( std::string fieldName,
                                                       arDataType dataType, int size ) {
  if( _userInitCalled ) {
    std::cerr << _label << " warning: ignoring addTransferField() after init()." << std::endl;
    return false;
  }

  if( size <= 0 ) {
    std::cerr << _label << " warning: ignoring addTransferField() with size "
              << size << "." << std::endl;
    return false;
  }

  const std::string realName = "USER_" + fieldName;
  if( _transferTemplate.getAttributeID( realName ) >= 0 ) {
    std::cerr << _label
              << " warning: ignoring addTransferField() with duplicate name." << std::endl;
    return false;
  }

  void* data = ar_allocateBuffer( dataType, size );
  if( !data ) {
    std::cerr << "arExperiment error: memory panic." << std::endl;
    return false;
  }

  _transferTemplate.addAttribute( realName, dataType );

  const arTransferFieldDescriptor descriptor( dataType, data, size );
  _internalTransferFieldData.insert( arTransferFieldData::value_type( realName,descriptor ) );

  return true;
}

bool arMasterSlaveFramework::setInternalTransferFieldSize( std::string fieldName,
                                                           arDataType dataType, int newSize ) {
  if (!getMaster()) {
    std::cerr << "arMasterSlaveFramework warning: ignoring setInternalTransferFieldSize() "
              << "on slave." << std::endl;
    return false;
  }

  const std::string realName = "USER_" + fieldName;

  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if( iter == _internalTransferFieldData.end() ) {
    std::cerr << "arMasterSlaveFramework error: internal transfer field "
              << fieldName << " not found." << std::endl;
    return false;
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    std::cerr << _label << " error: wrong type "
              << arDataTypeName( dataType ) << " specified for transfer field "
              << fieldName << "; should be " << arDataTypeName( p.type ) << std::endl;
    return false;
  }

  int currSize = p.size;
  if ( newSize == currSize ) {
    return true;
  }

  ar_deallocateBuffer( p.data );
  p.data = ar_allocateBuffer(  p.type, newSize );
  if( !p.data ) {
    std::cerr << "arMasterSlaveFramework error: failed to resize " << fieldName << std::endl;
    return false;
  }

  p.size = newSize;
  return true;
}

void* arMasterSlaveFramework::getTransferField( std::string fieldName,
                                                arDataType dataType, int& size ) {
  const string realName = "USER_" + fieldName;

  arTransferFieldData::iterator iter = _internalTransferFieldData.find( realName );
  if( iter == _internalTransferFieldData.end() ) {
    iter = _transferFieldData.find( realName );
    if( iter == _transferFieldData.end() ) {
      std::cerr << _label << " error: transfer field "
                << fieldName << " not found." << std::endl;
      return (void*)0;
    }
  }

  arTransferFieldDescriptor& p = iter->second;
  if( dataType != p.type ) {
    std::cerr << _label << " error: wrong type "
              << arDataTypeName( dataType ) << " specified for transfer field "
              << fieldName << "; should be " << arDataTypeName( p.type ) << std::endl;
    return (void*)0;
  }

  size = p.size;
  return p.data;
}

arGraphicsWindow* arMasterSlaveFramework::getGraphicsWindow( const int windowID ) {
  return _wm->getGraphicsWindow( windowID );
}

void arMasterSlaveFramework::returnGraphicsWindow( const int windowID ) {
  _wm->returnGraphicsWindow( windowID );
}

void arMasterSlaveFramework::setPlayTransform( void ){
  if( soundActive() ) {
    _speakerObject.loadMatrices( _inputState->getMatrix( 0 ) );
  }
}

void arMasterSlaveFramework::drawGraphicsDatabase( void ){
  _graphicsDatabase.draw();
}

/*
void arMasterSlaveFramework::draw( void ) {
  std::cout << "arMasterSlaveFramework warning: arMasterSlaveFramework::draw() "
            << "has been renamed drawGraphicsDatabase()." << std::endl
            << "    Please modify your code accordingly." << std::endl;
}
*/

void arMasterSlaveFramework::setRandomSeed( const long newSeed ) {
  if (!_master) {
    return;
  }

  if( newSeed==0 ) {
    std::cerr << _label
	            << " warning: illegal random seed value 0 replaced with -1." << std::endl;
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
    std::cerr << _label << " warning: unequal numbers of calls to randUniformFloat() "
              << "on different machines." << std::endl;
  }

  if(_randSynchError & 2 ) {
    std::cerr << _label << " warning: divergence of random number seeds "
         << "on different machines." << std::endl;
  }

  if( _randSynchError & 4 ) {
    std::cerr << _label << " warning: divergence of random number values "
              << "on different machines." << std::endl;
  }

  bool success = ( _randSynchError == 0 );
  _randSynchError = 0;

  return success;
}


//************************************************************************
// Various small utility functions
//************************************************************************

void arMasterSlaveFramework::_setMaster( bool master ){
  _master = master;
}

bool arMasterSlaveFramework::_sync( void ){
  if( _master ) {
    // VERY IMPORTANT THAT WE DO NOT CALL localSync() IF NO ONE IS
    // CONNECTED. THE localSync() CALL CONTAINS A THROTTLE IN CASE
    // NO ONE IS CONNECTED. IF WE CALLED IT, WE WOULD CAUSE A
    // PERFORMANCE HIT IN THE COMMON CASE OF JUST ONE APPLICATION INSTANCE.
    if( _stateServer->getNumberConnectedActive() > 0 ) {
      _barrierServer->localSync();
    }

    return true;
  }

  return ( !_stateClientConnected || _barrierClient->sync() );
}

//************************************************************************
// Slightly more involved, but still miscellaneous, utility functions.
// These deal with graphics.
//************************************************************************

/*
void arMasterSlaveFramework::_createGLUTWindow( void ) {
  // create the window
  char** argv = new char*[ 1 ];
  argv[ 0 ] = "application";
  int argc = 1;

  glutInit(&argc, argv);
  glutInitDisplayMode( _glutDisplayMode | (_stereoMode ? GLUT_STEREO : 0));
  glutInitWindowPosition(0,0);
  char label[256]; /// \todo fixed size buffer
  ar_stringToBuffer(_label, label, sizeof(label));
  glutInitWindowPosition(_windowPositionX, _windowPositionY);
  if (_windowSizeX>0 && _windowSizeY>0){
    glutInitWindowSize(_windowSizeX, _windowSizeY);
    glutCreateWindow(label);
    glutReshapeFunc(ar_masterSlaveFrameworkReshapeFunction);
  }
  else{
    glutInitWindowSize(640,480);
    glutCreateWindow(label);
    glutReshapeFunc(ar_masterSlaveFrameworkReshapeFunction);
    glutFullScreen();
  }

  glutSetCursor(GLUT_CURSOR_NONE);
  glutDisplayFunc(ar_masterSlaveFrameworkDisplayFunction);
  glutKeyboardFunc(ar_masterSlaveFrameworkKeyboardFunction);
  glutIdleFunc(ar_masterSlaveFrameworkDisplayFunction);
  glutMouseFunc(ar_masterSlaveFrameworkButtonFunction);
  glutMotionFunc(ar_masterSlaveFrameworkMouseFunction);
  glutPassiveMotionFunc(ar_masterSlaveFrameworkMouseFunction);
}
*/

/// Check and see if we are supposed to take a screenshot. If so, go
/// ahead and take it!
void arMasterSlaveFramework::_handleScreenshot( bool stereo ) {
  if( _screenshotFlag ) {
    char numberBuffer[ 8 ];

    sprintf( numberBuffer,"%i", _whichScreenshot );
    std::string screenshotName = std::string( "screenshot" ) + numberBuffer +
                                 std::string( ".jpg" );

    char* buf1 = new char[ _screenshotWidth * _screenshotHeight * 3 ];

    glReadBuffer( stereo ? GL_FRONT_LEFT : GL_FRONT );

    glReadPixels( _screenshotStartX, _screenshotStartY,
                  _screenshotWidth, _screenshotHeight,
                  GL_RGB, GL_UNSIGNED_BYTE, buf1 );

    arTexture texture;
    texture.setPixels( buf1, _screenshotWidth, _screenshotHeight );
    if( !texture.writeJPEG( screenshotName.c_str(),_dataPath ) ) {
      std::cerr << "arMasterSlaveFramework remark: screenshot write failed."
                << std::endl;
    }

    // the pixels are copied into arTexture's memory so we must delete them here.
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
    std::cerr << _label << " warning: ignoring slave's _sendData." << std::endl;
    return false;
  }

  // Pack user data.
  arTransferFieldData::iterator i;
  for( i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      std::cerr << _label << " warning: aborting _sendData with nil data." << std::endl;
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata,
                                i->second.type, i->second.size ) ) {
      std::cerr << _label << " warning: problem in _sendData." << std::endl;
    }
  }

  // Pack internally-stored user data.
  for( i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end(); ++i ) {
    const void* pdata = i->second.data;
    if( !pdata ) {
      std::cerr << _label << " warning: aborting _sendData with nil data." << std::endl;
      return false;
    }

    if( !_transferData->dataIn( i->first, pdata,
                                i->second.type, i->second.size ) ) {
      std::cerr << _label << " warning: problem in _sendData." << std::endl;
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

  // Activate pending connections.
  if( _barrierServer->checkWaitingSockets() ) {
    _barrierServer->activatePassiveSockets( _stateServer );
  }

  // Send data, if there are any receivers.
  if( _stateServer->getNumberConnectedActive() > 0 &&
      !_stateServer->sendData( _transferData ) ){
    std::cout << _label
              << " remark: state server failed to send data." << std::endl;
    return false;
  }

  return true;
}

bool arMasterSlaveFramework::_getData( void ) {
  if( _master ) {
    std::cerr << _label << " warning: ignoring master's _getData." << std::endl;
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
    std::cerr << _label << " warning: state client failed to receive data." << std::endl;
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
      if( !i->second.data ) {
        std::cerr << "arMasterSlaveFramework error: failed to allocate memory "
                  << "for transfer field " <<  i->first << std::endl;
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

void arMasterSlaveFramework::_eventCallback( arInputEvent& event ) {
  ar_mutex_lock( &_eventLock );
  _eventQueue.appendEvent( event );
  ar_mutex_unlock( &_eventLock );
}

void arMasterSlaveFramework::_pollInputData( void ) {
  // Should ensure that start() has been called already.
  if( !_master ) {
    std::cerr << _label << " error: slave tried to _pollInputData." << std::endl;
    return;
  }

  if( !_inputActive ) {
    return;
  }

  if( _firstTimePoll ) {
    _startTime = ar_time();
    _time = 0;
    _firstTimePoll = false;
  }
  else {
    double temp = ar_difftime( ar_time(), _startTime ) / 1000.0;
    _lastFrameTime = temp - _time;

    // Set a lower bound for low-resolution system clocks.
    if( _lastFrameTime < 0.005 ) {
      _lastFrameTime = 0.005;
    }

    _time = temp;
  }

  // If we are in standalone mode, get the input events now.
  // AARGH!!!! This could be done more generally...
  if( _standalone && _standaloneControlMode == "simulator" ) {
    _simulator.advance();
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
    std::cerr << _label << " warning: problem in _packInputData." << std::endl;
  }

  if( !ar_saveInputStateToStructuredData( _inputState, _transferData ) ) {
    std::cerr << _label << " warning: failed to pack input state data." << std::endl;
  }

  _numRandCalls = 0;
  _randSeedSet = 0;
  _firstTransfer = 0;
}

void arMasterSlaveFramework::_unpackInputData( void ){
  _transferData->dataOut( "time",            &_time,                 AR_DOUBLE, 1 );
  _transferData->dataOut( "lastFrameTime",   &_lastFrameTime,        AR_DOUBLE, 1 );
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
  if (!ar_setInputStateFromStructuredData( _inputState, _transferData )) {
    cerr << _label << " warning: failed to unpack input state data.\n";
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
    _firstTransfer = 0;
  }

  _numRandCalls = 0;
}

//************************************************************************
// functions pertaining to starting the application
//************************************************************************

/// Determines whether or not we are the master instance
bool arMasterSlaveFramework::_determineMaster( std::stringstream& initResponse ) {
  // each master/slave application has it's own unique service,
  // since each has its own unique protocol
  _serviceName = _SZGClient.createComplexServiceName( std::string( "SZG_MASTER_" ) + _label );

  _serviceNameBarrier = _SZGClient.createComplexServiceName( std::string( "SZG_MASTER_" ) + _label + "_BARRIER" );

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
        initResponse << _label << " error: component failed to be master." << std::endl;
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

/// Sometimes we may want to run the program by itself, without connecting
/// to the distributed system.
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
  if (_standaloneControlMode == "simulator") {
    _simulator.registerInputNode( _inputDevice );
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
      std::cout << error;
      return false;
    }

    arInputSource* driver = (arInputSource*) joystickObject->createObject();

    // The input node is not responsible for clean-up
    _inputDevice->addInputSource( driver, false );
    if( pforthProgramName == "NULL" ) {
      std::cout << "arMasterSlaveFramework remark: no pforth program for "
	              << "standalone joystick." << std::endl;
    }
    else {
      std::string pforthProgram = _SZGClient.getGlobalAttribute( pforthProgramName );
      if( pforthProgram == "NULL" ) {
        std::cout << "arMasterSlaveFramework remark: no pforth program exists for "
	                << "name = " << pforthProgramName << std::endl;
      }
      else {
        arPForthFilter* filter = new arPForthFilter();
        ar_PForthSetSZGClient( &_SZGClient );

        if( !filter->configure( pforthProgram ) ) {
	        std::cout << "arMasterSlaveFramework remark: failed to configure pforth\n"
	                  << "filter with program =\n " << pforthProgram << std::endl;
          return false;
        }

        // The input node is not responsible for clean-up
        _inputDevice->addFilter( filter, false );
      }
    }
  }

  // NOTE: this will probably fail under the current hacked regime...
  // BUT... if this doesn't occur, then the various filters and such
  // complain constantly.
  _inputDevice->init( _SZGClient );
  _inputDevice->start();

  // the below sound initialization must occur here (and not in
  // startStandaloneObjects) since this occurs before the user-defined init.
  // Probably the key thing is that the _soundClient needs to hit its
  // registerLocalConnection method before any dsLoop, etc. calls are made.
  arSpeakerObject* speakerObject = new arSpeakerObject();

  // NOTE: using "new" to create the _soundClient since, otherwise, the
  // constructor creates a conflict with programs that want to use fmod
  // themselves. (of course, the conflict is still present in
  // standalone mode...)
  _soundClient = new arSoundClient();
  _soundClient->setSpeakerObject( speakerObject );
  _soundClient->configure( &_SZGClient );
  _soundClient->_cliSync.registerLocalConnection( &_soundServer._syncServer );

  // The underlying sound engine for the sound client must be initialized.
  (void)_soundClient->init();

  // Sound is inactive for the time being
  _soundActive = false;

  // This always succeeds.
  return true;
}

/// This is lousy factoring. I think it would be a good idea to eventually
/// fold init and start in together.
bool arMasterSlaveFramework::_startStandaloneObjects( void ) {
  _soundServer.start();
  _soundActive = true;
  return true;
}

bool arMasterSlaveFramework::_initMasterObjects( std::stringstream& initResponse ) {
  // attempt to initialize the barrier server
  _barrierServer = new arBarrierServer();

  if( !_barrierServer ) {
    initResponse << _label
                 << "error: master failed to construct barrier server." << std::endl;
    return false;
  }

  _barrierServer->setServiceName( _serviceNameBarrier );
  _barrierServer->setChannel( "graphics" );

  if( !_barrierServer->init( _SZGClient ) ) {
    initResponse << _label << " error: failed to initialize barrier "
	               << "server." << std::endl;
  }

  _barrierServer->registerLocal();
  _barrierServer->setSignalObject( &_swapSignal );

  // attempt to initialize the input device
  _inputActive = false;
  _inputDevice = new arInputNode( true );

  if( !_inputDevice ) {
    initResponse << _label
                 << " error: master failed to construct input device." << std::endl;
    return false;
  }

  _inputState = &_inputDevice->_inputState;
  _inputDevice->addInputSource( &_netInputSource, false );
  _netInputSource.setSlot( 0 );

  if( !_inputDevice->init( _SZGClient ) ) {
    initResponse << _label << " error: master failed to init input device." << std::endl;

    delete _inputDevice;
    _inputDevice = NULL;

    return false;
  }

  // attempt to initialize the sound device
  _soundActive = false;
  if( !_soundServer.init( _SZGClient ) ) {
    initResponse << _label << " error: sound server failed to init." << std::endl
                 << "  (Is another application running?)" << std::endl;
    return false;
  }

  // we've succeeded in initializing the various objects
  initResponse << _label << " remark: master's objects initialized." << std::endl;
  return true;
}

/// Starts the objects needed by the master.
bool arMasterSlaveFramework::_startMasterObjects( std::stringstream& startResponse ) {
  // go ahead and start the master's service
  _stateServer = new arDataServer( 1000 );

  if( !_stateServer ) {
    startResponse << _label
                  << " error: master failed to construct state server." << std::endl;
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
    startResponse << _label << " warning: failed to listen on port "
		              << _masterPort[ 0 ] << "." << std::endl;

    _SZGClient.requestNewPorts( _serviceName, "graphics", 1 ,_masterPort );
    _stateServer->setPort( _masterPort[ 0 ] );
  }

  if( tries >= 10 ) {
    // failed to bind to the ports
    return false;
  }

  if( !_SZGClient.confirmPorts( _serviceName, "graphics", 1, _masterPort ) ) {
    startResponse << _label << " error: brokered port unconfirmed." << std::endl;
    return false;
  }

  if( !_barrierServer->start() ) {
    startResponse << _label << " error: failed to start barrier server." << std::endl;
    return false;
  }

  if( !_inputDevice->start() ) {
    startResponse << _label << " error: failed to start input device." << std::endl;

    delete _inputDevice;
    _inputDevice = NULL;

    return false;
  }

  _inputActive = true;

  // start the sound server
  if( !_soundServer.start() ) {
    startResponse << _label << " error: sound server failed to listen." << std::endl
                  << "  (Is another application running?)" << std::endl;
    return false;
  }

  _soundActive = true;

  startResponse << _label << " remark: master's objects started.\n";
  return true;
}

bool arMasterSlaveFramework::_initSlaveObjects( std::stringstream& initResponse ) {
  // slave instead of master
  // in this case, the component must know which networks on which it
  // should attempt to connect
  _networks = _SZGClient.getNetworks( "graphics" );
  _inBufferSize = 1000;
  _inBuffer = new ARchar[ _inBufferSize ];
  _barrierClient = new arBarrierClient;

  if( !_barrierClient ) {
    initResponse << _label
                  << "error: slave failed to construct barrier client." << std::endl;
    return false;
  }

  _barrierClient->setNetworks( _networks );
  _barrierClient->setServiceName( _serviceNameBarrier );

  if( !_barrierClient->init( _SZGClient ) ) {
    initResponse << _label << " error: barrier client failed to start." << std::endl;
    return false;
  }

  _inputState = new arInputState();

  if( !_inputState ) {
    initResponse << _label
                 << "error: slave failed to construct input state." << std::endl;
    return false;
  }
  // of course, the state client should not have weird delays on small
  // packets, as would be the case in the Win32 TCP/IP stack without
  // doing the following
  _stateClient.smallPacketOptimize( true );

  // we've succeeded in the init
  initResponse << _label << " remark: the initialization of the slave "
	             << "objects succeeded." << std::endl;
  return true;
}

/// Starts the objects needed by the slaves
bool arMasterSlaveFramework::_startSlaveObjects( std::stringstream& startResponse ) {
  // the barrier client is the only object to start
  if( !_barrierClient->start() ) {
    startResponse << _label << " error: barrier client failed to start." << std::endl;
    return false;
  }

  startResponse << _label << " remark: slave objects started." << std::endl;
  return true;
}

bool arMasterSlaveFramework::_startObjects( void ){
  // THIS IS GUARANTEED TO BEGIN AFTER THE USER-DEFINED INIT!

  // want to be able to write to the start stream
  stringstream& startResponse = _SZGClient.startResponse();

  // Create the language.
  _transferLanguage.add( &_transferTemplate );
  _transferData = new arStructuredData( &_transferTemplate );

  if( !_transferData ) {
    startResponse << _label
                  << " error: master failed to construct _transferData." << std::endl;
    return false;
  }

  // Go ahead and start the arMasterSlaveDataRouter (this just creates the
  // language to be used by that device... using the registered
  // arFrameworkObjects). Always succeeds as of now.
  (void) _dataRouter.start();

  // By now, we know whether or not we are the master
  if( _master ) {
    if( !_startMasterObjects( startResponse ) ) {
      return false;
    }
  }
  else {
    if( !_startSlaveObjects( startResponse ) ) {
      return false;
    }
  }

  // Both master and slave need a connection thread
  if( !_connectionThread.beginThread( ar_masterSlaveFrameworkConnectionTask,
                                      this ) ) {
    startResponse << _label << " error: failed to start connection thread." << std::endl;
    return false;
  }

  _graphicsDatabase.loadAlphabet( _textPath );
  _graphicsDatabase.setTexturePath( _texturePath );

  return true;
}

/*
/// What we need to do to start in standalone mode... this should be factored
/// BACK into my normal stuff. HACK HACK HACK HACK HACK HACK HACK HACK HACK
bool arMasterSlaveFramework::_startStandalone( bool useWindowing ){
  // _graphicsWindow.setDrawCallback( new arMasterSlaveRenderCallback( *this ) );

  if( !onStart( _SZGClient ) ) {
    return false;
  }

  if( _useWindowing ) {
    _createWindowing();
  }

  // this is from _startObjects... a CUT_AND_PASTE!!
  _graphicsDatabase.loadAlphabet( _textPath );
  _graphicsDatabase.setTexturePath( _texturePath );
  _startStandaloneObjects();
  _userInitCalled = true;

  if ( useWindowing ) {
    _displayThreadRunning = true;
    std::cout << _label << " remark: entering glutMainLoop()." << std::endl;
    // glutMainLoop(); // never returns
    // PLAYING WITH SWAP (formerly startWithoutSwap), how about startWithSwap
    _wm->startWithoutSwap();
  }

  return true;
}
*/

/// Functionality common to start() and startWithoutGLUT().
bool arMasterSlaveFramework::_start( bool useWindowing ) {
  _useWindowing = useWindowing;

  if( !_parametersLoaded ) {
    if( _SZGClient ) {
      _SZGClient.initResponse() << _label
                                << " error: start() method called before init() method." << std::endl;

      if( !_SZGClient.sendInitResponse( false ) ) {
        cerr << _label << " error: maybe szgserver died." << std::endl;
      }
    }

    return false;
  }

  if( !_standalone ) {
    // make sure we get the screen resource
    // this lock should only be grabbed AFTER an application launching.
    // i.e. DO NOT do this in init(), which is called even on the trigger
    // instance... but do it here (which is only reached on render instances)
    std::string screenLock = _SZGClient.getComputerName() + "/" +
                             _SZGClient.getMode( "graphics" );
    int graphicsID;

    if( !_SZGClient.getLock( screenLock, graphicsID ) ) {
      char buf[ 20 ];
      sprintf( buf, "%d", graphicsID );
      return _startrespond( "failed to get screen resource held by component " +
  	                        std::string( buf ) + ".\n(dkill that component to proceed.)" );
    }
  }

  /*
  _graphicsWindow.setDrawCallback( new arMasterSlaveRenderCallback( *this ) );

  if( _useGLUT ) {
    _createGLUTWindow();
  }
  */

  // do the user-defined init
  // NOTE: so that _startCallback can know if this instance is the master or
  // a slave, it is important to call this AFTER _startDetermineMaster(...)
  if( !onStart( _SZGClient ) ) {
    if( _SZGClient ) {
      return _startrespond( "arMasterSlaveFramework start callback failed." );
    }
    else {
      return false;
    }
  }

  if( _useWindowing ) {
    _createWindowing();
  }

  if( _standalone ) {
    // this is from _startObjects... a CUT_AND_PASTE!!
    _graphicsDatabase.loadAlphabet( _textPath );
    _graphicsDatabase.setTexturePath( _texturePath );
    _startStandaloneObjects();
  }
  else {
    // set-up the various objects and start services
    if( !_startObjects() ) {
      return _startrespond( "Objects failed to start." );
    }
  }

  _userInitCalled = true;

  // the start succeeded
  if( _SZGClient && !_SZGClient.sendStartResponse( true ) ) {
    std::cerr << _label << " error: maybe szgserver died." << std::endl;
  }

  if( !_standalone ) {
    _wm->findFramelock();
  }

  if( _useWindowing ) {
    _displayThreadRunning = true;
    // This used to be the GLUT main loop....
    // glutMainLoop(); // never returns

    // unrolled event loop
    while( true ) {
      ar_mutex_lock( &_windowingMutex );
      preDraw();

      _wm->drawAllWindows( true );

      postDraw();

      // Will _internalBufferSwap always be "true"?
      if( _internalBufferSwap ) {
        _wm->swapAllWindowBuffers( true );
      }
      _wm->processWindowEvents();
      ar_mutex_unlock( &_windowingMutex );
      
    }
  }

  return true;
}

//**************************************************************************
// Other system-level functions
//**************************************************************************

void arMasterSlaveFramework::_createWindowing( void ) {
  std::vector< arGUIXMLWindowConstruct* >* windowConstructs = _guiXMLParser->getWindowingConstruct()->getWindowConstructs();

  if( !windowConstructs ) {
    // print error?
    return;
  }

  // populate the callbacks for both the gui and graphics windows and the head
  // for any vr cameras
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

  // actually create the windows
  if( _wm->createWindows( _guiXMLParser->getWindowingConstruct() ) < 0 ) {
    std::cout << "could not create windows" << std::endl;
    // exit( 0 );
  }
}

bool arMasterSlaveFramework::_loadParameters( void ) {

  // The graphics configuration is in a bit of a state of transition.
  // Some of the global window values will still be gotten from the
  // SZG_SCREENx data structure (like window size and view mode)... but
  // this data sctructure is moving towards defining the camera for a
  // viewport.
  std::cout << "arMasterSlaveFramework remark: reloading parameters." << std::endl;

  // some things just depend on the SZG_RENDER
  _texturePath = _SZGClient.getAttribute( "SZG_RENDER","texture_path" );
  std::string received( _SZGClient.getAttribute( "SZG_RENDER","text_path" ) );
  ar_stringToBuffer( ar_pathAddSlash( received ), _textPath, sizeof( _textPath ) );

  // We set a few window-wide attributes based on screen name. THIS IS AN
  // UGLY HACK!!!! (stereo, window size, window position, wildcat framelock)
  const string screenName( _SZGClient.getMode( "graphics" ) );

  // There is a transition in configuration occuring, from the previous one
  // to one based on arGUI. The "magic" keywords are changing from
  // SZG_SCREENn to SZG_DISPLAYn.
  //std::string whichDisplay = _SZGClient.getMode( "gui" );
  string whichDisplay
    = "SZG_DISPLAY" + screenName.substr( screenName.length() - 1, 1 );

  std::string displayName  = _SZGClient.getAttribute( whichDisplay, "name" );

  std::cout << "Using display: " << whichDisplay << " : "
            << displayName << std::endl;

  _guiXMLParser->setConfig( _SZGClient.getGlobalAttribute( displayName ) );

  // perform the parsing of the xml
  if( _guiXMLParser->parse() < 0 ) {
    // already complained, just return
    return false;
  }

  /*
  // Which screen defines the view?
  _defaultScreen.configure( screenName, _SZGClient );
  _defaultCamera.setScreen( &_defaultScreen );
  // It could be that we've already set a custom camera
  // (as in the non-euclidean math visualizations)
  if (!_graphicsWindow.getCamera()){
    _graphicsWindow.setCamera( &_defaultCamera );
  }
  else{
    cout << "arMasterSlaveFramework remark: using a custom camera.\n";
  }
  _graphicsWindow.configure(_SZGClient);

  _stereoMode = _graphicsWindow.getUseOGLStereo();
  // NOTE: quad-buffered stereo is NOT compatible with custom viewports
  //  _stereoMode = _SZGClient.getAttribute(screenName, "stereo",
  //    "|false|true|") == "true";
  //  _graphicsWindow.useOGLStereo( _stereoMode );

  float temp[2];
  if (_SZGClient.getAttributeFloats(screenName, "size", temp, 2)){
    _windowSizeX = int(temp[0]);
    _windowSizeY = int(temp[1]);
  }
  else {
    // default for when there's no configuration
    _windowSizeX = 640;
    _windowSizeY = 480;
  }

  if (_SZGClient.getAttributeFloats(screenName, "position", temp, 2)){
    _windowPositionX = int(temp[0]);
    _windowPositionY = int(temp[1]);
  }
  else{
    _windowPositionX = 0;
    _windowPositionY = 0;
  }
  */

  if( getMaster() ) {
    _head.configure( _SZGClient );
  }

  if( _standalone ) {
    _simulator.configure( _SZGClient );
  }

  // Don't think the speaker object configuration actually does anything
  // yet!!!!
  if( !_speakerObject.configure( &_SZGClient ) ) {
    return false;
  }

  // Load the other parameters.
  _loadNavParameters();

  // Make sure everybody gets the right bundle map, both for standalone
  // and for normal operation.
  _dataPath = _SZGClient.getAttribute( "SZG_DATA", "path" );

  if( _dataPath == "NULL" ) {
    _dataPath = std::string( "" );
  }
  else {
    _soundServer.addDataBundlePathMap( "SZG_DATA", _dataPath );

    if (_soundClient ) {
      _soundClient->addDataBundlePathMap( "SZG_DATA", _dataPath );
    }
  }

  std::string pythonPath = _SZGClient.getAttribute( "SZG_PYTHON", "path" );

  if( pythonPath != "NULL" ) {
    _soundServer.addDataBundlePathMap( "SZG_PYTHON", pythonPath );

    if( _soundClient ) {
      _soundClient->addDataBundlePathMap( "SZG_PYTHON", pythonPath );
    }
  }

  return true;
}

void arMasterSlaveFramework::_messageTask( void ) {
  // might be a good idea to shut this down cleanly... BUT currently
  // there's no way to shut the arSZGClient down cleanly.
  std::string messageType, messageBody;

  while( !_exitProgram ) {
    // NOTE: it is possible for receiveMessage to fail, precisely in the
    // case that the szgserver has *hard-shutdown* our client object.
    if( !_SZGClient.receiveMessage( &messageType, &messageBody ) ) {
      // NOTE: the shutdown procedure is the same as for the "quit" message.
      stop( true );

      if( !_useExternalThread ) {
        exit( 0 );
      }

      // We DO NOT want to hit this again! (since things are DISCONNECTED)
      break;
    }

    if( messageType == "quit" ) {
      // at this point, we do our best to bring everything to an orderly halt.
      // this kind of care keeps some programs from seg-faulting on exit,
      // which is bad news on Win32 since it brings up a dialog box that
      // must be clicked!
      // NOTE: we block here until the display thread is finished.'
      stop( true );

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
      (void) _loadParameters();

      // recreate the windows as a diff against the current set of windows
      ar_mutex_lock( &_windowingMutex );
      _createWindowing();
      ar_mutex_unlock( &_windowingMutex );
    }
    else if ( messageType == "user" ) {
      onUserMessage( messageBody );
    }
    else if( messageType == "color" ) {
      if( messageBody == "NULL" || messageBody == "off") {
        _noDrawFillColor = arVector3( -1.0f, -1.0f, -1.0f );
      }
      else {
	      float tmp[ 3 ];
	      ar_parseFloatString( messageBody, tmp, 3 );
	      /// \todo error checking
        memcpy( _noDrawFillColor.v, tmp, 3 * sizeof( AR_FLOAT ) );
      }
    }
    else if( messageType == "screenshot" ) {
      if( _dataPath == "NULL" ) {
	      std::cerr << _label << " warning: screenshot failed, no SZG_DATA/path." << std::endl;
      }
      else {
        _screenshotFlag = true;

        if( messageBody != "NULL" ) {
          int tmp[ 4 ];
          ar_parseIntString( messageBody, tmp, 4 );
	        /// \todo error checking
          _screenshotStartX = tmp[ 0 ];
      	  _screenshotStartY = tmp[ 1 ];
      	  _screenshotWidth  = tmp[ 2 ];
      	  _screenshotHeight = tmp[ 3 ];
	      }
      }
    }
    else if( messageType == "look" ) {
      /*
      if (messageBody == "NULL") {
	      // the default camera
        _graphicsWindow.setCamera( &_defaultCamera );
      }
      else{
	      // Activate a new camera, which may be better for screenshots.
        // tmp = 6 glFrustum params followed by 9 gluLookat params
        float tmp[ 15 ];
        int numberArgs = ar_parseFloatString( messageBody, tmp, 15 );

        if (numberArgs != 15) {
          cerr << "arMasterSlaveFramework warning: body of 'look' message "
               << "contained wrong number of args\n  ("
               << numberArgs << ", should be 15).\n";
        }
        else {
          // The Irix compiler does not like things like foo(&constructor(...)),
          // consequently, use temp
          arPerspectiveCamera temp( tmp, tmp+6 );
          _graphicsWindow.setCamera( &temp );
        }
      }
      */
    }
    else if( messageType == "demo" ) {
      bool onoff = ( messageBody == "on" ) ? true : false;
      setFixedHeadMode( onoff );
    }

    else if ( messageType == "viewmode" ) {
      // setViewMode( messageBody );
    }

    //*********************************************************
    // There's quite a bit of copy-pasting between the
    // messages accepted by szgrender and here... how can we
    // reuse messaging functionality?????
    //*********************************************************
    if( messageType == "delay" ) {
      if( messageBody == "on" ) {
        _framerateThrottle = true;
      }
      else {
        _framerateThrottle = false;
      }
    }
    else if ( messageType == "pause" ) {
      if ( messageBody == "on" ) {
        ar_mutex_lock( &_pauseLock );
        // do not pause if we are exitting
        if( !_exitProgram ) {
	        _pauseFlag = true;
	      }
	      ar_mutex_unlock(&_pauseLock);
      }
      else if( messageBody == "off" ) {
        ar_mutex_lock( &_pauseLock );
        _pauseFlag = false;
        _pauseVar.signal();
        ar_mutex_unlock( &_pauseLock );
      }
      else
        std::cerr << _label << " warning: ignoring unexpected pause arg \""
	                << messageBody << "\"." << std::endl;
    }

  }
}

void arMasterSlaveFramework::_connectionTask( void ) {
  // shutdown depends on the value of _connectionThreadRunning
  _connectionThreadRunning = true;

  if( _master ) {
    // THE MASTER'S METHOD OF HANDLING CONNECTIONS
    while( !_exitProgram ) {
      // TODO TODO TODO TODO TODO TODO
      // As a hack, since non-blocking connection accept has yet to be
      // implemented, we have to pretend the connection thread isn't running
      // during the blocking call
      _connectionThreadRunning = false;
      arSocket* theSocket = _stateServer->acceptConnectionNoSend();
      _connectionThreadRunning = true;

      if( _exitProgram ) {
        break;
      }

      if( !theSocket || _stateServer->getNumberConnected() <= 0 ) {
        // something bad happened.  Don't keep trying infinitely.
        _exitProgram = true;
	      break;
      }

      std::cout << _label << " remark: slave connected to master";
      const int num = _stateServer->getNumberConnected();

      if( num > 1 ) {
	      std::cout << " (" << num << " in all)";
	    }
      std::cout << std::endl;

      ar_mutex_lock( &_connectFlagMutex );
      _newSlaveConnected = true;
      if( _stateServer ) {
        _numSlavesConnected = _stateServer->getNumberConnected();
      }
      else {
        _numSlavesConnected = -1;
      }
      ar_mutex_unlock( &_connectFlagMutex );
    }
  }
  else {
    // THE SLAVE'S METHOD OF HANDLING CONNECTIONS
    while( !_exitProgram ) {
      // make sure barrier is connected first
      while( !_barrierClient->checkConnection() && !_exitProgram ) {
        ar_usleep( 100000 );
      }

      if( _exitProgram ) {
        break;
      }

      // TODO TODO TODO TODO TODO TODO
      // As a hack, since discoverService is currently a blocking call
      // and the arSZGClient does not have a shutdown mechanism in place
      _connectionThreadRunning = false;
      const arPhleetAddress result =
        _SZGClient.discoverService( _serviceName, _networks, true );

      _connectionThreadRunning = true;

      if( _exitProgram ) {
        break;
      }

      std::cout << _label << " remark: connecting to "
	              << result.address << ":" << result.portIDs[ 0 ] << std::endl;

      if( !result.valid ||
          !_stateClient.dialUpFallThrough( result.address, result.portIDs[ 0 ] ) ) {
        std::cout << _label << " warning: brokering process failed. Will retry." << std::endl;
        continue;
      }

      // Bond the appropriate data channel to this sync channel.
      _barrierClient->setBondedSocketID( _stateClient.getSocketIDRemote() );
      _stateClientConnected = true;

      std::cout << _label << " remark: slave connected to master." << std::endl;
      while( _stateClientConnected && !_exitProgram ) {
        ar_usleep( 300000 );
      }

      if( _exitProgram ) {
        break;
      }

      std::cout << _label << " remark: slave disconnected.";
      _stateClient.closeConnection();
    }
  }

  _connectionThreadRunning = false;
}

//**************************************************************************
// Functions directly pertaining to drawing
//**************************************************************************

/// This function is responsible for displaying a whole arGUIWindow.
/// Look at the definition of arMasterSlaveRenderCallback to understand
/// how it is called from arGUIWindow.
void arMasterSlaveFramework::_drawWindow( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow ) {
  if( !windowInfo || !graphicsWindow ) {
    std::cerr << "arMasterSlaveFramework error: NULL arGUIWindowInfo*|arGraphicsWindow* passed to _drawWindow()." << std::endl;;
    return;
  }

  int currentWinID = windowInfo->getWindowID();
  if( !_wm->windowExists( currentWinID ) ) {
    std:: cerr << "arMasterSlaveFramework error: arGraphicsWindow with ID " << currentWinID
         << " not found in _drawWindow()." << std::endl;
    return;
  }

  // stuff pixel dimensions into arGraphicsWindow
  graphicsWindow->setPixelDimensions( windowInfo->getPosX(), windowInfo->getPosY(),
                                      windowInfo->getSizeX(), windowInfo->getSizeY() );
  // draw the window
  if(!_exitProgram ) {
    if( _noDrawFillColor[ 0 ] == -1 ) {
      if( getConnected() ) {
        graphicsWindow->draw();

        if (currentWinID == 0) {
          if( _standalone && _standaloneControlMode == "simulator" ) {
            _simulator.drawWithComposition();
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
      // we just want a colored background
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

  // it seems like glFlush/glFinish are a little bit unreliable... not
  // every vendor has done a good job of implementing these.
  // Consequently, we do a small pixel read to force drawing to complete.
  // THIS IS EXTREMELY IMPORTANT!
  char buffer[ 32 ];
  glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
  // Weirdly, with arGUI, the following seems necessary for "high quality"
  // synchronization. Maybe GLUT was implicitly posting one of these itself?
  // What makes this strange is that I'd expect the glReadPixels to have
  // already done this...
  glFinish();

  // if we are supposed to take a screenshot, go ahead and do so.
  // only the "first" window (if there are multiple GUI windows) will be
  // captured.
  if( _wm->isFirstWindow( currentWinID ) ) {
    _handleScreenshot( _wm->isStereo( currentWinID ) );
  }

  // if we are in shutdown mode, we want to stop everything and
  // then go away. NOTE: there are special problems since _drawWindow()
  // and the keyboard function where the ESC press is caught are in the
  // same thread
  if( _exitProgram ) {
    // guaranteed to get here--user-defined cleanup will be called
    // at the right time
    onCleanup();

    // wildcat framelock is only deactivated if this makes sense...
    // and if it makes sense, it is important to do so before exiting.
    // also important that we do this in the display thread
    // ar_deactivateWildcatFramelock();
    _wm->deactivateFramelock();

    std::cout << _label << " remark: window " << currentWinID << " done." << std::endl;

    _displayThreadRunning = false;
    // if stop(...) is called from the GLUT keyboard function, we
    // want to exit here. (_blockUntilDisplayExit will be false)
    // Otherwise, we want to wait here for the exit to occur in the
    // message thread
    while( _blockUntilDisplayExit ){
      ar_usleep( 100000 );
    }

    exit( 0 );
  }
}

/*
// OUCH! This might or might not be safe under the current regime!
bool arMasterSlaveFramework::setViewMode( const std::string& viewMode ) {
//  cerr << _label << " remark: setting view mode to "
//       << viewMode << endl;
  return _graphicsWindow.setViewMode( viewMode );
}
*/

int arMasterSlaveFramework::getNumberSlavesConnected( void ) const {
  if( !getMaster() ) {
    std::cerr << "arMasterSlaveFramework warning: getNumberSlavesConnected() called on slave." << std::endl;
    return -1;
  }

  return _numSlavesConnected;
}
