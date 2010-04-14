//@+leo-ver=4-thin
//@+node:jimc.20100409112755.164:@thin graphics\arGUIWindow.cpp
//@@language c++
//@@tabwidth -2
//@+others
//@+node:jimc.20100409112755.165:arGUIWindow declarations
/**
 * @file arGUIWindow.cpp
 * Implementation of the arGUIWindowConfig, arWMEvent,
                         arGUIWindow and arDefaultGUIRenderCallback classes.
 */
#include "arPrecompiled.h"

#if defined( AR_USE_WIN_32 )

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  // #include <Xm/MwmUtil.h>
#endif

#include "arStructuredData.h"
#include "arDataUtilities.h"
#include "arLogStream.h"
#include "arGUIEventManager.h"
#include "arGUIInfo.h"
#include "arGUIWindow.h"

// convenience define
const double USEC = 1000000.0;

//@-node:jimc.20100409112755.165:arGUIWindow declarations
//@+node:jimc.20100409112755.166:arDefaultGUIRenderCallback::operator
// the fall-throughs in these operators /might/ be 'dangerous' (e.g. a
// drawcallback expecting a windowinfo and/or a graphicsWindow gets passed
// NULL(s)), but they *should* be robust to deal with such a situation.  This
// allows us a bit more flexibility in how callbacks are registered and used.
void arDefaultGUIRenderCallback::operator()( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow ) {
  if ( _drawCallback0 ) {
    _drawCallback0( windowInfo, graphicsWindow );
  }
  else if ( _drawCallback1 ) {
    _drawCallback1( windowInfo );
  }
}
//@-node:jimc.20100409112755.166:arDefaultGUIRenderCallback::operator
//@+node:jimc.20100409112755.167:arDefaultGUIRenderCallback::operator

void arDefaultGUIRenderCallback::operator()( arGUIWindowInfo* windowInfo ) {
  if ( _drawCallback1 ) {
    _drawCallback1( windowInfo );
  }
  else if ( _drawCallback0 ) {
    _drawCallback0( windowInfo, NULL );
  }
}
//@-node:jimc.20100409112755.167:arDefaultGUIRenderCallback::operator
//@+node:jimc.20100409112755.168:arDefaultGUIRenderCallback::operator

void arDefaultGUIRenderCallback::operator()( arGraphicsWindow& graphicsWindow, arViewport& viewport ) {
  if ( _drawCallback2 ) {
    _drawCallback2( graphicsWindow, viewport );
  }
  else if ( _drawCallback0 ) {
    _drawCallback0( NULL, NULL );
  }
  else if ( _drawCallback1 ) {
    _drawCallback1( NULL );
  }
}
//@-node:jimc.20100409112755.168:arDefaultGUIRenderCallback::operator
//@+node:jimc.20100409112755.169:arGUIWindowConfig::arGUIWindowConfig

// The following causes the linker to barf (VC++6, anyway) on the Python bindings.
//const std::string arGUIWindowConfig::_titleDefault("SyzygyWindow");

arGUIWindowConfig::arGUIWindowConfig( int x, int y, int width, int height,
                                      int bpp, int hz,
                                      bool decorate, arZOrder zorder,
                                      bool fullscreen, bool stereo,
                                      const std::string& title,
                                      const std::string& XDisplay,
                                      arCursor cursor ) :
  _x( x ),
  _y( y ),
  _width( width ),
  _height( height ),
  _bpp( bpp ),
  _Hz( hz ),
  _decorate( decorate ),
  _fullscreen( fullscreen ),
  _stereo( stereo ),
  _zorder( zorder ),
  _title( title ),
  _XDisplay( XDisplay ),
  _cursor( cursor )
{
}
//@-node:jimc.20100409112755.169:arGUIWindowConfig::arGUIWindowConfig
//@+node:jimc.20100409112755.170:arWMEvent::arWMEvent

arWMEvent::arWMEvent( const arGUIWindowInfo& event ) :
  _eventCond("arWMEvent type " + ar_intToString(event.getEventType()) + ", state " + ar_intToString(event.getState()))
{
  reset(event);
}
//@-node:jimc.20100409112755.170:arWMEvent::arWMEvent
//@+node:jimc.20100409112755.171:arWMEvent::reset

void arWMEvent::reset( const arGUIWindowInfo& event )
{
  _event = event;
  _done = 0;
  arGuard _(_eventMutex, "arWMEvent::reset");
  _conditionFlag = false;
}
//@-node:jimc.20100409112755.171:arWMEvent::reset
//@+node:jimc.20100409112755.172:arWMEvent::wait

void arWMEvent::wait( const bool blocking )
{
  if ( blocking ) {
    arGuard _(_eventMutex, "arWMEvent::wait");
    while ( !_conditionFlag ) {
      if ( !_eventCond.wait( _eventMutex ) ) {
        // print error?
      }
    }
  }

  // Let this event be reused.
  ++_done;
}
//@-node:jimc.20100409112755.172:arWMEvent::wait
//@+node:jimc.20100409112755.173:arWMEvent::signal

void arWMEvent::signal( void )
{
  _eventMutex.lock("arWMEvent::signal");
    _conditionFlag = true;
    _eventCond.signal();
  _eventMutex.unlock();
  ++_done;
}
//@-node:jimc.20100409112755.173:arWMEvent::signal
//@+node:jimc.20100409112755.174:arGUIWindowBuffer::arGUIWindowBuffer

arGUIWindowBuffer::arGUIWindowBuffer( bool dblBuf ) :
  _dblBuf( dblBuf )
{
}
//@-node:jimc.20100409112755.174:arGUIWindowBuffer::arGUIWindowBuffer
//@+node:jimc.20100409112755.175:arGUIWindowBuffer::swapBuffer

