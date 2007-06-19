#include "arPrecompiled.h"

#include "arStructuredData.h"
#include "arMath.h"
#include "arDataUtilities.h"
#include "arLogStream.h"
#include "arGUIWindowManager.h"
#include "arGUIEventManager.h"
#include "arGUIWindow.h"
#include "arGUIXMLParser.h"
#include "arWildcatUtilities.h"

// Default callbacks for window and keyboard events.

void ar_windowManagerDefaultKeyboardFunction( arGUIKeyInfo* ki){
  if (!ki)
    return;
  if ( ki->getState() != AR_KEY_DOWN )
    return;
  if (ki->getKey() == AR_VK_ESC) {
    arGUIWindowManager* wm = ki->getWindowManager();
    if (wm)
      wm->deleteWindow(ki->getWindowID());
  }
}

void ar_windowManagerDefaultWindowFunction( arGUIWindowInfo* wi ){
  if (!wi)
    return;
  arGUIWindowManager* wm = wi->getWindowManager();
  if (!wm)
    return;

  switch( wi->getState() ){
  case AR_WINDOW_RESIZE:
    wm->setWindowViewport(wi->getWindowID(), 0, 0, wi->getSizeX(), wi->getSizeY());
    break;
  case AR_WINDOW_CLOSE:
    wm->deleteWindow(wi->getWindowID());
    break;
  default:
    // avoid compiler warning
    break;
  }
}

arGUIWindowManager::arGUIWindowManager( void (*windowCB)( arGUIWindowInfo* ) ,
                                        void (*keyboardCB)( arGUIKeyInfo* ),
                                        void (*mouseCB)( arGUIMouseInfo* ),
                                        void (*windowInitGLCB)( arGUIWindowInfo* ),
                                        bool threaded ) :
  _keyboardCallback( keyboardCB ? keyboardCB : ar_windowManagerDefaultKeyboardFunction),
  _mouseCallback( mouseCB ),
  _windowCallback( windowCB ? windowCB : ar_windowManagerDefaultWindowFunction),
  _windowInitGLCallback( windowInitGLCB ),
  _maxID( 0 ),
  _threaded( threaded )
{
#if defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  // seems to be necessary on OS X, not necessarily under linux, but probably
  // doesn't hurt to just always enable it though
  if( XInitThreads() == 0 ) {
    ar_log_error() << "arGUIWindowManager failed to init Xlib multi-threading.\n";
  }
#endif
}

arGUIWindowManager::~arGUIWindowManager( void )
{
  for( WindowIterator witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    delete witr->second;
  }
}

void arGUIWindowManager::_keyboardHandler( arGUIKeyInfo* keyInfo )
{
  if( _keyboardCallback ) {
    _keyboardCallback( keyInfo );
  }
}

void arGUIWindowManager::_mouseHandler( arGUIMouseInfo* mouseInfo )
{
  if( _mouseCallback ) {
    _mouseCallback( mouseInfo );
  }
}

void arGUIWindowManager::_windowHandler( arGUIWindowInfo* windowInfo )
{
  if( _windowCallback ) {
    _windowCallback( windowInfo );
  }
}

void arGUIWindowManager::registerWindowCallback( void (*windowCallback) ( arGUIWindowInfo* ) )
{
  if( _windowCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new window callback.\n";
  }
  _windowCallback = windowCallback;
}

void arGUIWindowManager::registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) )
{
  if( _keyboardCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new keyboard callback.\n";
  }
  _keyboardCallback = keyboardCallback;
}

void arGUIWindowManager::registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) )
{
  if( _mouseCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new mouse callback.\n";
  }
  _mouseCallback = mouseCallback;
}

void arGUIWindowManager::registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) )
{
  if( _windowInitGLCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new window init GL callback.\n";
  }
  _windowInitGLCallback = windowInitGLCallback;
}

// todo: merge these two.

int arGUIWindowManager::startWithSwap( void )
{
  while( true ) {
    drawAllWindows( false );
    swapAllWindowBuffers( true );
    processWindowEvents();
  }
  return 0;
}

