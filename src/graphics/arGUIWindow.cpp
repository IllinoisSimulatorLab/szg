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

#include <iostream>

#include "arStructuredData.h"
#include "arDataUtilities.h"
#include "arWildcatUtilities.h"

#include "arGUIEventManager.h"
#include "arGUIInfo.h"
#include "arGUIWindow.h"

// convenience define
const double USEC = 1000000.0;

// the fall-throughs in these operators /might/ be 'dangerous' (e.g. a
// drawcallback expecting a windowinfo and/or a graphicsWindow gets passed
// NULL(s)), but they *should* be robust to deal with such a situation.  This
// allows us a bit more flexibility in how callbacks are registered and used.
void arDefaultGUIRenderCallback::operator()( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow ) {
  if( _drawCallback0 ) {
    _drawCallback0( windowInfo, graphicsWindow );
  }
  else if( _drawCallback1 ) {
    _drawCallback1( windowInfo );
  }
}

void arDefaultGUIRenderCallback::operator()( arGUIWindowInfo* windowInfo ) {
  if( _drawCallback1 ) {
    _drawCallback1( windowInfo );
  }
  else if( _drawCallback0 ) {
    _drawCallback0( windowInfo, NULL );
  }
}

void arDefaultGUIRenderCallback::operator()( arGraphicsWindow& graphicsWindow, arViewport& viewport ) {
  if( _drawCallback2 ) {
    _drawCallback2( graphicsWindow, viewport );
  }
  else if( _drawCallback0 ) {
    _drawCallback0( NULL, NULL );
  }
  else if( _drawCallback1 ) {
    _drawCallback1( NULL );
  }
}

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

arGUIWindowConfig::~arGUIWindowConfig( void )
{
}

arWMEvent::arWMEvent( const arGUIWindowInfo& event ) :
  _event( event ),
  _conditionFlag( false ),
  _done( 0 )
{
  ar_mutex_init( &_eventMutex );
  ar_mutex_init( &_doneMutex );
}

void arWMEvent::reset( const arGUIWindowInfo& event )
{
  _event = event;

  ar_mutex_lock( &_eventMutex );
    _conditionFlag = false;
  ar_mutex_unlock( &_eventMutex );

  ar_mutex_lock( &_doneMutex );
    _done = 0;
  ar_mutex_unlock( &_doneMutex );
}

void arWMEvent::wait( const bool blocking )
{
  if( blocking ) {
    ar_mutex_lock( &_eventMutex );
    while( !_conditionFlag ) {
      if( !_eventCond.wait( &_eventMutex ) ) {
        // print error?
      }
    }
    ar_mutex_unlock( &_eventMutex );
  }

  // even if not blocking, _done still needs to be updated to signal
  // that this event can be re-used if necessary
  ar_mutex_lock( &_doneMutex );
    ++_done;
  ar_mutex_unlock( &_doneMutex );
}

void arWMEvent::signal( void )
{
  ar_mutex_lock( &_eventMutex );
    _conditionFlag = true;
    _eventCond.signal();
  ar_mutex_unlock( &_eventMutex );

  ar_mutex_lock( &_doneMutex );
    ++_done;
  ar_mutex_unlock( &_doneMutex );
}

arWMEvent::~arWMEvent( void )
{
}

arGUIWindowBuffer::arGUIWindowBuffer( bool dblBuf ) :
  _dblBuf( dblBuf )
{
}

arGUIWindowBuffer::~arGUIWindowBuffer( void )
{
}

int arGUIWindowBuffer::swapBuffer( const arGUIWindowHandle& windowHandle, const bool stereo ) const
{
  // NOTE: since this should only be called from arGUIWindow, we've already
  // ensured that the correct gl context is on deck in arGUIWindow::swap

  // call glFlush before we swap buffers?

#if defined( AR_USE_WIN_32 )

  if( stereo ) {
    // according to http://www.stereographics.com/support/developers/pcsdk.htm
    // this is the correct function to use in active stereo mode though it's
    // not the one glut/freeglut use...
    if( !wglSwapLayerBuffers( windowHandle._hDC, WGL_SWAP_MAIN_PLANE ) ) {
      std::cerr << "swapBuffer: wglSwapLayerBuffers error" << std::endl;
    }
  }
  else {
    if( !SwapBuffers( windowHandle._hDC ) ) {
      std::cerr << "swapBuffer: SwapBuffers error" << std::endl;
    }
  }

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( windowHandle._dpy );
  glXSwapBuffers( windowHandle._dpy, windowHandle._win );
  XUnlockDisplay( windowHandle._dpy );

#endif

  return 0;
}