int arGUIWindowBuffer::swapBuffer( const arGUIWindowHandle& h, const bool stereo ) const
{
  // Since only arGUIWindow calls this,
  // arGUIWindow::swap ensures that the gl context is correct.

  // Call glFlush first?

#if defined( AR_USE_WIN_32 )

  if ( stereo ) {
    // www.stereographics.com/support/developers/pcsdk.htm says to call this
    // for active stereo, even though glut and freeglut don't.
    if ( !wglSwapLayerBuffers( h._hDC, WGL_SWAP_MAIN_PLANE ) ) {
      ar_log_error() << "wglSwapLayerBuffers failed.\n";
    }
  }
  else {
    if ( !SwapBuffers( h._hDC ) ) {
      // Window might be minimized.
      ar_log_debug() << "swapBuffer failed.\n";
    }
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  (void)stereo; // avoid compiler warning
  XLockDisplay( h._dpy );
  glXSwapBuffers( h._dpy, h._win );
  XUnlockDisplay( h._dpy );

#endif

  return 0;
}
//@-node:jimc.20100409112755.175:arGUIWindowBuffer::swapBuffer
//@+node:jimc.20100409112755.176:arGUIWindow::arGUIWindow

arGUIWindow::arGUIWindow( int ID, arGUIWindowConfig windowConfig,
                          void (*windowInitGLCallback)( arGUIWindowInfo* ),
                          void* userData ) :
  _ID( ID ),
  _className(windowConfig.getTitle() + ar_intToString(ID)),
  _windowConfig( windowConfig ),
  _drawCallback( NULL ),
  _windowInitGLCallback( windowInitGLCallback ),
  _visible( false ),
  _running( false ),
  _threaded( false ),
  _fullscreen( false ),
  _decorate( true ),
  _zorder( AR_ZORDER_TOP ),
  _cursor( AR_CURSOR_NONE ),
  _lastFrameTime(ar_time()),
  _GUIEventManager(new arGUIEventManager( userData )),
  _windowBuffer(new arGUIWindowBuffer( true )),
  _creationCond("arGUIWindow-create, ID = " + ar_intToString(_ID)),
  _destructionCond("arGUIWindow-destroy, ID = " + ar_intToString(_ID)),
  _creationFlag( false ),
  _destructionFlag( false ),
  _userData( userData ),
  _windowManager( NULL ),
  _graphicsWindow( NULL ),
  _WMEventsVar("arGUIWindow-events, ID = " + ar_intToString(_ID))
{
}
//@-node:jimc.20100409112755.176:arGUIWindow::arGUIWindow
//@+node:jimc.20100409112755.177:arGUIWindow

arGUIWindow::~arGUIWindow( void )
{
#if 0
  if ( _running ) {
    if ( _killWindow() < 0 ) {
      // print error?
    }
  }
#endif

  // delete both event queues (what if the wm is still holding onto a handle?)
  if ( _GUIEventManager ) {
    delete _GUIEventManager;
  }

  if ( _windowBuffer ) {
    delete _windowBuffer;
  }

  if ( _drawCallback ) {
    delete _drawCallback;
  }
}
//@-node:jimc.20100409112755.177:arGUIWindow
//@+node:jimc.20100409112755.178:arGUIWindow::registerDrawCallback

void arGUIWindow::registerDrawCallback( arGUIRenderCallback* drawCallback )
{
  arGuard _(_creationMutex, "arGUIWindow::registerDrawCallback");
  if ( _drawCallback ) {
    // warn that previous callback is being overwritten?
    delete _drawCallback;
    _drawCallback = NULL; // Don't let it be reused.
  }

  if ( !drawCallback ) {
    // warn that there is now no draw callback?
  }
  else {
    _drawCallback = drawCallback;
  }
}
//@-node:jimc.20100409112755.178:arGUIWindow::registerDrawCallback
//@+node:jimc.20100409112755.179:arGUIWindow::_drawHandler

void arGUIWindow::_drawHandler( const bool drawLeftBuffer )
{
  arGuard _(_creationMutex, "arGUIWindow::_drawHandler");
  if ( !_running || !_drawCallback )
    return;

  // ensure (in non-threaded mode) that this window's opengl context is current
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "_drawHandler failed to make context current.\n";
  }

  // locking the display here brings up some issues in single threaded mode,
  // the user could call something like wm->swapall at the end of the display
  // callback and then we'd be deadlocked as the swap call needs to lock
  // the display as well...
  // NOTE: in fact we won't lock here, we can't control what the user will
  // call and there may well be a valid need to call something like swap or
  // resize in the draw callback, just make sure everything else that touches
  // the display properly locks and unlocks

#if 0 // defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  XLockDisplay( _windowHandle._dpy );
#endif

  // always try to pass through the graphicswindow, if the user has
  // registered a callback that does not take it, the callback class will
  // handle letting that fall through
  if (_graphicsWindow) {
    _graphicsWindow->setPixelDimensions( getPosX(), getPosY(), getWidth(), getHeight() );
  }
  (*_drawCallback)( _ID, _graphicsWindow, drawLeftBuffer );

#if 0 // defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  XUnlockDisplay( _windowHandle._dpy );
#endif
}
//@-node:jimc.20100409112755.179:arGUIWindow::_drawHandler
//@+node:jimc.20100409112755.180:InitGL

// the most vanilla opengl setup possible
void InitGL( int width, int height )
{
  glClearColor( 0.0f, 0.0f, 0.0f, 0.5f );
  glClearDepth( 1.0 );
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );
  glShadeModel( GL_SMOOTH );
  glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

  glViewport( 0, 0, width, height );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();

  gluPerspective( 45.0, (double) width / (double) height, 0.1, 100.0 );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
}
//@-node:jimc.20100409112755.180:InitGL
//@+node:jimc.20100409112755.181:arGUIWindow::beginEventThread

int arGUIWindow::beginEventThread( void )
{
  if ( !_windowEventThread.beginThread( arGUIWindow::mainLoop, this ) ) {
    ar_log_error() << "beginEventThread: beginThread failed.\n";
    return -1;
  }

  _threaded = true;

  // wait for the window to actually be created, to avoid
  // race conditions from window manager trying to modify a
  // half-baked window.  Time out after 5 seconds.
  _creationMutex.lock("arGUIWindow::beginEventThread"); // lock with unlock AND with wait
    while ( !_creationFlag ) {
      if ( !_creationCond.wait( _creationMutex, 5000 ) )
        return -1;
    }
  _creationMutex.unlock();
  return 0;
}
//@-node:jimc.20100409112755.181:arGUIWindow::beginEventThread
//@+node:jimc.20100409112755.182:arGUIWindow::mainLoop

void arGUIWindow::mainLoop( void* data )
{
  arGUIWindow* w = (arGUIWindow*) data;
  if (w)
    w->_mainLoop();
}
//@-node:jimc.20100409112755.182:arGUIWindow::mainLoop
//@+node:jimc.20100409112755.183:arGUIWindow::_mainLoop

void arGUIWindow::_mainLoop( void )
{
  if ( _performWindowCreation() < 0 ) {
    // cleanup?
    return;
  }

  while ( _running ) {
    if ( _consumeWindowEvents() < 0 ) {
      // print error?
    }
  }
}
//@-node:jimc.20100409112755.183:arGUIWindow::_mainLoop
//@+node:jimc.20100409112755.184:arGUIWindow::_consumeWindowEvents

int arGUIWindow::_consumeWindowEvents( void )
{
  if ( !_running )
    return -1;

  if ( _GUIEventManager->consumeEvents( this, false ) < 0 )
    return -1;

  // Spin only a "little bit" (time out).
  _WMEventsMutex.lock("arGUIWindow::_consumeWindowEvents");
    if (_threaded && _WMEvents.empty())
      _WMEventsVar.wait(_WMEventsMutex, 100);
  _WMEventsMutex.unlock();

  // Queue may be empty, if the timeout was short.  That's OK.
  return _processWMEvents()<0 ? -1 : 0;
}
//@-node:jimc.20100409112755.184:arGUIWindow::_consumeWindowEvents
//@+node:jimc.20100409112755.185:arGUIWindow::getNextGUIEvent

arGUIInfo* arGUIWindow::getNextGUIEvent( void )
{
  if ( !_running )
    return NULL;

  arGUIInfo* result = _GUIEventManager->getNextEvent();
  // Provide these pointers to the callbacks.
  result->setWindowManager(_windowManager);
  result->setUserData(_userData);
  return result;
}
//@-node:jimc.20100409112755.185:arGUIWindow::getNextGUIEvent
//@+node:jimc.20100409112755.186:arGUIWindow::eventsPending

bool arGUIWindow::eventsPending( void ) const
{
  return _running && _GUIEventManager->eventsPending();
}
//@-node:jimc.20100409112755.186:arGUIWindow::eventsPending
//@+node:jimc.20100409112755.187:arGUIWindow::getGraphicsWindow

arGraphicsWindow* arGUIWindow::getGraphicsWindow( void )
{
  _graphicsWindowMutex.lock("arGUIWindow::getGraphicsWindow");
  return _graphicsWindow;
}
//@-node:jimc.20100409112755.187:arGUIWindow::getGraphicsWindow
//@+node:jimc.20100409112755.188:arGUIWindow::returnGraphicsWindow

void arGUIWindow::returnGraphicsWindow( void )
{
  _graphicsWindowMutex.unlock();
}
//@-node:jimc.20100409112755.188:arGUIWindow::returnGraphicsWindow
//@+node:jimc.20100409112755.189:arGUIWindow::setGraphicsWindow

void arGUIWindow::setGraphicsWindow( arGraphicsWindow* graphicsWindow )
{
  arGuard _(_creationMutex, "arGUIWindow::setGraphicsWindow creation");
  arGuard __(_graphicsWindowMutex, "arGUIWindow::setGraphicsWindow graphicsWindow");
  if ( _graphicsWindow ) {
    delete _graphicsWindow; // do we really own this?
  }
  _graphicsWindow = graphicsWindow;
}
//@-node:jimc.20100409112755.189:arGUIWindow::setGraphicsWindow
//@+node:jimc.20100409112755.190:arGUIWindow::addWMEvent