int arGUIWindowManager::startWithoutSwap( void )
{
  while( true ) {
    drawAllWindows( false );
    processWindowEvents();
  }
  return 0;
}

int arGUIWindowManager::addWindow( const arGUIWindowConfig& windowConfig,
                                   bool useWindowing )
{
  cerr << "addWindow().\n";
  arGUIWindow* window = new arGUIWindow( _maxID, windowConfig,
                                         _windowInitGLCallback, _userData );
  // Tell the window who owns it... so events will include this info.
  // Let event processing callbacks refer to the window manager.
  window->setWindowManager( this );

  _windows[ _maxID ] = window;

  // Make the OS window, if it was requested.
  if (useWindowing){
    if( _threaded ) {
      // This should only return once the window is actually up and running
      if( window->beginEventThread() < 0 ) {
        // Already printed warning.
        delete window;
        _windows.erase( _maxID );
        return -1;
      }
    }
    else {
      if( window->_performWindowCreation() < 0 ) {
        // Already printed warning.
        delete window;
        _windows.erase( _maxID );
        return -1;
      }
    }
  }

  return _maxID++;
}

int arGUIWindowManager::registerDrawCallback( const int ID, arGUIRenderCallback* drawCallback )
{
  if (!windowExists(ID))
    return -1;

  _windows[ ID ]->registerDrawCallback( drawCallback );
  return 0;
}

int arGUIWindowManager::processWindowEvents( void )
{
  // no registered callbacks, user should be handling events with getNextEvent
  // (what about window close events?)
  if( !_keyboardCallback && !_mouseCallback && !_windowCallback ) {
    // could be subclassed, in which case this is actually ok
    // return -1;
  }

  // if the WM is in single-threaded mode, first it needs to tell the windows
  // to push any pending gui events onto their stack since they are not doing
  // this in their own thread
  if( !_threaded && ( consumeAllWindowEvents() < 0 ) ) {
    ar_log_warning() << "arGUIWindowManager processWindowEvents consumeAllWindowEvents problem.\n";
  }

  for( WindowIterator it = _windows.begin(); it != _windows.end(); ++it ) {
    arGUIWindow* currentWindow = it->second;

    while( currentWindow->eventsPending() ) {
      arGUIInfo* GUIInfo = currentWindow->getNextGUIEvent();

      if( !GUIInfo ) {
        // print a warning? this meant arGUIEventManager threw a NULL event
        // into the window's event queue
        continue;
      }

      // NOTE: the user _CANNOT_ make any opengl calls in the callbacks below,
      // as this thread does not have any opengl contexts associated with it
      // (or in single-threaded mode the context of the last window added),
      // they must either use some recorded state in the drawcallback, or use
      // the provided accessors off the window manager
      switch( GUIInfo->getEventType() ) {
        case AR_KEY_EVENT:
          if( _keyboardCallback ) {
            _keyboardHandler( (arGUIKeyInfo*) GUIInfo );
          }
        break;

        case AR_MOUSE_EVENT:
          if( _mouseCallback ) {
            _mouseHandler( (arGUIMouseInfo*) GUIInfo );
          }
        break;

        case AR_WINDOW_EVENT:
          if( _windowCallback ) {
            _windowHandler( (arGUIWindowInfo*) GUIInfo );
          }
          else {
           // take whatever measures are necessary for minimal functionality
           // (i.e. if the user never registered a callback, at least handle
           // the close event)
          }
        break;

        default:
          ar_log_warning() << "arGUIWindowManager warning: processWindowEvents: Unknown Event Type" << ar_endl;
        break;
      }

      delete GUIInfo;
    }
  }

  return 0;
}

arGUIInfo* arGUIWindowManager::getNextWindowEvent( const int ID )
{
  return windowExists(ID) ?  _windows[ ID ]->getNextGUIEvent() : NULL;
}