arGUIWindow::arGUIWindow( int ID, arGUIWindowConfig windowConfig,
                          void (*windowInitGLCallback)( arGUIWindowInfo* ),
                          void* userData ) :
  _ID( ID ),
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
  _creationFlag( false ),
  _destructionFlag( false ),
  _userData( userData ),
  _windowManager( NULL ),
  _graphicsWindow( NULL )
{
  // construct a unique class name for window registration under Win32
  std::stringstream ss; ss << _ID;
  _className = std::string( _windowConfig.getTitle() + ss.str() );

  _windowBuffer = new arGUIWindowBuffer( true );

  _GUIEventManager = new arGUIEventManager( _userData );

  _lastFrameTime = ar_time();

  ar_mutex_init( &_WMEventsMutex );
  ar_mutex_init( &_usableEventsMutex );
  ar_mutex_init( &_creationMutex );
  ar_mutex_init( &_destructionMutex );
  ar_mutex_init( &_graphicsWindowMutex );
}

arGUIWindow::~arGUIWindow( void )
{
  if( _running ) {
    if( _killWindow() < 0 ) {
      // print error?
    }
  }

  // delete both event queues (what if the wm is still holding onto a handle?)
  if( _GUIEventManager ) {
    delete _GUIEventManager;
  }

  if( _windowBuffer ) {
    delete _windowBuffer;
  }

  if( _drawCallback ) {
    delete _drawCallback;
  }
}

void arGUIWindow::registerDrawCallback( arGUIRenderCallback* drawCallback )
{
  ar_mutex_lock(&_creationMutex);
  if( _drawCallback ) {
    // print warning that previous callback is being overwritten?
    delete _drawCallback;
  }

  if( !drawCallback ) {
    // print warning that there is now no draw callback?
    _drawCallback = NULL;
  }
  else {
    _drawCallback = drawCallback;
  }
  ar_mutex_unlock(&_creationMutex);
}

void arGUIWindow::_drawHandler( void )
{
  ar_mutex_lock(&_creationMutex);
  if( _running && _drawCallback ) {

    // need to ensure (in non-threaded mode) that this window's opengl context
    // is in fact current
    if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
      std::cerr << "_drawHandler: could not make context current" << std::endl;
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

    arGUIWindowInfo* windowInfo = new arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DRAW );
    windowInfo->setWindowID( _ID );

    // always try to pass through the graphicswindow, if the user has
    // registered a callback that does not take it, the callback class will
    // handle letting that fall through
    (*_drawCallback)( windowInfo, _graphicsWindow );

    delete windowInfo;

    #if 0 // defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
    XUnlockDisplay( _windowHandle._dpy );
    #endif
  }
  ar_mutex_unlock(&_creationMutex);
}

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

int arGUIWindow::beginEventThread( void )
{
  if( !_windowEventThread.beginThread( arGUIWindow::mainLoop, this ) ) {
    std::cerr << "beginEventThread: beginThread Error" << std::endl;
    return -1;
  }

  _threaded = true;

  // wait for the window to actually be created to avoid
  // race conditions from window manager calls trying to operate on a
  // half-baked window.  Give up after 5 seconds.
  ar_mutex_lock( &_creationMutex );
    while( !_creationFlag ) {
      if( !_creationCond.wait( &_creationMutex, 5000 ) )
	return -1;
    }
  ar_mutex_unlock( &_creationMutex );
  return 0;
}

void arGUIWindow::mainLoop( void* data )
{
  arGUIWindow* w = (arGUIWindow*) data;
  if (w)
    w->_mainLoop();
}

void arGUIWindow::_mainLoop( void )
{
  if( _performWindowCreation() < 0 ) {
    // cleanup?
    return;
  }

  while( _running ) {
    if( _consumeWindowEvents() < 0 ) {
      // print error?
    }
  }
}

int arGUIWindow::_consumeWindowEvents( void )
{
  if( !_running )
    return -1;

  if( _GUIEventManager->consumeEvents( this, false ) < 0 )
    return -1;

  // Only spin a "little bit". Note that there is a time-out on the wait.
  // (which guarantees we won't wait forever). Thus, we can, conceivably,
  // make it to _processWMEvents with nothing in the queue. But that is OK.
  ar_mutex_lock(&_WMEventsMutex);
    if (_threaded && _WMEvents.empty())
      _WMEventsVar.wait(&_WMEventsMutex, 100);
  ar_mutex_unlock(&_WMEventsMutex);

  if( _processWMEvents() < 0 )
    return -1;

  return 0;
}

arGUIInfo* arGUIWindow::getNextGUIEvent( void )
{
  if( !_running )
    return NULL;

  arGUIInfo* result = _GUIEventManager->getNextEvent();
  // Must provide these pointers to the callbacks.
  result->setWindowManager(_windowManager);
  result->setUserData(_userData);
  return result;
}