arWMEvent* arGUIWindow::addWMEvent( arGUIWindowInfo& wmEvent )
{
  if ( !_running )
    return NULL;

  arWMEvent* event = NULL;

  // every event's user data should be set
  if ( !wmEvent.getUserData() )
    wmEvent.setUserData( _userData );

  _usableEventsMutex.lock("arGUIWindow::addWMEvent usableEvents");
    // find an 'empty' event and recycle it
    for (EventIterator eitr = _usableEvents.begin();
         eitr != _usableEvents.end(); eitr++ ) {
      if ( (*eitr)->getDone() == 2 ) {
        event = *eitr;
        event->reset( wmEvent );
        break;
      }
    }
    // no usable events found, need to create a new one
    if ( !event ) {
      event = new arWMEvent( wmEvent );
      _usableEvents.push_back( event );
    }
  _usableEventsMutex.unlock();

  arGuard _(_WMEventsMutex, "arGUIWindow::addWMEvent WMEvents");
  _WMEvents.push( event );
  // If we are waiting in _consumeWindowEvents, release.
  _WMEventsVar.signal();
  return event;
}
//@-node:jimc.20100409112755.190:arGUIWindow::addWMEvent
//@+node:jimc.20100409112755.191:arGUIWindow::_processWMEvents

int arGUIWindow::_processWMEvents( void )
{
//  if ( !_running ) {
//     // print warning / return?
//  }

  arGuard _(_WMEventsMutex, "arGUIWindow::_processWMEvents");

  // process every wm event received in the last iteration
  while ( !_WMEvents.empty() ) {

    arWMEvent* wmEvent = _WMEvents.front();

    switch( wmEvent->getEvent().getState() ) {
      case AR_WINDOW_DRAW_LEFT:

        _drawHandler( true );

        _lastFrameTime = ar_time();
      break;

      case AR_WINDOW_DRAW_RIGHT:

        _drawHandler( false );

        _lastFrameTime = ar_time();
      break;

      case AR_WINDOW_SWAP:
        if ( swap() < 0 ) {
          ar_log_error() << "_processWMEvents: swap failed.\n";
        }
      break;

      case AR_WINDOW_MOVE:
        if ( move( wmEvent->getEvent().getPosX(), wmEvent->getEvent().getPosY() ) < 0 ) {
          ar_log_error() << "_processWMEvents: move failed.\n";
        }
      break;

      case AR_WINDOW_RESIZE:
        if ( resize( wmEvent->getEvent().getSizeX(), wmEvent->getEvent().getSizeY() ) < 0 ) {
          ar_log_error() << "_processWMEvents: resize failed.\n";
        }
      break;

      case AR_WINDOW_VIEWPORT:
        if ( setViewport( wmEvent->getEvent().getPosX(), wmEvent->getEvent().getPosY(),
                         wmEvent->getEvent().getSizeX(), wmEvent->getEvent().getSizeY() ) < 0 ) {
          ar_log_error() << "_processWMEvents: setViewport failed.\n";
        }
      break;

      case AR_WINDOW_FULLSCREEN:
        fullscreen();
      break;

      case AR_WINDOW_DECORATE:
        decorate( (wmEvent->getEvent().getFlag() == 1) );
      break;

      case AR_WINDOW_RAISE:
        raise( arZOrder( wmEvent->getEvent().getFlag() ) );
      break;

      case AR_WINDOW_CURSOR:
        setCursor( arCursor( wmEvent->getEvent().getFlag() ) );
      break;

      case AR_WINDOW_CLOSE:
        if ( _running ) {
          if ( _killWindow() < 0 ) {
            // print error?
          }
        }
      break;

      default:
        // print warning?
      break;
    }

    // Announce that this event has finally been processed.
    wmEvent->signal();

    _WMEvents.pop();
  }

  return 0;
}
//@-node:jimc.20100409112755.191:arGUIWindow::_processWMEvents
//@+node:jimc.20100409112755.192:arGUIWindow::_performWindowCreation

int arGUIWindow::_performWindowCreation( void )
{
  if ( _setupWindowCreation() < 0 ) {
    ar_log_error() << "arGUIWindow: _setupWindowCreation failed.\n";
    return -1;
  }

  if ( _windowCreation() < 0 ) {
    // Already warned.
    return -1;
  }

  if ( _tearDownWindowCreation() < 0 ) {
    ar_log_error() << "arGUIWindow: _tearDownWindowCreation failed.\n";
    return -1;
  }

  // windowInitGL callback does this instead
  // InitGL( getWidth(), getHeight() );

  // Announce that the window has been created and initialized.
  arGuard _(_creationMutex, "arGUIWindow::_performWindowCreation");
  _creationFlag = true;
  _creationCond.signal();
  return 0;
}
//@-node:jimc.20100409112755.192:arGUIWindow::_performWindowCreation
//@+node:jimc.20100409112755.193:arGUIWindow::_windowCreation