arWMEvent* arGUIWindowManager::addWMEvent( const int ID, arGUIWindowInfo e )
{
  if (!windowExists(ID))
    return NULL;

  arWMEvent* eventHandle = NULL;

  // Some functions need the correct windowID set.
  e.setWindowID( ID );
  arGUIWindow* w = _windows[ID];

  if (_threaded) {
    eventHandle = w->addWMEvent( e );
  }
  else {
    // Don't pass a message. Just do the command.
    switch( e.getState() ) {
      case AR_WINDOW_SWAP:
        w->swap();
      break;

      case AR_WINDOW_DRAW:
        w->_drawHandler();
      break;

      case AR_WINDOW_RESIZE:
        w->resize( e.getSizeX(), e.getSizeY() );
      break;

      case AR_WINDOW_MOVE:
        w->move( e.getPosX(), e.getPosY() );
      break;

      case AR_WINDOW_VIEWPORT:
        w->setViewport( e.getPosX(), e.getPosY(), e.getSizeX(), e.getSizeY() );
      break;

      case AR_WINDOW_FULLSCREEN:
        w->fullscreen();
      break;

      case AR_WINDOW_DECORATE:
        w->decorate( (e.getFlag() == 1) );
      break;

      case AR_WINDOW_RAISE:
        w->raise( arZOrder( e.getFlag() ) );
      break;

      case AR_WINDOW_CURSOR:
        w->setCursor( arCursor( e.getFlag() ) );
      break;

      case AR_WINDOW_CLOSE:
        // bug: fails sometimes in OS X.
#ifndef AR_USE_DARWIN
        w->_killWindow();
#endif
      break;

      default:
        // warn?
      break;
    }
  }

  // Do not wait on the eventHandle.  The caller decides if this request blocks.
  return eventHandle;
}

int arGUIWindowManager::addAllWMEvent( arGUIWindowInfo wmEvent, bool blocking ){
  static bool warn = false;
  if( !warn && blocking && !_threaded ) {
    // Bugs in a syzygy framework might cause a deadlock here.
    ar_log_debug() << "arGUIWindowManager: addAllWMEvent blocking while singlethreaded.\n";
    warn = true;
  }

  // Pass the event to all windows so they can get started on it
  WindowIterator witr;
  EventVector eventHandles;
  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    arWMEvent* eventHandle = addWMEvent( witr->second->getID(), wmEvent );
    if( eventHandle ) {
      eventHandles.push_back( eventHandle );
    }
    else if( _threaded ) {
      // If !_threaded, addWMEvent returns NULL.  Warn?
    }
  }

  // Wait for the events to complete.
  EventIterator eitr;
  for( eitr = eventHandles.begin(); eitr != eventHandles.end(); eitr++ )
    (*eitr)->wait( blocking );
  return 0;
}

int arGUIWindowManager::swapWindowBuffer( const int ID, bool blocking )
{
  arWMEvent* eventHandle = addWMEvent(
    ID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ) );
  if( eventHandle )
    eventHandle->wait( blocking );
  return 0;
}

int arGUIWindowManager::drawWindow( const int ID, bool blocking )
{
  arWMEvent* eventHandle = addWMEvent(
    ID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DRAW ) );
  if( eventHandle )
    eventHandle->wait( blocking );
  return 0;
}

int arGUIWindowManager::swapAllWindowBuffers( bool blocking )
{
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ), blocking );
}

int arGUIWindowManager::drawAllWindows( bool blocking )
{
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DRAW ), blocking );
}

int arGUIWindowManager::consumeWindowEvents( const int ID, bool /*blocking*/ )
{
  return (_threaded || !windowExists(ID)) ? -1 :
    _windows[ ID ]->_consumeWindowEvents();
}

int arGUIWindowManager::consumeAllWindowEvents( bool /*blocking*/ )
{
  if( _threaded )
    return -1;

  bool ok = true;
  for( WindowIterator itr = _windows.begin(); itr != _windows.end(); itr++ ) {
    ok &= itr->second->_consumeWindowEvents() >= 0;
  }
  return ok ? 0 : -1;
}

int arGUIWindowManager::_doEvent(const int ID, const arGUIWindowInfo& event) {
  arWMEvent* eventHandle = addWMEvent( ID, event );
  if (eventHandle)
    eventHandle->wait(false);
  return 0;
}

