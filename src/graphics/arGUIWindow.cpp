/**
 * @file arGUIWindow.cpp
 * Implementation of the arGUIWindow class.
 */
#include "arPrecompiled.h"

#if defined( AR_USE_WIN_32 )

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  // #include <Xm/MwmUtil.h>
#endif

#include <iostream>

#include "arStructuredData.h"
#include "arDataUtilities.h"

#include "arGUIEventManager.h"
#include "arGUIInfo.h"
#include "arGUIWindow.h"

// convenience define
#define USEC 1000000.0

arGUIWindowConfig::arGUIWindowConfig( int x, int y, int width, int height, int bpp, int hz,
                                      bool decorate, bool topmost,
                                      bool fullscreen, bool stereo,
                                      const std::string& title, const std::string& XDisplay ) :
  _x( x ),
  _y( y ),
  _width( width ),
  _height( height ),
  _bpp( bpp ),
  _Hz( hz ),
  _decorate( decorate ),
  _topmost( topmost ),
  _fullscreen( fullscreen ),
  _stereo( stereo ),
  _title( title ),
  _XDisplay( XDisplay )
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
  _done++;
  ar_mutex_unlock( &_doneMutex );
}

void arWMEvent::signal( void )
{
  ar_mutex_lock( &_eventMutex );
  _conditionFlag = true;
  _eventCond.signal();
  ar_mutex_unlock( &_eventMutex );

  ar_mutex_lock( &_doneMutex );
  _done++;
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
  if( !_dblBuf ) {
    return -1;
  }

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

arGUIWindow::arGUIWindow( int ID, arGUIWindowConfig windowConfig, void (*drawCallback)( arGUIWindowInfo* ) ) :
  _drawCallback( drawCallback ),
  _ID( ID ),
  _windowConfig( windowConfig ),
  _visible( false ),
  _running( false ),
  _threaded( false ),
  _fullscreen( false ),
  _creationFlag( false ),
  _destructionFlag( false )
{
  _windowBuffer = new arGUIWindowBuffer( true );

  _GUIEventManager = new arGUIEventManager();

  _lastFrameTime = ar_time();

  ar_mutex_init( &_WMEventsMutex );
  ar_mutex_init( &_usableEventsMutex );
  ar_mutex_init( &_creationMutex );
  ar_mutex_init( &_destructionMutex );
}

arGUIWindow::~arGUIWindow( void )
{
  if( _running ) {
    if( _killWindow() < 0 ) {
      // print error?
    }
  }

  // delete both event queues (what if the wm is still holding onto a handle?)

  delete _GUIEventManager;

  delete _windowBuffer;
}

void arGUIWindow::registerDrawCallback( void (*drawCallback)( arGUIWindowInfo* ) )
{
  if( _drawCallback ) {
    // print warning that previous callback is being overwritten?
  }

  _drawCallback = drawCallback;
}

void arGUIWindow::_drawHandler( void )
{
  if( _running && _drawCallback ) {

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
    windowInfo->_windowID = _ID;

    _drawCallback( windowInfo );

    delete windowInfo;

    #if 0 // defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
    XUnlockDisplay( _windowHandle._dpy );
    #endif
  }
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

  // wait for the window to actually be created to avoid any potential
  // race conditions from window manager calls trying to operate on a
  // half-baked window.  if it takes longer than 5 seconds then declare
  // a failure and return
  ar_mutex_lock( &_creationMutex );
  while( !_creationFlag ) {
    if( !_creationCond.wait( &_creationMutex, 5000 ) ) {
      return -1;
    }
  }
  ar_mutex_unlock( &_creationMutex );

  return 0;
}

void arGUIWindow::mainLoop( void* data )
{
  arGUIWindow* This = (arGUIWindow*) data;

  if( This ) {
    This->_mainLoop();
  }
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

  // because of some threading changes, this is no longer necessary, the
  // window will now be killed in processarWMEvents() (i.e. running == false
  // iff _killWindow has /already/ been called)
  /*
  if( _killWindow() < 0 ) {
    // window was not cleaned up properly, print warning?
  }
  */
}

int arGUIWindow::_consumeWindowEvents( void )
{
  if( !_running ) {
    return -1;
  }

  if( _GUIEventManager->consumeEvents( this, false ) < 0 ) {
    return -1;
  }

  // ARGH!! the thread will just spin forever in this loop, even if there
  // are no GUI events or wm events.  This can cause a delay of around 8-10ms
  // between draw/swap messages as the window threads do the draw then just
  // spin in this loop not allowing the wm to send the next draw message.
  // need some way (besides sleeping) of allowing the window threads to
  // relinquish control when they really don't have anything to do, seems to
  // really only be a problem with multiple windows open (only seems to be a
  // a real problem on linux, but it aint snappy under windows either)
  if( _threaded && _WMEvents.empty() ) {
    // what we should probably do is wait on a condition variable the wm
    // sets at a routine interval (ala heartbeating).  however, this raises
    // its own issues, i.e. at what interval?  how does it keep this interval
    // if it's just spinning in its own loop (where its ostensibly sending us
    // draw messages and filling up _wmEvents anyhow)?  what if the user is
    // controlling the loop, do we require that they have to send a heartbeat?
    ar_usleep( 0 );
  }

  if( _processWMEvents() < 0 ) {
    return -1;
  }

  return 0;
}

arGUIInfo* arGUIWindow::getNextGUIEvent( void )
{
  if( !_running ) {
    return NULL;
  }

  return _GUIEventManager->getNextEvent();
}

bool arGUIWindow::eventsPending( void ) const
{
  if( !_running ) {
    return false;
  }

  return _GUIEventManager->eventsPending();
}

arWMEvent* arGUIWindow::addWMEvent( const arGUIWindowInfo& wmEvent )
{
  if( !_running ) {
    return NULL;
  }

  arWMEvent* event = NULL;

  EventIterator eitr;

  ar_mutex_lock( &_usableEventsMutex );

  // find an 'empty' event and recycle it
  for( eitr = _usableEvents.begin(); eitr != _usableEvents.end(); eitr++ ) {
    if( (*eitr)->_done == 2 ) {
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

    switch( wmEvent->_event._state ) {
      case AR_WINDOW_DRAW:

        // NOTE: for now we won't worry about _Hz, it complicates things quite
        // a bit as far as control flow in different modes is concerned and is
        // probably not that important a feature, revisit it when arGUI has
        // been successfully integrated into syzygy.
        if( 0 /* _windowConfig._Hz > 0 */ ) {
          ar_timeval currentTime = ar_time();

          // NOTE: assumes the user won't want less than 1 frame per second
          int uSec = int( USEC / double( _windowConfig._Hz ) );
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
        if( move( wmEvent->_event._posX, wmEvent->_event._posY ) < 0 ) {
          std::cerr << "_processWMEvents: move error" << std::endl;
        }
      break;

      case AR_WINDOW_RESIZE:
        if( resize( wmEvent->_event._sizeX, wmEvent->_event._sizeY ) < 0 ) {
          std::cerr << "_processWMEvents: resize error" << std::endl;
        }
      break;

      case AR_WINDOW_VIEWPORT:
        if( setViewport( wmEvent->_event._posX, wmEvent->_event._posY,
                         wmEvent->_event._sizeX, wmEvent->_event._sizeY ) < 0 ) {
          std::cerr << "_processWMEvents: setViewport error" << std::endl;
        }
      break;

      case AR_WINDOW_FULLSCREEN:
        fullscreen();
      break;

      case AR_WINDOW_DECORATE:
        decorate( wmEvent->_event._flag == 1 );
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

  InitGL( getWidth(), getHeight() );

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

  DWORD windowStyle;
  DWORD windowExtendedStyle;
  GLuint PixelFormat;
  int trueX, trueY, trueWidth, trueHeight;

  int dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

  if( _windowConfig._stereo ) {
    dwFlags |= PFD_STEREO;
  }

  PIXELFORMATDESCRIPTOR pfd = {
    sizeof( PIXELFORMATDESCRIPTOR ),
    1,                                  // version number
    dwFlags,                            // window creation flags
    PFD_TYPE_RGBA,                      // request an RGBA format
    _windowConfig._bpp,                 // select the color depth
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

  windowExtendedStyle = WS_EX_APPWINDOW;
  windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;

  if( _windowConfig._decorate ) {
    windowStyle |= WS_OVERLAPPEDWINDOW;
  }
  else {
    windowStyle |= WS_POPUP;
    windowExtendedStyle |= WS_EX_TOOLWINDOW;
  }

  trueX = _windowConfig._x;
  trueY = _windowConfig._y;
  trueWidth = _windowConfig._width;
  trueHeight = _windowConfig._height;

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
                                 _windowConfig._title.c_str(),
                                 _windowConfig._title.c_str(),
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

  if( !( PixelFormat = ChoosePixelFormat( _windowHandle._hDC, &pfd ) ) ) {
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

  if( _windowConfig._fullscreen ) {
    fullscreen();
  }

  ShowWindow( _windowHandle._hWnd, SW_SHOW );

  if( _windowConfig._topmost ) {
    raise();
  }

  if( !SetForegroundWindow( _windowHandle._hWnd ) ) {
    std::cerr << "_windowCreation: SetForegroundWindow failure" << std::endl;
  }

  if( !SetFocus( _windowHandle._hWnd ) ) {
    std::cerr << "_windowCreation: SetFocus failure" << std::endl;
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  int attrList[ 32 ] = { GLX_RGBA,
                         GLX_DOUBLEBUFFER,
                         GLX_RED_SIZE, 4,
                         GLX_GREEN_SIZE, 4,
                         GLX_BLUE_SIZE, 4,
                         GLX_DEPTH_SIZE, 16,
                         None };

  _windowHandle._dpy = NULL;
  _windowHandle._vi = NULL;
  _windowHandle._wHints = None;
  unsigned long mask = 0;
  XSizeHints sizeHints;
  XWMHints wmHints;
  XTextProperty textProperty;
  int trueX, trueY, trueWidth, trueHeight;

  if( _windowConfig._stereo ) {
    attrList[ 10 ] = GLX_STEREO;
    attrList[ 11 ] = None;
  }

  // if the XDisplay window config parameter has not been set, use the $DISPLAY env variable.
  _windowHandle._dpy = XOpenDisplay( _windowConfig._XDisplay.length() ? _windowConfig._XDisplay.c_str() : NULL );

  if( !_windowHandle._dpy ) {
    std::cout << "_windowCreation: XOpenDisplay failure on: " << _windowConfig._XDisplay << std::endl;
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

  mask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;

  // NOTE: for now we aren't going to even try to use extensions as they
  // aren't supported on every platform and the #else clause is just a massage
  // of fullscreen() anyhow
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

    XF86VidModeModeInfo **modes;
    int modeNum = 0, bestMode = 0;

    trueX = trueY = 0;

    XF86VidModeGetAllModeLines( _windowHandle._dpy, _windowHandle._screen, &modeNum, &modes );

    // check freeglut::glutGameMode for more about resetting the desktop to its original state
    _windowHandle._dMode = *modes[ 0 ];

    for( int i = 0; i < modeNum; i++) {
      if( ( modes[ i ]->hdisplay == _windowConfig._width ) &&
          ( modes[ i ]->vdisplay == _windowConfig._height ) ) {
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

    int x, y;
    Window w;

    trueWidth = DisplayWidth( _windowHandle._dpy, _windowHandle._vi->screen );
    trueHeight = DisplayHeight( _windowHandle._dpy, _windowHandle._vi->screen );

    trueX = trueY = 0;

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
    trueX = _windowConfig._x;
    trueY = _windowConfig._y;
    trueWidth = _windowConfig._width;
    trueHeight = _windowConfig._height;

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
  memset( &sizeHints, 0, sizeof( XSizeHints ) );
  sizeHints.flags  = USPosition | USSize;
  sizeHints.x      = trueX;
  sizeHints.y      = trueY;
  sizeHints.width  = trueWidth;
  sizeHints.height = trueHeight;
  sizeHints.base_width = trueWidth;
  sizeHints.base_height = trueHeight;

  wmHints.flags = StateHint;
  wmHints.initial_state = NormalState;

  const char* title = _windowConfig._title.c_str();

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

  if( _windowConfig._topmost ) {
    raise();
  }

  decorate( _windowConfig._decorate );

  if( _windowConfig._fullscreen ) {
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
  // created with the same title, it will cause problems, may want to include
  // some random element into the classname so this isn't possible
  windowClass.lpszClassName = _windowConfig._title.c_str();

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

  return _windowBuffer->swapBuffer( _windowHandle, _windowConfig._stereo );
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

  // prevent any divide-by-0 errors
  if( newHeight == 0 ) {
    newHeight = 1;
  }

  #if defined( AR_USE_WIN_32 )

  RECT rect;

  if( !GetWindowRect( _windowHandle._hWnd, &rect ) ) {
    std::cerr << "resize: GetWindowRect error" << std::endl;
  }

  if( _windowConfig._decorate )
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
    if( _windowConfig._decorate ) {
      decorate( true );
    }
    #endif

    // move the window back to its original position
    move( _windowConfig._x, _windowConfig._y );

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
    _GUIEventManager->addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RESIZE, _ID, 0, getPosX(), getPosY(), getWidth(), getHeight() ) );
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

  AdjustWindowRect( &windowRect,
                    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                    false );

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
  _GUIEventManager->addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RESIZE, _ID, 0, getPosX(), getPosY(), getWidth(), getHeight() ) );

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
  raise();

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

  // needed to flush the changes
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
    XUnmapWindow( _windowHandle._dpy, _windowHandle._win );
    XMapWindow( _windowHandle._dpy, _windowHandle._win );
    #endif
  }

  XFlush( _windowHandle._dpy );

  XUnlockDisplay( _windowHandle._dpy );

  #endif
}

void arGUIWindow::raise( void )
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

  SetWindowPos( _windowHandle._hWnd, HWND_TOP,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE );

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

  if( _windowConfig._decorate ) {
    rect.left  += GetSystemMetrics( SM_CXSIZEFRAME );
    rect.right -= GetSystemMetrics( SM_CXSIZEFRAME );
  }

  return rect.right - rect.left;

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XWindowAttributes winAttributes;

  XLockDisplay( _windowHandle._dpy );

  XGetWindowAttributes( _windowHandle._dpy, _windowHandle._win,
                        &winAttributes );

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

  if( _windowConfig._decorate ) {
    rect.top    += GetSystemMetrics( SM_CYSIZEFRAME ) + GetSystemMetrics( SM_CYCAPTION );
    rect.bottom -= GetSystemMetrics( SM_CYSIZEFRAME );
  }

  return rect.bottom - rect.top;

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XWindowAttributes winAttributes;

  XLockDisplay( _windowHandle._dpy );

  XGetWindowAttributes( _windowHandle._dpy, _windowHandle._win,
                        &winAttributes );

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

  int x, y, bx, by;
  Window w;

  XLockDisplay( _windowHandle._dpy );

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if( _windowConfig._decorate ) {
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

  int x, y, bx, by;
  Window w;

  XLockDisplay( _windowHandle._dpy );

  XTranslateCoordinates( _windowHandle._dpy, _windowHandle._win, _windowHandle._root,
                         0, 0, &x, &y, &w );

  if( _windowConfig._decorate ) {
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
  dmScreenSettings.dmPelsWidth  = _windowConfig._width;
  dmScreenSettings.dmPelsHeight = _windowConfig._height;
  dmScreenSettings.dmBitsPerPel = _windowConfig._bpp;
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
  if( _windowConfig._fullscreen ) {
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
      std::cerr << "_killWindow: release RC failed" << std::endl;
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

  if( !UnregisterClass( _windowConfig._title.c_str(), _windowHandle._hInstance ) ) {
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
  if( _windowConfig._fullscreen ) {
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
  else if( !wglMakeCurrent( _windowHandle._hDC, _windowHandle._hRC ) ) {
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
  else if( !glXMakeCurrent( _windowHandle._dpy, _windowHandle._win, _windowHandle._ctx ) ) {
    XUnlockDisplay( _windowHandle._dpy );
    return -1;
  }

  XUnlockDisplay( _windowHandle._dpy );

  #endif

  return 0;
}