bool arGUIWindow::eventsPending( void ) const
{
  if( !_running )
    return false;
  return _GUIEventManager->eventsPending();
}

arGraphicsWindow* arGUIWindow::getGraphicsWindow( void )
{
  ar_mutex_lock( &_graphicsWindowMutex );
  return _graphicsWindow;
}

void arGUIWindow::returnGraphicsWindow( void )
{
  ar_mutex_unlock( &_graphicsWindowMutex );
}

void arGUIWindow::setGraphicsWindow( arGraphicsWindow* graphicsWindow )
{
  ar_mutex_lock(&_creationMutex);
  ar_mutex_lock(&_graphicsWindowMutex);
    if( _graphicsWindow ) {
      // are we sure we own this graphics window?
      delete _graphicsWindow;
    }
    _graphicsWindow = graphicsWindow;
  ar_mutex_unlock(&_graphicsWindowMutex);
  ar_mutex_unlock(&_creationMutex);
}

arWMEvent* arGUIWindow::addWMEvent( arGUIWindowInfo& wmEvent )
{
  if( !_running )
    return NULL;

  arWMEvent* event = NULL;

  // every event's user data should be set
  if( !wmEvent.getUserData() )
    wmEvent.setUserData( _userData );

  EventIterator eitr;

  ar_mutex_lock( &_usableEventsMutex );

  // find an 'empty' event and recycle it
  for( eitr = _usableEvents.begin(); eitr != _usableEvents.end(); eitr++ ) {
    if( (*eitr)->getDone() == 2 ) {
      event = *eitr;
      event->reset( wmEvent );
      break;
    }
  }

  // no usable events found, need to create a new one
  if( !event ) {
    event = new arWMEvent( wmEvent );
    _usableEvents.push_back( event );
  }

  ar_mutex_unlock( &_usableEventsMutex );

  ar_mutex_lock( &_WMEventsMutex );
    _WMEvents.push( event );
    // If we are waiting in _consumeWindowEvents, go ahead and release.
    _WMEventsVar.signal();
  ar_mutex_unlock( &_WMEventsMutex );
  return event;
}