int arGUIWindowManager::resizeWindow( const int ID, int width, int height )
{
  arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_RESIZE );
  e.setSize(width, height);
  // call the user's window callback with a resize event?
  return _doEvent(ID, e);
}

int arGUIWindowManager::moveWindow( const int ID, int x, int y )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_MOVE, -1, 0, x, y );
  // call the user's window callback with a move event?
  return _doEvent(ID, e);
}

int arGUIWindowManager::setWindowViewport( const int ID, int x, int y, int width, int height )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_VIEWPORT, -1, 0, x, y, width, height);
  return _doEvent(ID, e);
}

int arGUIWindowManager::fullscreenWindow( const int ID )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_FULLSCREEN );
  return _doEvent(ID, e);
}

int arGUIWindowManager::decorateWindow( const int ID, bool decorate )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_DECORATE, -1, decorate?1:0 );
  return _doEvent(ID, e);
}

int arGUIWindowManager::raiseWindow( const int ID, arZOrder zorder )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_RAISE, -1, int(zorder) );
  return _doEvent(ID, e);
}

int arGUIWindowManager::setWindowCursor( const int ID, arCursor cursor )
{
  const arGUIWindowInfo e( AR_WINDOW_EVENT, AR_WINDOW_CURSOR, -1, int(cursor) );
  return _doEvent(ID, e);
}

int arGUIWindowManager::getBpp( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getBpp() : 0;
}

std::string arGUIWindowManager::getTitle( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getTitle() : std::string( "" );
}

std::string arGUIWindowManager::getXDisplay( const int ID ){
  return windowExists(ID) ? _windows[ ID ]->getXDisplay() : std::string( "" );
}

void arGUIWindowManager::setTitle( const int ID, const std::string& title )
{
  if (windowExists(ID))
    _windows[ ID ]->setTitle( title );
}

void arGUIWindowManager::setAllTitles( const std::string& baseTitle, bool overwrite )
{
  const bool fOneWindow = _windows.size() == 1; // bug? this is true more often than it should be?
  for (WindowIterator iter = _windows.begin(); iter != _windows.end(); ++iter) {
    arGUIWindow* win = iter->second;
    if (!overwrite && !win->untitled())
      continue;

    std::string title = baseTitle;
    if (!fOneWindow) {
      ostringstream os;
      os << iter->first;
      title += " #" + os.str();
    }
    win->setTitle( title );
  }
}

arVector3 arGUIWindowManager::getWindowSize( const int ID )
{
  return windowExists(ID) ?
    arVector3( _windows[ ID ]->getWidth(), _windows[ ID ]->getHeight(), 0.0f ) :
    arVector3( -1.0f, -1.0f, -1.0f );
}

arVector3 arGUIWindowManager::getWindowPos( const int ID )
{
  return windowExists(ID) ?
    arVector3( _windows[ ID ]->getPosX(), _windows[ ID ]->getPosY(), 0.0f ) :
    arVector3( -1.0f, -1.0f, -1.0f );
}

bool arGUIWindowManager::isStereo( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isStereo();
}
bool arGUIWindowManager::isFullscreen( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isFullscreen();
}

bool arGUIWindowManager::isDecorated( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isDecorated();
}

arZOrder arGUIWindowManager::getZOrder( const int ID )
{
  return windowExists(ID) ?
    _windows[ ID ]->getZOrder() : AR_ZORDER_TOP /* default - no nil value? */;
}

void* arGUIWindowManager::getUserData( const int ID )
{
  return windowExists(ID) ?  _windows[ ID ]->getUserData() : NULL;
}

void arGUIWindowManager::setUserData( const int ID, void* userData )
{
  if (windowExists(ID))
    _windows[ ID ]->setUserData( userData );
}

arGraphicsWindow* arGUIWindowManager::getGraphicsWindow( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getGraphicsWindow() : NULL;
}

void arGUIWindowManager::returnGraphicsWindow( const int ID )
{
  if (windowExists(ID))
    _windows[ ID ]->returnGraphicsWindow();
}