int arGUIWindow::_windowCreation( void )
{
#if defined( AR_USE_WIN_32 )

  int dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  if ( _windowConfig.getStereo() ) {
    ar_log_debug() << "Creating a stereo arGUIWindow.\n";
    dwFlags |= PFD_STEREO;
  } else {
    ar_log_debug() << "Creating a mono arGUIWindow.\n";
  }

  PIXELFORMATDESCRIPTOR pfd = {
    sizeof( PIXELFORMATDESCRIPTOR ),
    1,                                  // version number
    dwFlags,                            // window creation flags
    PFD_TYPE_RGBA,                      // request an RGBA format
    _windowConfig.getBpp(),             // select the color depth
    0, 0, 0, 0, 0, 0,                   // color bits are ignored
    0,                                  // no alpha buffer
    0,                                  // shift bit is ignored
    0,                                  // no accumulation buffer
    0, 0, 0, 0,                         // accumulation bits ignored
    16,                                 // 16 bit Z-buffer
    0,                                  // no stencil buffer
    0,                                  // no auxiliary buffer
    PFD_MAIN_PLANE,                     // main drawing layer
    0,                                  // reserved
    0, 0, 0                             // layer masks are ignored
  };

  DWORD windowExtendedStyle = WS_EX_APPWINDOW;
  DWORD windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;

  if ( _windowConfig.getDecorate() ) {
    windowStyle |= WS_OVERLAPPEDWINDOW;
  }
  else {
    windowStyle |= WS_POPUP;
    windowExtendedStyle |= WS_EX_TOOLWINDOW;
  }

  const int trueX = _windowConfig.getPosX();
  const int trueY = _windowConfig.getPosY();
  const int trueWidth = _windowConfig.getWidth();
  const int trueHeight = _windowConfig.getHeight();

  RECT windowRect = { trueX, trueY, trueX + trueWidth, trueY + trueHeight };

  // adjust window to true requested size since Win32 treats the width and
  // height as the whole window (including decorations) but we want it to be
  // just the client area
  if ( !AdjustWindowRectEx( &windowRect, windowStyle, 0, windowExtendedStyle ) ) {
    ar_log_error() << "_windowCreation: AdjustWindowRectEx failed.\n";
  }

  // create the OpenGL window
  // NOTE: the last argument needs to be a pointer to this object so that the
  // static event processing function in GUIEventManager can access the window
  if ( !( _windowHandle._hWnd = CreateWindowEx( windowExtendedStyle,
                                 _className.c_str(),
                                 _windowConfig.getTitle().c_str(),
                                 windowStyle,
                                 windowRect.left, windowRect.top,
                                 windowRect.right - windowRect.left,
                                 windowRect.bottom - windowRect.top,
                                 (HWND) NULL,
                                 (HMENU) NULL,
                                 _windowHandle._hInstance,
                                 (void*) this ) ) ) {
    ar_log_error() << "_windowCreation: CreateWindowEx failed.\n";
    _killWindow();
    return -1;
  }

  if ( !( _windowHandle._hDC = GetDC( _windowHandle._hWnd ) ) ) {
    ar_log_error() << "_windowCreation: GetDC failed.\n";
    _killWindow();
    return -1;
  }

  const GLuint PixelFormat = ChoosePixelFormat( _windowHandle._hDC, &pfd );
  if (!PixelFormat) {
    ar_log_error() << "_windowCreation: ChoosePixelFormat failed.\n";
    _killWindow();
    return -1;
  }

  if ( !SetPixelFormat( _windowHandle._hDC, PixelFormat, &pfd ) ) {
    ar_log_error() << "_windowCreation: SetPixelFormat failed.\n";
    _killWindow();
    return -1;
  }

  // should check if setting up stereo succeeded with DescribePixelFormat

  if ( !( _windowHandle._hRC = wglCreateContext( _windowHandle._hDC ) ) ) {
    ar_log_error() << "_windowCreation: wglCreateContext failed.\n";
    _killWindow();
    return -1;
  }

  if ( !wglMakeCurrent( _windowHandle._hDC, _windowHandle._hRC ) ) {
    ar_log_error() << "_windowCreation: wglMakeCurrent failed.\n";
    _killWindow();
    return -1;
  }

  _running = true;

  if ( _windowConfig.getFullscreen() ) {
    fullscreen();
  }

  _decorate = _windowConfig.getDecorate();

  ShowWindow( _windowHandle._hWnd, SW_SHOW );

  if ( !SetForegroundWindow( _windowHandle._hWnd ) ) {
    // Commonly happens on "caveinabox" multi-screen PCs.
    ar_log_remark() << "_windowCreation: SetForegroundWindow failed.\n";
  }

  if ( !SetFocus( _windowHandle._hWnd ) ) {
    ar_log_error() << "_windowCreation: SetFocus failure.\n";
  }

  // Not redundant, since SetForegroundWindow might have failed.
  if ( !_windowConfig.getFullscreen() ) {
    raise( _windowConfig.getZOrder() );
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  int attrList[ 32 ] = { GLX_RGBA,
                         GLX_DOUBLEBUFFER,
                         GLX_RED_SIZE,      4,
                         GLX_GREEN_SIZE,    4,
                         GLX_BLUE_SIZE,     4,
                         GLX_DEPTH_SIZE,   16,
                         None };

  if ( _windowConfig.getStereo() ) {
    attrList[ 10 ] = GLX_STEREO;
    attrList[ 11 ] = None;
  }

  _windowHandle._dpy = NULL;
  _windowHandle._vi = NULL;
  _windowHandle._wHints = None;

  // XOpenDisplay defaults to $DISPLAY, if passed NULL.
  const string disp_str = _windowConfig.getXDisplay();
  const char* disp = disp_str.empty() ? NULL : disp_str.c_str();
  _windowHandle._dpy = XOpenDisplay(disp);

  if ( !_windowHandle._dpy ) {
    ar_log_error() << "_windowCreation failed to open X11 display '" << disp << "'.\n";

    // Sometimes $DISPLAY is :1.0 but getXDisplay still says :0.0.
    // So fall back to $DISPLAY, and then to :1.0 .
    disp = ar_getenv("DISPLAY").c_str();
    _windowHandle._dpy = XOpenDisplay(disp);
    if ( !_windowHandle._dpy ) {
      const char* dispFallback = ":1.0";
      // Don't try dispFallback twice.
      if (!strcmp(disp, dispFallback) ||
          !(_windowHandle._dpy = XOpenDisplay(disp = dispFallback))) {
        ar_log_error() << "_windowCreation failed to open X11 display '" << disp << "'.\n";
        return -1;
      }
    }
  }

  _windowHandle._screen = DefaultScreen( _windowHandle._dpy );

  if ( !glXQueryExtension( _windowHandle._dpy, NULL, NULL ) ) {
    ar_log_error() << "_windowCreation: OpenGL GLX extensions not supported.\n";
    return -1;
  }

  if ( !( _windowHandle._vi = glXChooseVisual( _windowHandle._dpy, _windowHandle._screen, attrList ) ) ) {
    ar_log_error() << "_windowCreation failed to create double-buffered window.\n";
    // _killWindow();
    return -1;
  }

  _windowHandle._root = RootWindow( _windowHandle._dpy, _windowHandle._vi->screen );

  if ( !( _windowHandle._ctx = glXCreateContext( _windowHandle._dpy, _windowHandle._vi, NULL, GL_TRUE ) ) ) {
    ar_log_error() << "_windowCreation failed to create rendering context.\n";
    // _killWindow();
    return -1;
  }

  _windowHandle._attr.colormap = XCreateColormap(
    _windowHandle._dpy, _windowHandle._root, _windowHandle._vi->visual, AllocNone);
  _windowHandle._attr.border_pixel = 0;
  _windowHandle._attr.background_pixel = 0;
  _windowHandle._attr.background_pixmap = None;

  // tell X which events we want to be notified about
  _windowHandle._attr.event_mask = ExposureMask | StructureNotifyMask | VisibilityChangeMask |
                                   KeyPressMask | KeyReleaseMask |
                                   ButtonPressMask | ButtonReleaseMask |
                                   PointerMotionMask | ButtonMotionMask;

  unsigned long mask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;

  // NOTE: for now we aren't going to even try to use extensions as they
  // aren't supported on every platform and the #else clause is just a massage
  // of fullscreen() anyhow
  int trueX = 0;
  int trueY = 0;
  int trueWidth = 0;
  int trueHeight = 0;
  if ( 0 /* _windowConfig._fullscreen */ ) {

#ifdef HAVE_X11_EXTENSIONS

    XF86VidModeModeInfo **modes = NULL;
    int modeNum = 0;
    int bestMode = 0;
    int trueX = 0;
    int trueY = 0;
    XF86VidModeGetAllModeLines( _windowHandle._dpy, _windowHandle._screen, &modeNum, &modes );

    // check freeglut::glutGameMode for more about resetting the desktop to its original state
    _windowHandle._dMode = *modes[ 0 ];

    for( int i = 0; i < modeNum; i++) {
      if ( ( modes[ i ]->hdisplay == _windowConfig.getWidth() ) &&
          ( modes[ i ]->vdisplay == _windowConfig.getHeight() ) ) {
        bestMode = i;
        // break here? i.e. should the last or first usable mode we find be the
        // one we actually use?
      }
    }

    // if no modes are found above, bestMode defaults to modes[ 0 ] (the default
    // root window mode)

    // override the WM's ability to manage this window (result is that there is
    // no window decoration and we have to grab the keyboard ourself)
    _windowHandle._attr.override_redirect = True;
    mask |= CWOverrideRedirect;

    _windowHandle._win = XCreateWindow( _windowHandle._dpy, _windowHandle._root,
                          trueX, trueY,
                          modes[ bestMode ]->hdisplay, modes[ bestMode ]->vdisplay,
                          0, _windowHandle._vi->depth, InputOutput, _windowHandle._vi->visual,
                          mask, &_windowHandle._attr );

    _running = true;

    XF86VidModeSwitchToMode( _windowHandle._dpy, _windowHandle._screen, modes[ bestMode ] );
    XF86VidModeSetViewPort( _windowHandle._dpy, _windowHandle._screen, 0, 0 );

    XFree( modes );

#else // we dont have X11 extensions, just make the window the size of the desktop

    int x = 0, y = 0;
    Window w;

    const int trueWidth = DisplayWidth( _windowHandle._dpy, _windowHandle._vi->screen );
    const int trueHeight = DisplayHeight( _windowHandle._dpy, _windowHandle._vi->screen );
    trueX = 0;
    trueY = 0;

    _windowHandle._attr.override_redirect = True;
    mask |= CWOverrideRedirect;

    _windowHandle._win = XCreateWindow( _windowHandle._dpy, _windowHandle._root,
                           trueX, trueY, trueWidth, trueHeight,
                           0, _windowHandle._vi->depth, InputOutput, _windowHandle._vi->visual,
                           mask, &_windowHandle._attr );

    _running = true;

    XMoveResizeWindow( _windowHandle._dpy, _windowHandle._win,
                       trueX, trueY, trueWidth, trueHeight );

    XFlush( _windowHandle._dpy );

    // put the window in the correct position relative to the desktop
    XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                           trueX, trueY, &x, &y, &w );

    if ( x || y ) {
      XMoveWindow( _windowHandle._dpy, _windowHandle._win, -x, -y );
    }

#endif

    _fullscreen = true;
  }
  else {
    trueX = _windowConfig.getPosX();
    trueY = _windowConfig.getPosY();
    trueWidth = _windowConfig.getWidth();
    trueHeight = _windowConfig.getHeight();

    _windowHandle._win = XCreateWindow( _windowHandle._dpy, _windowHandle._root,
                           trueX, trueY, trueWidth, trueHeight,
                           0, _windowHandle._vi->depth, InputOutput, _windowHandle._vi->visual,
                           mask, &_windowHandle._attr );

    _running = true;
  }

  XSelectInput( _windowHandle._dpy, _windowHandle._win,
                _windowHandle._attr.event_mask | FocusChangeMask |
                PropertyChangeMask | KeymapStateMask );

  // regardless of what is set in XCreateWindow, these hints are what the WM
  // seems to honor
  XSizeHints sizeHints;
  memset( &sizeHints, 0, sizeof( XSizeHints ) );
  sizeHints.flags       = USPosition | USSize;
  sizeHints.x           = trueX;
  sizeHints.y           = trueY;
  sizeHints.width       = trueWidth;
  sizeHints.height      = trueHeight;
  sizeHints.base_width  = trueWidth;
  sizeHints.base_height = trueHeight;

  XWMHints wmHints;
  wmHints.flags         = StateHint;
  wmHints.initial_state = NormalState;

  const char* title = _windowConfig.getTitle().c_str();

  XTextProperty textProperty;
  XStringListToTextProperty( (char **) &title, 1, &textProperty );

  XSetWMProperties( _windowHandle._dpy, _windowHandle._win,
                    &textProperty, &textProperty, 0, 0,
                    &sizeHints, &wmHints, NULL );

  // need this atom so we can properly check for window close events in
  // arGUIEventManager::consumeEvents
  _windowHandle._wDelete = XInternAtom( _windowHandle._dpy, "WM_DELETE_WINDOW", True );
  XSetWMProtocols( _windowHandle._dpy, _windowHandle._win, &_windowHandle._wDelete, 1 );

  /*
  if ( _windowConfig._fullscreen ) {
    XGrabKeyboard( _windowHandle._dpy, _windowHandle._win, True, GrabModeAsync, GrabModeAsync, CurrentTime );
    XGrabPointer( _windowHandle._dpy, _windowHandle._win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync,
                  _windowHandle._win, None, CurrentTime );
  }
  */

  if ( _windowConfig.getFullscreen() ) {
    raise( _windowConfig.getZOrder() );
  }

  decorate( _windowConfig.getDecorate() );

  if ( _windowConfig.getFullscreen() ) {
    fullscreen();
  }

  XSync( _windowHandle._dpy, False );

  XMapWindow( _windowHandle._dpy, _windowHandle._win );

  glXMakeCurrent( _windowHandle._dpy, _windowHandle._win, _windowHandle._ctx );

  if ( !glXIsDirect( _windowHandle._dpy, _windowHandle._ctx ) ) {
    ar_log_error() << "No hardware acceleration available.\n";
  }

#endif

  // this should get properly set in arGUIEventManager, probably shouldn't
  // set it here
  _visible = true;

  setCursor( _windowConfig.getCursor() );

  return 0;
}
//@-node:jimc.20100409112755.193:arGUIWindow::_windowCreation
//@+node:jimc.20100409112755.194:arGUIWindow::_setupWindowCreation