int arGUIWindow::_processWMEvents( void )
{
  if( !_running ) {
     // print warning / return?
  }

  ar_mutex_lock( &_WMEventsMutex );

  // process every wm event received in the last iteration
  while( !_WMEvents.empty() ) {

    arWMEvent* wmEvent = _WMEvents.front();

    switch( wmEvent->getEvent().getState() ) {
      case AR_WINDOW_DRAW:

        // NOTE: for now we won't worry about _Hz, it complicates things quite
        // a bit as far as control flow in different modes is concerned and is
        // probably not that important a feature, revisit it when arGUI has
        // been successfully integrated into syzygy.
        if( 0 /* _windowConfig._Hz > 0 */ ) {
          ar_timeval currentTime = ar_time();

          // NOTE: assumes the user won't want less than 1 frame per second
          int uSec = int( USEC / double( _windowConfig.getHz() ) );
          ar_timeval nextFrameTime( _lastFrameTime.sec, _lastFrameTime.usec + ( uSec > int( USEC ) ? int( USEC ) : uSec ) );

          // rollover the microseconds
          if( nextFrameTime.usec > 1000000 ) {
            nextFrameTime.sec++;
            nextFrameTime.usec = nextFrameTime.usec - 1000000;
          }

          // relinquish the cpu until it's time to draw the next frame
          while( ar_difftime( nextFrameTime, currentTime ) > 0.0 ) {
            // without actually returning from this function all windows are kept
            // to the window with the lowest Hz as the wm will have to wait to
            // add events to the 'slow' window until it is done sleeping in
            // this function, by returning we give the wm a chance to add the
            // event to this 'slow' window and move on to 'faster' windows (the
            // wm will wait if either its in singlethreaded mode or the swaps
            // are blocking {both fairly 'normal' modes of operation})
            ar_mutex_unlock( &_WMEventsMutex );
            return 0;
            // ar_usleep( 0 );
            // currentTime = ar_time();
          }
        }

        _drawHandler();

        _lastFrameTime = ar_time();
      break;

      case AR_WINDOW_SWAP:
        if( swap() < 0 ) {
          std::cerr << "_processWMEvents: swap error" << std::endl;
        }
      break;

      case AR_WINDOW_MOVE:
        if( move( wmEvent->getEvent().getPosX(), wmEvent->getEvent().getPosY() ) < 0 ) {
          std::cerr << "_processWMEvents: move error" << std::endl;
        }
      break;

      case AR_WINDOW_RESIZE:
        if( resize( wmEvent->getEvent().getSizeX(), wmEvent->getEvent().getSizeY() ) < 0 ) {
          std::cerr << "_processWMEvents: resize error" << std::endl;
        }
      break;

      case AR_WINDOW_VIEWPORT:
        if( setViewport( wmEvent->getEvent().getPosX(), wmEvent->getEvent().getPosY(),
                         wmEvent->getEvent().getSizeX(), wmEvent->getEvent().getSizeY() ) < 0 ) {
          std::cerr << "_processWMEvents: setViewport error" << std::endl;
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
        if( _running ) {
          if( _killWindow() < 0 ) {
            // print error?
          }
        }
      break;

      default:
        // print warning?
      break;
    }

    // signal anyone waiting on this event that it has finally been processed
    wmEvent->signal();

    _WMEvents.pop();
  }

  ar_mutex_unlock( &_WMEventsMutex );

  return 0;
}

int arGUIWindow::_performWindowCreation( void )
{
  if( _setupWindowCreation() < 0 ) {
    std::cerr << "arGUIWindow: _setupWindowCreation failed" << std::endl;
    return -1;
  }

  if( _windowCreation() < 0 ) {
    std::cerr << "arGUIWindow: _windowCreation failed" << std::endl;
    return -1;
  }

  if( _tearDownWindowCreation() < 0 ) {
    std::cerr << "arGUIWindow: _tearDownWindowCreation failed" << std::endl;
    return -1;
  }

  // taken care of by the windowInitGL callback now
  // InitGL( getWidth(), getHeight() );

  // tell anyone listening that the window has been successfully created
  // and initialized
  ar_mutex_lock( &_creationMutex );
  _creationFlag = true;
  _creationCond.signal();
  ar_mutex_unlock( &_creationMutex );

  return 0;
}

int arGUIWindow::_windowCreation( void )
{
  #if defined( AR_USE_WIN_32 )

  int dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  if( _windowConfig.getStereo() ) {
    dwFlags |= PFD_STEREO;
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

  /*
  if( _windowConfig._fullscreen && ( _changeScreenResolution() < 0 ) ) {
    std::cerr << "_windowCreation: ChangeScreenResolution failed...running in windowed mode" << std::endl;
    _windowConfig._fullscreen = false;
  }
  */

  DWORD windowExtendedStyle = WS_EX_APPWINDOW;
  DWORD windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;

  if( _windowConfig.getDecorate() ) {
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

  /*
  if( _windowConfig._topmost ) {
    windowExtendedStyle |= WS_EX_TOPMOST;
  }
  */

  RECT windowRect = { trueX, trueY, trueX + trueWidth, trueY + trueHeight };

  // adjust window to true requested size since Win32 treats the width and
  // height as the whole window (including decorations) but we want it to be
  // just the client area
  if( !AdjustWindowRectEx( &windowRect, windowStyle, 0, windowExtendedStyle ) ) {
    std::cerr << "_windowCreation: AdjustWindowRectEx error" << std::endl;
  }

  // create the OpenGL window
  // NOTE: the last argument needs to be a pointer to this object so that the
  // static event processing function in GUIEventManager can access the window
  if( !( _windowHandle._hWnd = CreateWindowEx( windowExtendedStyle,
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
    std::cerr << "_windowCreation: CreateWindowEx failed" << std::endl;
    _killWindow();
    return -1;
  }

  if( !( _windowHandle._hDC = GetDC( _windowHandle._hWnd ) ) ) {
    std::cerr << "_windowCreation: GetDC failed" << std::endl;
    _killWindow();
    return -1;
  }

  const GLuint PixelFormat = ChoosePixelFormat( _windowHandle._hDC, &pfd );
  if(!PixelFormat) {
    std::cerr << "_windowCreation: ChoosePixelFormat failed" << std::endl;
    _killWindow();
    return -1;
  }

  if( !SetPixelFormat( _windowHandle._hDC, PixelFormat, &pfd ) ) {
    std::cerr << "_windowCreation: SetPixelFormat failed" << std::endl;
    _killWindow();
    return -1;
  }

  // should check if setting up stereo succeeded with DescribePixelFormat

  if( !( _windowHandle._hRC = wglCreateContext( _windowHandle._hDC ) ) ) {
    std::cerr << "_windowCreation: wglCreateContext failed" << std::endl;
    _killWindow();
    return -1;
  }

  if( !wglMakeCurrent( _windowHandle._hDC, _windowHandle._hRC ) ) {
    std::cerr << "_windowCreation: wglMakeCurrent failed" << std::endl;
    _killWindow();
    return -1;
  }

  _running = true;

  if( _windowConfig.getFullscreen() ) {
    fullscreen();
  }

  ShowWindow( _windowHandle._hWnd, SW_SHOW );

  if( !SetForegroundWindow( _windowHandle._hWnd ) ) {
    std::cerr << "_windowCreation: SetForegroundWindow failure" << std::endl;
  }

  if( !SetFocus( _windowHandle._hWnd ) ) {
    std::cerr << "_windowCreation: SetFocus failure" << std::endl;
  }

  // seems to be somewhat superfluous since there are SetForegroundWindow
  // and SetFocus calls above
  if( !_windowConfig.getFullscreen() ) {
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

  _windowHandle._dpy = NULL;
  _windowHandle._vi = NULL;
  _windowHandle._wHints = None;

  if( _windowConfig.getStereo() ) {
    attrList[ 10 ] = GLX_STEREO;
    attrList[ 11 ] = None;
  }

  // if the XDisplay window config parameter has not been set, use the $DISPLAY env variable.
  _windowHandle._dpy = XOpenDisplay( _windowConfig.getXDisplay().length() ? _windowConfig.getXDisplay().c_str() : NULL );

  if( !_windowHandle._dpy ) {
    std::cerr << "_windowCreation: XOpenDisplay failure on: " << _windowConfig.getXDisplay() << std::endl;
    return -1;
  }

  _windowHandle._screen = DefaultScreen( _windowHandle._dpy );

  if( !glXQueryExtension( _windowHandle._dpy, NULL, NULL ) ) {
    std::cerr << "_windowCreation: OpenGL GLX extensions not supported" << std::endl;
    return -1;
  }

  if( !( _windowHandle._vi = glXChooseVisual( _windowHandle._dpy, _windowHandle._screen, attrList ) ) ) {
    std::cerr << "_windowCreation: could not create double-buffered window" << std::endl;
    // _killWindow();
    return -1;
  }

  _windowHandle._root = RootWindow( _windowHandle._dpy, _windowHandle._vi->screen );

  if( !( _windowHandle._ctx = glXCreateContext( _windowHandle._dpy, _windowHandle._vi, NULL, GL_TRUE ) ) ) {
    std::cerr << "_windowCreation: could not create rendering context" << std::endl;
    // _killWindow();
    return -1;
  }

  _windowHandle._attr.colormap = XCreateColormap( _windowHandle._dpy, _windowHandle._root,
                                                  _windowHandle._vi->visual, AllocNone );
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
  if( 0 /* _windowConfig._fullscreen */ ) {
    /*
    int numExt;
    char** extensions = XListExtensions( _windowHandle._dpy, &numExt );

    for( int i = 0; i < numExt; i++ ) {
      std::cout << extensions[ i ] << std::endl;
    }

    XFree( modes );
    */

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
      if( ( modes[ i ]->hdisplay == _windowConfig.getWidth() ) &&
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

    if( x || y ) {
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
  if( _windowConfig._fullscreen ) {
    XGrabKeyboard( _windowHandle._dpy, _windowHandle._win, True, GrabModeAsync, GrabModeAsync, CurrentTime );
    XGrabPointer( _windowHandle._dpy, _windowHandle._win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync,
                  _windowHandle._win, None, CurrentTime );
  }
  */

  if( _windowConfig.getFullscreen() ) {
    raise( _windowConfig.getZOrder() );
  }

  decorate( _windowConfig.getDecorate() );

  if( _windowConfig.getFullscreen() ) {
    fullscreen();
  }

  XSync( _windowHandle._dpy, False );

  XMapWindow( _windowHandle._dpy, _windowHandle._win );

  glXMakeCurrent( _windowHandle._dpy, _windowHandle._win, _windowHandle._ctx );

  if( !glXIsDirect( _windowHandle._dpy, _windowHandle._ctx ) ) {
    std::cerr << "No hardware acceleration available." << std::endl;
  }

  #endif

  // this should get properly set in arGUIEventManager, probably shouldn't
  // set it here
  _visible = true;

  setCursor( _windowConfig.getCursor() );

  return 0;
}

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
  // NOTE: classname's *must* be unique to each window, if two windows are
  // created with the same title, it will cause problems, _className mitigates
  // this issue
  windowClass.lpszClassName = _className.c_str();

  if( !RegisterClassEx( &windowClass ) ) {
    std::cerr << "_setupWindowCreation: RegisterClassEx Failed" << std::endl;
    return -1;
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #endif

  return 0;
}

int arGUIWindow::_tearDownWindowCreation( void )
{
  // now that the window is created, perform any necessary user-defined opengl
  // initialization.
  if( _windowInitGLCallback ) {
    arGUIWindowInfo* windowInfo = new arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_INITGL, _ID );
    windowInfo->setUserData( _userData );
    _windowInitGLCallback( windowInfo );
    delete windowInfo;
  }

  #if defined( AR_USE_WIN_32 )

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #endif

  return 0;
}

int arGUIWindow::swap( void )
{
  if( !_running ) {
    return -1;
  }

  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "swap: could not make context current" << std::endl;
  }

  return _windowBuffer->swapBuffer( _windowHandle, _windowConfig.getStereo() );
}

int arGUIWindow::resize( int newWidth, int newHeight )
{
  if( !_running ) {
    return -1;
  }

  // check arguments for sanity?

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "resize: could not make context current" << std::endl;
  }
  */

  // avoid divide-by-0
  if( newHeight == 0 ) {
    newHeight = 1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "resize: GetWindowRect error" << std::endl;
  }

  if( _decorate )
  {
    newWidth  += GetSystemMetrics( SM_CXSIZEFRAME ) * 2;
    newHeight += GetSystemMetrics( SM_CYSIZEFRAME ) * 2 +
                 GetSystemMetrics( SM_CYCAPTION );
  }

  if( !SetWindowPos( _windowHandle._hWnd, HWND_TOP,
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
  if( _fullscreen ) {
    _fullscreen = false;

    // don't need to do this under win32 as the window didn't get undecorated
    // to go fullscreen
    #if defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
    if( _windowConfig.getDecorate() ) {
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

int arGUIWindow::fullscreen( void )
{
  if( !_running || _fullscreen ) {
    return -1;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "fullscreen: could not make context current" << std::endl;
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

  // raise a resize event so the user can adjust the viewport if necessary
  // NOTE: seems to only be necessary on windows as the event is 'correctly'
  // raised on linux (odder still is that the move to 0x0 is raised, just not
  // the resize...)
  // NOTE: DO NOT query the window for it's believed x,y,width,height here.
  // These haven't yet been changed with a resize event.
  // That is a mistake on ever-finicky Win32. Instead, go ahead and feed
  // it (again) the information we computed before. This is needed so that
  // the viewport is resized appropriately at the framework level.
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

  if( x || y ) {
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

  #endif

  _fullscreen = true;

  return 0;
}

int arGUIWindow::setViewport( int newX, int newY, int newWidth, int newHeight )
{
  if( !_running ) {
    return -1;
  }

  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "setviewport: could not make context current" << std::endl;
  }

  glViewport( newX, newY, newWidth, newHeight );
  return 0;
}

void arGUIWindow::decorate( const bool decorate )
{
  if( !_running || _fullscreen ) {
    return;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "decorateWindow: could not make context current" << std::endl;
  }
  */

  #if defined( AR_USE_WIN_32 )

  DWORD windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
  DWORD windowExtendedStyle = WS_EX_APPWINDOW;

  if( !decorate ) {
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

  if( !decorate && _windowHandle._wHints == None ) {
    // try Motif hints
    if( XInternAtom( _windowHandle._dpy, "_MOTIF_WM_HINTS", False ) != None ) {
      _windowHandle._wHints = XInternAtom( _windowHandle._dpy, "_MOTIF_WM_HINTS", False );

      MotifWmHints hints = { (1L << 1), 0, 0, 0, 0 };
      XChangeProperty( _windowHandle._dpy, _windowHandle._win,
                       _windowHandle._wHints, _windowHandle._wHints, 32, PropModeReplace,
                       (unsigned char *) &hints, sizeof( hints ) / sizeof( long ) );
    }
    else
    // try KDE hints
    if( XInternAtom( _windowHandle._dpy, "KWM_WIN_DECORATION", False ) != None ) {
      _windowHandle._wHints = XInternAtom( _windowHandle._dpy, "KWM_WIN_DECORATION", False );

      long hints = 0;
      XChangeProperty( _windowHandle._dpy, _windowHandle._win,
                       _windowHandle._wHints, _windowHandle._wHints, 32, PropModeReplace,
                       (unsigned char *) &hints, sizeof( hints ) / sizeof( long ) );
    }
    else
    // try GNOME hints
    if( XInternAtom( _windowHandle._dpy, "_WIN_HINTS", False ) != None ) {
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
  else if( decorate && _windowHandle._wHints != None ) {
    XDeleteProperty( _windowHandle._dpy, _windowHandle._win, _windowHandle._wHints );
    _windowHandle._wHints = None;

    set = true;
  }

  // Under Xandros 2.0 (a tweaked version of KDE 3.1.4) the window has to be
  // remapped in order for decoration changes to take.  Unfortunately, this
  // causes OSX to freak out (the client area of the window blanks to white),
  // but if the window is /not/ remapped the changes don't take under OSX
  // either, so for now windows on OSX will not be able to be redecorated
  // (which has implications for going in and out of fullscreen mode as well).
  // And, of course, Slackware 10.1 (KDE 3.4.0) doesn't need any remapping at
  // all for this stuff to work...
  if( set ) {
    #if !defined( AR_USE_DARWIN )
    // While Xandros needs these calls to be able to redecorate a window during
    // runtime, they also seem to preclude fullscreen mode working properly
    // (since they window must be un-decorated before going fullscreen) in that
    // the fullscreen window does not cover the taskbar.  As above, under
    // Slackware this is not a problem.  Since working fullscreen is more
    // important than re-decoration they are commented out for the time being

    // XUnmapWindow( _windowHandle._dpy, _windowHandle._win );
    // XMapWindow( _windowHandle._dpy, _windowHandle._win );
    #endif
  }

  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

  #endif

  _decorate = decorate;
}

void arGUIWindow::raise( arZOrder zorder )
{
  if( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "raise: could not make context current" << std::endl;
  }
  */

  #if defined( AR_USE_WIN_32 )

  HWND flag;

  if( zorder == AR_ZORDER_NORMAL ) {
    flag = HWND_NOTOPMOST;
  }
  else if( zorder == AR_ZORDER_TOP ) {
    flag = HWND_TOP;
  }
  else if( zorder == AR_ZORDER_TOPMOST ) {
    flag = HWND_TOPMOST;
  }

  SetWindowPos( _windowHandle._hWnd, flag,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE );

  _zorder = zorder;

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );
  XRaiseWindow( _windowHandle._dpy, _windowHandle._win );
  XFlush( _windowHandle._dpy );
  XUnlockDisplay( _windowHandle._dpy );

  #endif

  _visible = true;
}

void arGUIWindow::lower( void )
{
  if( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "lowerWindow: could not make context current" << std::endl;
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

void arGUIWindow::minimize( void )
{
  if( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "minimizeWindow: could not make context current" << std::endl;
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

void arGUIWindow::restore( void )
{
  if( !_running ) {
    return;
  }

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "restoreWindow: could not make context current" << std::endl;
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

int arGUIWindow::move( int newX, int newY )
{
  if( !_running || _fullscreen ) {
    return -1;
  }

  // check arguments for sanity?

  /*
  // is this necessary in this function?
  if( !_threaded && ( makeCurrent( false ) < 0 ) ) {
    std::cerr << "move: could not make context current" << std::endl;
  }
  */

#if defined( AR_USE_WIN_32 )

  /*
  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "move: GetWindowRect error" << std::endl;
    return -1;
  }

  if( !MoveWindow( _windowHandle._hWnd, newX, newY,
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

arCursor arGUIWindow::setCursor( arCursor cursor )
{
  if( !_running ) {
    cout << "arGUIWindow remark: ignoring setCursor while not running.\n";
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
      // print error?
      cout << "arGUIWindow remark: given an invalid cursor type.\n";
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
        std::cerr << "Could not create AR_CURSOR_NONE" << std::endl;
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

  if( XCursor == None ) {
    std::cerr << "arGUIWindow error: failed to create requested X cursor.\n";
    return _cursor;
  }

  _cursor = cursor;
  XDefineCursor( _windowHandle._dpy, _windowHandle._win, XCursor );

#endif

  return _cursor;
}

void arGUIWindow::setTitle( const std::string& title )
{
  if( !_running ) {
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

int arGUIWindow::getWidth( void ) const
{
  if( !_running ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "getWidth: GetWindowRect error" << std::endl;
    return -1;
  }

  if( _decorate ) {
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

int arGUIWindow::getHeight( void ) const
{
  if( !_running ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "getHeight: GetWindowRect error" << std::endl;
    return -1;
  }

  if( _decorate ) {
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

int arGUIWindow::getPosX( void ) const
{
  if( !_running ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;
  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "getPosX: GetWindowRect error" << std::endl;
    return -1;
  }

  if( 0 /*_windowConfig._decorate*/ ) {
    rect.left += GetSystemMetrics( SM_CXSIZEFRAME );
  }
  return rect.left;

  #elif defined( AR_USE_LINUX )  || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  int x, y, bx, by;
  Window w;

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if( _windowConfig.getDecorate() ) {
    XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win,
                           w, 0, 0, &bx, &by, &w );
    x -= bx;
  }

  XUnlockDisplay( _windowHandle._dpy );
  return x;

  #endif
}

int arGUIWindow::getPosY( void ) const
{
  if( !_running ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "getPosY: GetWindowRect error" << std::endl;
    return -1;
  }

  if( 0 /*_windowConfig._decorate*/ ) {
    rect.top += GetSystemMetrics( SM_CYSIZEFRAME ) + GetSystemMetrics( SM_CYCAPTION );
  }

  return rect.top;

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  int x, y, bx, by;
  Window w;

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if( _windowConfig.getDecorate() ) {
    XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win,
                           w, 0, 0, &bx, &by, &w );
    y -= by;
  }

  XUnlockDisplay( _windowHandle._dpy );
  return y;

  #endif
}

int arGUIWindow::_changeScreenResolution( void )
{
  if( !_running ) {
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

  if( ChangeDisplaySettings( &dmScreenSettings, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL ) {
    std::cerr << "_changeScreenResolution: ChangeDisplaySettinges failed" << std::endl;
    return -1;
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #endif

  return 0;
}

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
  if( _fullscreen ) {
    /*
    if( ChangeDisplaySettings( NULL, 0 ) != DISP_CHANGE_SUCCESSFUL ) {
      std::cerr << "_killWindow: ChangeDisplaySettings failure" << std::endl;
      return -1;
    }
    */

    ShowCursor( true );
  }

  if( _windowHandle._hRC ){
    if( !wglMakeCurrent( NULL, NULL ) ) {
      std::cerr << "_killWindow: release of DC and RC failed" << std::endl;
    }

    if( !wglDeleteContext( _windowHandle._hRC ) ) {
      std::cerr << "_killWindow: delete RC failed" << std::endl;
    }

    _windowHandle._hRC = NULL;
  }

  if( _windowHandle._hDC && !ReleaseDC( _windowHandle._hWnd, _windowHandle._hDC ) ) {
    std::cerr << "_killWindow: release DC failed" << std::endl;
    _windowHandle._hDC = NULL;
  }

  if( _windowHandle._hWnd && !DestroyWindow( _windowHandle._hWnd ) ) {
    std::cerr << "_killWindow: could not release hWnd" << std::endl;
    _windowHandle._hWnd = NULL;
  }

  if( !UnregisterClass( _className.c_str(), _windowHandle._hInstance ) ) {
    std::cerr << "_killWindow: could not unregister class" << std::endl;
    _windowHandle._hInstance = NULL;
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  if( _windowHandle._ctx ) {
    if( !glXMakeCurrent( _windowHandle._dpy, None, NULL ) ) {
      std::cerr << "_killWindow: Error releasing drawing context" << std::endl;
    }

    glXDestroyContext( _windowHandle._dpy, _windowHandle._ctx );

    _windowHandle._ctx = NULL;
  }

  // switch back to the desktop
  if( _fullscreen ) {
    #ifdef HAVE_X11_EXTENSIONS
      XF86VidModeSwitchToMode( _windowHandle._dpy, _windowHandle._screen, &_windowHandle._dMode );
      XF86VidModeSetViewPort( _windowHandle._dpy, _windowHandle._screen, 0, 0 );
    #endif
  }

  if( XCloseDisplay( _windowHandle._dpy ) == BadGC ) {
    std::cerr << "_killWindow: Error closing display" << std::endl;
  }

  #endif

  _GUIEventManager->setActive( false );

  _running = false;

  // tell anybody who's listening that the window thread is exiting
  ar_mutex_lock( &_destructionMutex );
  _destructionFlag = true;
  _destructionCond.signal();
  ar_mutex_unlock( &_destructionMutex );

  return 0;
}

int arGUIWindow::makeCurrent( bool release )
{
  if( !_running ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  if( release ) {
    if( !wglMakeCurrent( NULL, NULL ) ) {
      return -1;
    }
  }
  else if( ( wglGetCurrentContext() == _windowHandle._hRC ) &&
           ( wglGetCurrentDC() == _windowHandle._hDC ) ) {
    // already current (do this check so we aren't constantly switching
    // contexts and getting hit by the flushes that that incurs)
    return 0;
  }
  if( !wglMakeCurrent( _windowHandle._hDC, _windowHandle._hRC ) ) {
    return -1;
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XLockDisplay( _windowHandle._dpy );

  if( release ) {
    if( !glXMakeCurrent( _windowHandle._dpy, None, NULL ) ) {
      XUnlockDisplay( _windowHandle._dpy );
      return -1;
    }
  }
  else if( ( glXGetCurrentContext() == _windowHandle._ctx ) &&
           ( glXGetCurrentDisplay() == _windowHandle._dpy ) ) {
    // already current (do this check so we aren't constantly switching
    // contexts and getting hit by the flushes that that incurs)
    XUnlockDisplay( _windowHandle._dpy );
    return 0;
  }
  if( !glXMakeCurrent( _windowHandle._dpy, _windowHandle._win, _windowHandle._ctx ) ) {
    XUnlockDisplay( _windowHandle._dpy );
    return -1;
  }

  XUnlockDisplay( _windowHandle._dpy );

  #endif

  return 0;
}

void arGUIWindow::useWildcatFramelock( bool isOn )
{
  ar_useWildcatFramelock( isOn );
}

void arGUIWindow::findWildcatFramelock( void )
{
  ar_findWildcatFramelock();
}

void arGUIWindow::activateWildcatFramelock( void )
{
  ar_activateWildcatFramelock();
}

void arGUIWindow::deactivateWildcatFramelock( void )
{
  ar_deactivateWildcatFramelock();
}