void arGUIWindowManager::setGraphicsWindow( const int ID, arGraphicsWindow* graphicsWindow )
{
  if (windowExists(ID))
    _windows[ ID ]->setGraphicsWindow( graphicsWindow );
}

arCursor arGUIWindowManager::getWindowCursor( const int ID )
{
  return windowExists(ID) ?
    _windows[ ID ]->getCursor() : AR_CURSOR_NONE;
}

arVector3 arGUIWindowManager::getMousePos( const int ID )
{
  if (!windowExists(ID))
    return arVector3( -1.0f, -1.0f, -1.0f );

  const arGUIMouseInfo mouseState = _windows[ ID ]->getGUIEventManager()->getMouseState();
  return arVector3( float( mouseState.getPosX() ), float( mouseState.getPosY() ), 0.0f );
}

arGUIKeyInfo arGUIWindowManager::getKeyState( const arGUIKey /*key*/ )
{
#if defined( AR_USE_WIN_32 )
#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
#endif
  return arGUIKeyInfo();
}

void arGUIWindowManager::setThreaded( bool threaded ) {
  if (!hasActiveWindows())
    _threaded = threaded;
}

void arGUIWindowManager::_sendDeleteEvent( const int ID )
{
  arWMEvent* eventHandle = addWMEvent( ID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_CLOSE ) );
  if( eventHandle ) {
    // if _threaded, return only after the window has processed the message.
    eventHandle->wait( true );
  }
}

int arGUIWindowManager::deleteWindow( const int ID )
{
  _sendDeleteEvent( ID );
  if (!windowExists(ID))
    return -1;

  // Not sure why deleting this here causes crashes, etc.
  //delete _windows[ ID ];

  _windows.erase( ID );

  // call the user's window callback with a close event? (probably shouldn't,
  // the callback is most likely how we got here in the first place)
  return 0;
}

int arGUIWindowManager::deleteAllWindows( void )
{
  for (WindowIterator witr = _windows.begin(); witr != _windows.end(); ) {
    _sendDeleteEvent( witr->first );
    delete witr->second;
    // Advance iterator "before" erase(), so erase() doesn't invalidate it.
    _windows.erase( witr++ );
  }
  return 0;
}