int arGUIWindow::_setupWindowCreation( void )
{
#if defined( AR_USE_WIN_32 )

  // Win32 requires that a window be registered before its creation
  WNDCLASSEX windowClass;
  ZeroMemory( &windowClass, sizeof( WNDCLASSEX ) );
  windowClass.cbSize        = sizeof( WNDCLASSEX );
  windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;        // redraw window on any movement / resizing
  windowClass.lpfnWndProc   = (WNDPROC)(arGUIEventManager::windowProc);  // window message handler
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = _windowHandle._hInstance;
  windowClass.hbrBackground = NULL;                                      // OpenGL doesn't require a background
  windowClass.hCursor       = LoadCursor( NULL, IDC_ARROW );
  windowClass.hIcon         = LoadIcon( NULL, IDI_WINLOGO );
  windowClass.lpszMenuName  = NULL;
  // Each window has its own _className, to avoid identically named windows.
  windowClass.lpszClassName = _className.c_str();

  if ( !RegisterClassEx( &windowClass ) ) {
    ar_log_error() << "_setupWindowCreation: RegisterClassEx Failed.\n";
    return -1;
  }

#endif

  return 0;
}
//@-node:jimc.20100409112755.194:arGUIWindow::_setupWindowCreation
//@+node:jimc.20100409112755.195:arGUIWindow::_tearDownWindowCreation

int arGUIWindow::_tearDownWindowCreation( void )
{
  // now that the window is created, perform any necessary user-defined opengl
  // initialization.
  if ( _windowInitGLCallback ) {
    arGUIWindowInfo* windowInfo = new arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_INITGL, _ID );
    windowInfo->setPos( getPosX(), getPosY() );
    windowInfo->setSize( getWidth(), getHeight() );
    windowInfo->setUserData( _userData );
    _windowInitGLCallback( windowInfo );
    delete windowInfo;
  }
  return 0;
}
//@-node:jimc.20100409112755.195:arGUIWindow::_tearDownWindowCreation
//@+node:jimc.20100409112755.196:arGUIWindow::swap

int arGUIWindow::swap( void )
{
  if ( !_running ) {
    return -1;
  }

  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "swap failed to make context current.\n";
  }

  return _windowBuffer->swapBuffer( _windowHandle, _windowConfig.getStereo() );
}
//@-node:jimc.20100409112755.196:arGUIWindow::swap
//@+node:jimc.20100409112755.197:arGUIWindow::resize

int arGUIWindow::resize( int newWidth, int newHeight )
{
  if ( !_running ) {
    return -1;
  }

  // check arguments for sanity?

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "resize: could not make context current.\n";
  }
  */

  // avoid divide-by-0
  if ( newHeight == 0 ) {
    newHeight = 1;
  }

#if defined( AR_USE_WIN_32 )

  RECT rect;

  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "resize: GetWindowRect failed.\n";
  }

  if ( _decorate )
  {
    newWidth  += GetSystemMetrics( SM_CXSIZEFRAME ) * 2;
    newHeight += GetSystemMetrics( SM_CYSIZEFRAME ) * 2 +
                 GetSystemMetrics( SM_CYCAPTION );
  }

  if ( !SetWindowPos( _windowHandle._hWnd, HWND_TOP,
                     rect.left, rect.top, newWidth, newHeight,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_NOZORDER ) ) {
    // print error?
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XResizeWindow( _windowHandle._dpy, _windowHandle._win, newWidth, newHeight );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  // glViewport( 0, 0, width, height );

  // changing from fullscreen to less-than-fullscreen -> position the window at
  // the original coordinates and redecorate if necessary
  if ( _fullscreen ) {
    _fullscreen = false;

    // don't need to do this under win32 as the window didn't get undecorated
    // to go fullscreen
#if defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
    if ( _windowConfig.getDecorate() ) {
      decorate( true );
    }
#endif

    // move the window back to its original position
    move( _windowConfig.getPosX(), _windowConfig.getPosY() );

    // NOTE: really what we should do for the last two calls (move and
    // decorate) is keep some prior state (e.g. right before the fullscreen
    // call) and restore the window to that, instead of to the original values

#if defined( AR_USE_WIN_32 )
    // get rid of the HWND_TOPMOST flag, but still keep the window 'on top'
    SetWindowPos( _windowHandle._hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                  SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE );
#endif

    // for some reason this resize event doesn't get raised either (see note in
    // fullscreen())
    _GUIEventManager->addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RESIZE, _ID, 0,
                                                 getPosX(), getPosY(), getWidth(), getHeight() ) );
  }

  return 0;
}
//@-node:jimc.20100409112755.197:arGUIWindow::resize
//@+node:jimc.20100409112755.198:arGUIWindow::fullscreen

int arGUIWindow::fullscreen( void )
{
  if ( !_running || _fullscreen ) {
    return -1;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "fullscreen failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  RECT windowRect;

  windowRect.left   = 0;
  windowRect.top    = 0;
  windowRect.right  = GetSystemMetrics( SM_CXSCREEN );
  windowRect.bottom = GetSystemMetrics( SM_CYSCREEN );

  // It's a good idea to comment this out. If these lines are used, there
  // are many subtle interactions between whether or not the window has
  // decoration, whether the taskbar is set to be always on top, etc.
  // The price we pay is that a fullscreen window with decoration will
  // (a) fill the screen but (b) have its decoration visible. In other
  // words, fullscreen is different from GLUT's version of fullscreen.
  // Actually, that might be a good thing.
  // AdjustWindowRect( &windowRect,
  //                   WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
  //                   false );

  // set HWND_TOPMOST as sometimes the taskbar will stay on the top of the
  // window (this flag needs to be unset if coming out of fullscreen mode)
  SetWindowPos( _windowHandle._hWnd, HWND_TOPMOST,
                windowRect.left, windowRect.top,
                windowRect.right  - windowRect.left,
                windowRect.bottom - windowRect.top,
                SWP_NOACTIVATE | SWP_NOSENDCHANGING );

  // Raise a resize event so the user can adjust the viewport if necessary.
  // Only win32 needs this, as linux 'correctly' raises the event
  // (oddly, the move to 0x0 is raised, just not the resize).
  // Do not query the window for its believed x,y,width,height here:
  // these haven't yet been changed with a resize event.
  // That is a mistake on finicky Win32. Instead, re-feed it
  // the precomputed info. This lets the framework resize the viewport.
  _GUIEventManager->addEvent( arGUIWindowInfo( AR_WINDOW_EVENT,
                                               AR_WINDOW_RESIZE, _ID, 0,
                                               windowRect.left, windowRect.top,
                                               windowRect.right - windowRect.left,
                                               windowRect.bottom - windowRect.top ) );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  /*
  int x, y;
  Window w;

  XMoveResizeWindow( _windowHandle._dpy, _windowHandle._win,
                     0, 0,
                     DisplayWidth( _windowHandle._dpy, _windowHandle._vi->screen ),
                     DisplayHeight( _windowHandle._dpy, _windowHandle._vi->screen ) );

  XFlush( _windowHandle._dpy );

  XTranslateCoordinates( _windowHandle._dpy,  _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if ( x || y ) {
    XMoveWindow( _windowHandle._dpy,  _windowHandle._win, -x, -y );
  }
  */

  XWindowChanges changes;

  decorate( false );

  XLockDisplay( _windowHandle._dpy );

  changes.x = 0;
  changes.y = 0;
  changes.width =  DisplayWidth( _windowHandle._dpy, _windowHandle._vi->screen );
  changes.height =  DisplayHeight( _windowHandle._dpy, _windowHandle._vi->screen );

  XConfigureWindow( _windowHandle._dpy, _windowHandle._win,
                    CWX | CWY | CWWidth | CWHeight, &changes );

  XFlush( _windowHandle._dpy );

  XUnlockDisplay( _windowHandle._dpy );

  // just to be safe... (NOTE: windows doesn't need this since we specify the
  // HWND_TOPMOST flag)
  raise( AR_ZORDER_TOPMOST );

  // Linux *does* need this, despite the claim in the windows code above.
  _GUIEventManager->addEvent( arGUIWindowInfo( AR_WINDOW_EVENT,
                                               AR_WINDOW_RESIZE, _ID, 0,
                                               changes.x, changes.y,
                                               changes.width,
					       changes.height ) );


#endif

  _fullscreen = true;

  return 0;
}
//@-node:jimc.20100409112755.198:arGUIWindow::fullscreen
//@+node:jimc.20100409112755.199:arGUIWindow::setViewport

int arGUIWindow::setViewport( int newX, int newY, int newWidth, int newHeight )
{
  if ( !_running ) {
    return -1;
  }

  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "setviewport: failed to make context current.\n";
  }

  glViewport( newX, newY, newWidth, newHeight );
  return 0;
}
//@-node:jimc.20100409112755.199:arGUIWindow::setViewport
//@+node:jimc.20100409112755.200:arGUIWindow::decorate