int arGUIWindowManager::createWindows( const arGUIWindowingConstruct* windowingConstruct, bool useWindowing )
{
  if (!windowingConstruct)
    return -1;

  const std::vector< arGUIXMLWindowConstruct* >* windowConstructs =
    windowingConstruct->getWindowConstructs();

  // If there are multiple windows, default to threaded mode, otherwise nonthreaded.
  // If this call is the result of a reload message, neither
  // setThreaded calls are going to have any effect, since this basic window
  // manager state can bet set only when the window is created.
  setThreaded( windowConstructs->size() > 1 );

  // If the XML mentions threading, override the default.
  int t = windowingConstruct->getThreaded(); // -1 means unassigned, otherwise bool.
  if (t != -1)
    setThreaded(t);

  // By default, no framelocking.
  t = windowingConstruct->getUseFramelock(); // -1 means unassigned, otherwise bool.
  useFramelock((t != -1) && t);

  // Instead of invalidating iterators in the {add|delete}Window calls
  // in the loops below, iterate over a copy of all the window IDs.

  std::vector< int > IDs;
  WindowIterator WItr;
  for( WItr = _windows.begin(); WItr != _windows.end(); WItr++ )
    IDs.push_back( WItr->first );

  std::vector< arGUIXMLWindowConstruct* >::const_iterator cItr;
  std::vector< int >::iterator wItr;

  for( cItr = windowConstructs->begin(), wItr = IDs.begin();
       (cItr != windowConstructs->end()) && (wItr != IDs.end());
       cItr++, wItr++ ) {
    // Abbreviations.
    const int ID = *wItr;
    const arGUIWindowConfig* config = (*cItr)->getWindowConfig();

    // Certain attributes cannot be tweaked during runtime.
    // If they differ from the new attributes, recreate the window.
    // This is overgenerous. The OS should be able
    // to add/remove decoration or enter/leave fullscreen... but some Linux
    // window managers (like Xandros circa 2005) can't.
    // Since it doesn't hurt to remap the windows, recreate anyways,
    // even on OS's where it isn't necessary.
    if( isStereo( ID ) != config->getStereo() ||
        getBpp( ID ) != config->getBpp() ||
        getXDisplay( ID ) != config->getXDisplay() ||
        isDecorated( ID ) != config->getDecorate() ||
        isFullscreen( ID ) != config->getFullscreen() ) {
      // Before deleting the current window, deactivate any framelocking.
      // When a new window has been mapped
      // and the first draw occurs, the user application will be responsible
      // for calling activateFramelock(). (actually the szg frameworks
      // do this).
      deactivateFramelock();
      // delete the current window.
      deleteWindow( ID );

      // create the new window
      if ( addWindow( *config, useWindowing ) < 0 ){
        return -1;
      }
    }
    else {
      // Tweak all the window's settings.
      // If fullscreen, resize before move and decorate.
      // There may be other order interactions as well.
      resizeWindow( ID, config->getWidth(), config->getHeight() );

      // Since decoration can change the window's size, re-resize.
      resizeWindow( ID, config->getWidth(), config->getHeight() );

      moveWindow( ID, config->getPosX(), config->getPosY() );
      setTitle( ID, config->getTitle() );
      setWindowCursor( ID, config->getCursor() );
      raiseWindow( ID, config->getZOrder() );
    }
  }

  // Delete any windows beyond what was parsed.
  for( ; wItr != IDs.end(); wItr++ ) {
    if( deleteWindow( *wItr ) < 0 ) {
      ar_log_warning() << "arGUIWindowManager warning: failed to delete window: " << *wItr << ar_endl;
    }
  }

  // If there are more parsed than created windows, then create them.
  for( ; cItr != windowConstructs->end(); cItr++ ) {
    if( addWindow( *((*cItr)->getWindowConfig()), useWindowing ) < 0 ) {
      // Do not print a complaint here.
      return -1;
    }
  }

  // set all the drawcallbacks and graphicsWindows (Now we can use the 'real'
  // _windows as the collections should now match in size and order and we
  // don't have to worry about invalidating any iterators)
  for( cItr = windowConstructs->begin(), WItr = _windows.begin();
       (cItr != windowConstructs->end()) && (WItr != _windows.end());
       cItr++, WItr++ ) {
    registerDrawCallback( WItr->first, (*cItr)->getGUIDrawCallback() );
    setGraphicsWindow( WItr->first, (*cItr)->getGraphicsWindow() );
  }

  return 0;
}

void arGUIWindowManager::useFramelock( bool isOn )
{
  // This call should be able to work BEFORE the windows have been
  // created (as in createWindows(...)).
  // If not (as in the checks in activateFramelock and deactivateFramelock)
  // framelocking will not work (useWildcatFramelock(true) would never get
  // issued).
  ar_useWildcatFramelock( isOn );
}

void arGUIWindowManager::findFramelock( void )
{
  // This call should be able to work BEFORE the windows have been
  // created (as in createWindows(...)).
  // If not (as in the checks in activateFramelock and deactivateFramelock),
  // findWildcatFramelock() will not get issued.
  ar_findWildcatFramelock();
}

void arGUIWindowManager::activateFramelock( void )
{
  if( _windows.size() == 1 && !_threaded ) {
    ar_activateWildcatFramelock();
    ar_log_remark() << "arGUIWindowManager activated framelock.\n";
  } else {
    ar_log_warning() << "Ignoring attempt to framelock with multiple windows "
                     << " or rendering threads.\n";
  }
}

void arGUIWindowManager::deactivateFramelock( void )
{
  if( _windows.size() == 1 && !_threaded ) {
    ar_deactivateWildcatFramelock();
    ar_log_remark() << "arGUIWindowManager deactivated framelock.\n";
  } else {
    ar_log_warning() << "Ignoring attempt to deactivate framelock with multiple windows "
                     << " or rendering threads.\n";
  }
}