void arGUIWindow::decorate( const bool decorate )
{
  if ( !_running || _fullscreen ) {
    return;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "decorateWindow: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  DWORD windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
  DWORD windowExtendedStyle = WS_EX_APPWINDOW;

  if ( !decorate ) {
    windowStyle |= WS_POPUP;
    windowExtendedStyle |= WS_EX_TOOLWINDOW;
  }
  else {
    windowStyle |= WS_OVERLAPPEDWINDOW;
  }

  SetWindowLong( _windowHandle._hWnd, GWL_STYLE, windowStyle );
  SetWindowLong( _windowHandle._hWnd, GWL_EXSTYLE, windowExtendedStyle );

  // needed to actually flush the changes
  SetWindowPos( _windowHandle._hWnd, HWND_TOP, 0, 0, 0, 0,
                SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  bool set = false;

  XLockDisplay( _windowHandle._dpy );

  if ( !decorate && _windowHandle._wHints == None ) {
    // try Motif hints
    if ( XInternAtom( _windowHandle._dpy, "_MOTIF_WM_HINTS", False ) != None ) {
      _windowHandle._wHints = XInternAtom( _windowHandle._dpy, "_MOTIF_WM_HINTS", False );

      MotifWmHints hints = { (1L << 1), 0, 0, 0, 0 };
      XChangeProperty( _windowHandle._dpy, _windowHandle._win,
                       _windowHandle._wHints, _windowHandle._wHints, 32, PropModeReplace,
                       (unsigned char *) &hints, sizeof( hints ) / sizeof( long ) );
    }
    else
    // try KDE hints
    if ( XInternAtom( _windowHandle._dpy, "KWM_WIN_DECORATION", False ) != None ) {
      _windowHandle._wHints = XInternAtom( _windowHandle._dpy, "KWM_WIN_DECORATION", False );

      long hints = 0;
      XChangeProperty( _windowHandle._dpy, _windowHandle._win,
                       _windowHandle._wHints, _windowHandle._wHints, 32, PropModeReplace,
                       (unsigned char *) &hints, sizeof( hints ) / sizeof( long ) );
    }
    else
    // try GNOME hints
    if ( XInternAtom( _windowHandle._dpy, "_WIN_HINTS", False ) != None ) {
      _windowHandle._wHints = XInternAtom( _windowHandle._dpy, "_WIN_HINTS", False );

      long hints = 0;
      XChangeProperty( _windowHandle._dpy, _windowHandle._win,
                       _windowHandle._wHints, _windowHandle._wHints, 32, PropModeReplace,
                       (unsigned char *) &hints, sizeof( hints ) / sizeof( long ) );
    }
    else {
      // couldn't find a decoration hint that matches this window manager.
      // print error/warning?
    }

    set = true;
  }
  else if ( decorate && _windowHandle._wHints != None ) {
    XDeleteProperty( _windowHandle._dpy, _windowHandle._win, _windowHandle._wHints );
    _windowHandle._wHints = None;

    set = true;
  }

  // Under Xandros 2.0 (a tweaked KDE 3.1.4) the window has to be
  // remapped for redecoration to work.  Unfortunately, this
  // causes OS X to blank out the client area of the window.
  // But if the window is /not/ remapped, redecoration again fails under OS X,
  // so for now windows on OS X can't be redecorated
  // (which has implications for going in and out of fullscreen mode as well).
  // Slackware 10.1 (KDE 3.4.0) doesn't need any remapping at all.
  if ( set ) {
#if !defined( AR_USE_DARWIN )
    // While Xandros needs these calls to redecorate a window at
    // runtime, they also seem to break fullscreen
    // (since the window must be undecorated before going fullscreen) in that
    // the fullscreen window does not cover the taskbar.  As above, under
    // Slackware this works. Since fullscreen trumps
    // redecoration, they are commented out.

    // XUnmapWindow( _windowHandle._dpy, _windowHandle._win );
    // XMapWindow( _windowHandle._dpy, _windowHandle._win );
#endif
  }

  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  _decorate = decorate;
}
//@-node:jimc.20100409112755.200:arGUIWindow::decorate
//@+node:jimc.20100409112755.201:arGUIWindow::raise

void arGUIWindow::raise( const arZOrder zorder )
{
  if ( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "raise: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  const HWND flag =
    zorder == AR_ZORDER_NORMAL ? HWND_NOTOPMOST :
    zorder == AR_ZORDER_TOP    ? HWND_TOP :
                                 HWND_TOPMOST; // zorder == AR_ZORDER_TOPMOST

  SetWindowPos( _windowHandle._hWnd, flag,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE );

  _zorder = zorder;

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  (void)zorder; // avoid compiler warning
  XLockDisplay( _windowHandle._dpy );
  XRaiseWindow( _windowHandle._dpy, _windowHandle._win );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  _visible = true;
}
//@-node:jimc.20100409112755.201:arGUIWindow::raise
//@+node:jimc.20100409112755.202:arGUIWindow::lower

void arGUIWindow::lower( void )
{
  if ( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "lowerWindow: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  SetWindowPos( _windowHandle._hWnd, HWND_BOTTOM,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XLowerWindow( _windowHandle._dpy, _windowHandle._win );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  _visible = false;
}
//@-node:jimc.20100409112755.202:arGUIWindow::lower
//@+node:jimc.20100409112755.203:arGUIWindow::minimize

void arGUIWindow::minimize( void )
{
  if ( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "minimizeWindow: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  ShowWindow( _windowHandle._hWnd, SW_MINIMIZE );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XIconifyWindow( _windowHandle._dpy, _windowHandle._win, _windowHandle._screen );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  _visible = false;
}
//@-node:jimc.20100409112755.203:arGUIWindow::minimize
//@+node:jimc.20100409112755.204:arGUIWindow::restore

void arGUIWindow::restore( void )
{
  if ( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "restoreWindow: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  ShowWindow( _windowHandle._hWnd, SW_SHOW );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XMapWindow( _windowHandle._dpy, _windowHandle._win );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  _visible = true;
}
//@-node:jimc.20100409112755.204:arGUIWindow::restore
//@+node:jimc.20100409112755.205:arGUIWindow::move

int arGUIWindow::move( int newX, int newY )
{
  if ( !_running || _fullscreen ) {
    return -1;
  }

  // check arguments for sanity?

  /*
  // is this necessary in this function?
  if ( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    ar_log_error() << "move: failed to make context current.\n";
  }
  */

#if defined( AR_USE_WIN_32 )

  /*
  RECT rect;

  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "move: GetWindowRect failed.\n";
    return -1;
  }

  if ( !MoveWindow( _windowHandle._hWnd, newX, newY,
                   rect.right - rect.left, rect.bottom - rect.top, false ) ) {
    // print error
  }
  */

  SetWindowPos( _windowHandle._hWnd, HWND_TOP, newX, newY, 0, 0,
                SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOSIZE );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XMoveWindow( _windowHandle._dpy, _windowHandle._win, newX, newY );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

#endif

  return 0;
}
//@-node:jimc.20100409112755.205:arGUIWindow::move
//@+node:jimc.20100409112755.206:arGUIWindow::setCursor

arCursor arGUIWindow::setCursor( arCursor cursor )
{
  if ( !_running ) {
    ar_log_remark() << "arGUIWindow ignoring setCursor while not running.\n";
    return _cursor;
  }

  // FIXME: must be a better way to check for valid cursor....
  switch( cursor ) {
    case AR_CURSOR_ARROW:
    case AR_CURSOR_WAIT:
    case AR_CURSOR_HELP:
    case AR_CURSOR_NONE:
      break;

    default:
      ar_log_remark() << "arGUIWindow ignoring invalid cursor type.\n";
      return _cursor;
  }

#if defined( AR_USE_WIN_32 )

  static LPSTR cursorCache[] = { IDC_ARROW, IDC_HELP, IDC_WAIT };

  switch( cursor ) {
    case AR_CURSOR_NONE:
      SetCursor( NULL );
      SetClassLong( _windowHandle._hWnd, GCL_HCURSOR, (LONG) NULL );
    break;

    case AR_CURSOR_ARROW:
    case AR_CURSOR_HELP:
    case AR_CURSOR_WAIT:
      SetCursor( LoadCursor( NULL, cursorCache[ cursor ] ) );
      SetClassLong( _windowHandle._hWnd, GCL_HCURSOR,
                    (LONG) LoadCursor( NULL, cursorCache[ cursor ] ) );
    break;
  }

  _cursor = cursor;

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  // Using the caching code below seems to cause segfaults on Linux when
  // running multiple windows in the master/slave framework.
  /*
  static cursorCacheEntry cursorCache[] = {
    { XC_arrow,           None },
    { XC_question_arrow,  None },
    { XC_watch,           None } };

  static Cursor cursorNone = None;
  */

  Cursor XCursor = None;

  switch( cursor ) {
    case AR_CURSOR_NONE:
    {
      // At first, this code was doing caching via cursorNone. However,
      // that lead to instability. Consequently, no caching now.
      char cursorNoneBits[ 32 ];
      XColor dontCare;
      Pixmap cursorNonePixmap;

      memset( cursorNoneBits, 0, sizeof( cursorNoneBits ) );
      memset( &dontCare, 0, sizeof( dontCare ) );

      cursorNonePixmap = XCreateBitmapFromData( _windowHandle._dpy,
                                                _windowHandle._root,
                                                cursorNoneBits, 16, 16 );
      if ( cursorNonePixmap == None ) {
        ar_log_error() << "failed to create AR_CURSOR_NONE.\n";
        return _cursor;
      }

      XCursor = XCreatePixmapCursor( _windowHandle._dpy,
                                     cursorNonePixmap, cursorNonePixmap,
                                     &dontCare, &dontCare, 0, 0 );

      XFreePixmap( _windowHandle._dpy, cursorNonePixmap );
    }
    break;

    case AR_CURSOR_ARROW:
      XCursor = XCreateFontCursor( _windowHandle._dpy, XC_top_left_arrow );
    break;

    case AR_CURSOR_WAIT:
      XCursor = XCreateFontCursor( _windowHandle._dpy, XC_watch );
    break;

    case AR_CURSOR_HELP:
      XCursor = XCreateFontCursor( _windowHandle._dpy, XC_question_arrow );
    break;
  }

  if ( XCursor == None ) {
    ar_log_error() << "failed to create requested X cursor.\n";
    return _cursor;
  }

  _cursor = cursor;
  XDefineCursor( _windowHandle._dpy, _windowHandle._win, XCursor );

#endif

  return _cursor;
}
//@-node:jimc.20100409112755.206:arGUIWindow::setCursor
//@+node:jimc.20100409112755.207:arGUIWindow::setTitle

void arGUIWindow::setTitle( const std::string& title )
{
  if ( !_running ) {
    return;
  }

#if defined( AR_USE_WIN_32 )

  SetWindowText( _windowHandle._hWnd, title.c_str() );

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XTextProperty text;
  text.value = (unsigned char *) title.c_str();
  text.encoding = XA_STRING;
  text.format = 8;
  text.nitems = title.length();
  XSetWMName( _windowHandle._dpy, _windowHandle._win, &text );

#endif
}
//@-node:jimc.20100409112755.207:arGUIWindow::setTitle
//@+node:jimc.20100409112755.208:arGUIWindow::getWidth

int arGUIWindow::getWidth( void ) const
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  RECT rect;

  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "getWidth: GetWindowRect failed.\n";
    return -1;
  }

  if ( _decorate ) {
    rect.left  += GetSystemMetrics( SM_CXSIZEFRAME );
    rect.right -= GetSystemMetrics( SM_CXSIZEFRAME );
  }

  return rect.right - rect.left;

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XWindowAttributes winAttributes;

  XLockDisplay( _windowHandle._dpy );
  XGetWindowAttributes( _windowHandle._dpy, _windowHandle._win, &winAttributes);
  XUnlockDisplay( _windowHandle._dpy );
  return winAttributes.width;

#endif
}
//@-node:jimc.20100409112755.208:arGUIWindow::getWidth
//@+node:jimc.20100409112755.209:arGUIWindow::getHeight

int arGUIWindow::getHeight( void ) const
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  RECT rect;

  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "getHeight: GetWindowRect failed.\n";
    return -1;
  }
  if ( _decorate ) {
    rect.top    += GetSystemMetrics( SM_CYSIZEFRAME ) + GetSystemMetrics( SM_CYCAPTION );
    rect.bottom -= GetSystemMetrics( SM_CYSIZEFRAME );
  }
  return rect.bottom - rect.top;

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XWindowAttributes winAttributes;
  XLockDisplay( _windowHandle._dpy );
  XGetWindowAttributes(_windowHandle._dpy, _windowHandle._win, &winAttributes);
  XUnlockDisplay( _windowHandle._dpy );
  return winAttributes.height;

#endif
}
//@-node:jimc.20100409112755.209:arGUIWindow::getHeight
//@+node:jimc.20100409112755.210:arGUIWindow::getPosX

int arGUIWindow::getPosX( void ) const
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  RECT rect;
  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "getPosX: GetWindowRect failed.\n";
    return -1;
  }

  if ( 0 /*_windowConfig._decorate*/ ) {
    rect.left += GetSystemMetrics( SM_CXSIZEFRAME );
  }
  return rect.left;

#elif defined( AR_USE_LINUX )  || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  int x, y, bx, by;
  Window w;

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if ( _windowConfig.getDecorate() ) {
    XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win,
                           w, 0, 0, &bx, &by, &w );
    x -= bx;
  }

  XUnlockDisplay( _windowHandle._dpy );
  return x;

#endif
}
//@-node:jimc.20100409112755.210:arGUIWindow::getPosX
//@+node:jimc.20100409112755.211:arGUIWindow::getPosY

int arGUIWindow::getPosY( void ) const
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  RECT rect;

  if ( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    ar_log_error() << "getPosY: GetWindowRect failed.\n";
    return -1;
  }

  if ( 0 /*_windowConfig._decorate*/ ) {
    rect.top += GetSystemMetrics( SM_CYSIZEFRAME ) + GetSystemMetrics( SM_CYCAPTION );
  }

  return rect.top;

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  int x, y, bx, by;
  Window w;

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if ( _windowConfig.getDecorate() ) {
    XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win,
                           w, 0, 0, &bx, &by, &w );
    y -= by;
  }

  XUnlockDisplay( _windowHandle._dpy );
  return y;

#endif
}
//@-node:jimc.20100409112755.211:arGUIWindow::getPosY
//@+node:jimc.20100409112755.212:arGUIWindow::_changeScreenResolution

int arGUIWindow::_changeScreenResolution( void )
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  DEVMODE dmScreenSettings;
  ZeroMemory( &dmScreenSettings, sizeof( DEVMODE ) );
  dmScreenSettings.dmSize       = sizeof( DEVMODE );
  dmScreenSettings.dmPelsWidth  = _windowConfig.getWidth();
  dmScreenSettings.dmPelsHeight = _windowConfig.getHeight();
  dmScreenSettings.dmBitsPerPel = _windowConfig.getBpp();
  dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

  if ( ChangeDisplaySettings( &dmScreenSettings, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL ) {
    ar_log_error() << "_changeScreenResolution: ChangeDisplaySettinges failed.\n";
    return -1;
  }

#endif

  return 0;
}
//@-node:jimc.20100409112755.212:arGUIWindow::_changeScreenResolution
//@+node:jimc.20100409112755.213:arGUIWindow::_killWindow

// if this function is called when the window is not running there will be a
// whole bunch of errors and probably some segfaults, but we can't check for
// running because of control flow in the above functions...
int arGUIWindow::_killWindow( void )
{
  // in multi-threading mode this function could probably be called while other
  // functions are still executing on the objects that get deleted below, need
  // some way of ensuring we don't execute the rest of this function until
  // everyone else is through with what they were doing

#if defined( AR_USE_WIN_32 )

  // switch back to the desktop
  if ( _fullscreen ) {
    /*
    if ( ChangeDisplaySettings( NULL, 0 ) != DISP_CHANGE_SUCCESSFUL ) {
      ar_log_error() << "_killWindow: ChangeDisplaySettings failed.\n";
      return -1;
    }
    */

    ShowCursor( true );
  }

  if ( _windowHandle._hRC ) {
    if ( !wglMakeCurrent( NULL, NULL ) ) {
      ar_log_error() << "_killWindow: release of DC and RC failed.\n";
    }

    if ( !wglDeleteContext( _windowHandle._hRC ) ) {
      ar_log_error() << "_killWindow: delete RC failed.\n";
    }

    _windowHandle._hRC = NULL;
  }

  if ( _windowHandle._hDC && !ReleaseDC( _windowHandle._hWnd, _windowHandle._hDC ) ) {
    ar_log_error() << "_killWindow: release DC failed.\n";
    _windowHandle._hDC = NULL;
  }

  if ( _windowHandle._hWnd && !DestroyWindow( _windowHandle._hWnd ) ) {
    ar_log_error() << "_killWindow: could not release hWnd.\n";
    _windowHandle._hWnd = NULL;
  }

  if ( !UnregisterClass( _className.c_str(), _windowHandle._hInstance ) ) {
    ar_log_error() << "_killWindow: failed to unregister class.\n";
    _windowHandle._hInstance = NULL;
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  if ( _windowHandle._ctx ) {
    if ( !glXMakeCurrent( _windowHandle._dpy, None, NULL ) ) {
      ar_log_error() << "_killWindow failed to release drawing context.\n";
    }

    glXDestroyContext( _windowHandle._dpy, _windowHandle._ctx );

    _windowHandle._ctx = NULL;
  }

  // switch back to the desktop
  if ( _fullscreen ) {
#ifdef HAVE_X11_EXTENSIONS
      XF86VidModeSwitchToMode( _windowHandle._dpy, _windowHandle._screen, &_windowHandle._dMode );
      XF86VidModeSetViewPort( _windowHandle._dpy, _windowHandle._screen, 0, 0 );
#endif
  }

  if ( XCloseDisplay( _windowHandle._dpy ) == BadGC ) {
    ar_log_error() << "_killWindow failed to close display.\n";
  }

#endif

  _GUIEventManager->setActive( false );
  _running = false;

  // Report that the window thread is exiting.
  arGuard _(_destructionMutex, "arGUIWindow::_killWindow");
  _destructionFlag = true;
  _destructionCond.signal();
  return 0;
}
//@-node:jimc.20100409112755.213:arGUIWindow::_killWindow
//@+node:jimc.20100409112755.214:arGUIWindow::makeCurrent

int arGUIWindow::makeCurrent( bool release )
{
  if ( !_running ) {
    return -1;
  }

#if defined( AR_USE_WIN_32 )

  if ( release ) {
    if ( !wglMakeCurrent( NULL, NULL ) ) {
      return -1;
    }
  }
  else if ( ( wglGetCurrentContext() == _windowHandle._hRC ) &&
           ( wglGetCurrentDC() == _windowHandle._hDC ) ) {
    // already current (do this check so we aren't constantly switching
    // contexts and getting hit by the flushes that that incurs)
    return 0;
  }
  if ( !wglMakeCurrent( _windowHandle._hDC, _windowHandle._hRC ) ) {
    return -1;
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  if ( release ) {
    if ( !glXMakeCurrent( _windowHandle._dpy, None, NULL ) ) {
      XUnlockDisplay( _windowHandle._dpy );
      return -1;
    }
  }
  else if ( ( glXGetCurrentContext() == _windowHandle._ctx ) &&
           ( glXGetCurrentDisplay() == _windowHandle._dpy ) ) {
    // already current (do this check so we aren't constantly switching
    // contexts and getting hit by the flushes that that incurs)
    XUnlockDisplay( _windowHandle._dpy );
    return 0;
  }
  if ( !glXMakeCurrent( _windowHandle._dpy, _windowHandle._win, _windowHandle._ctx ) ) {
    XUnlockDisplay( _windowHandle._dpy );
    return -1;
  }

  XUnlockDisplay( _windowHandle._dpy );

#endif

  return 0;
}
//@-node:jimc.20100409112755.214:arGUIWindow::makeCurrent
//@-others

//@-node:jimc.20100409112755.164:@thin graphics\arGUIWindow.cpp
//@-leo
