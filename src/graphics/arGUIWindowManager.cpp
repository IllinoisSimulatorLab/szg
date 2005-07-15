/**
 * @file arGUIWindowManager.cpp
 * Implementation of the arGUIWindowManager class.
 */
#include "arPrecompiled.h"

#include <iostream>

#include "arStructuredData.h"
#include "arMath.h"
#include "arDataUtilities.h"

#include "arGUIWindowManager.h"
#include "arGUIEventManager.h"
#include "arGUIWindow.h"
#include "arGUIXMLParser.h"

arGUIWindowManager::arGUIWindowManager( void (*windowCallback)( arGUIWindowInfo* ) ,
                                        void (*keyboardCallback)( arGUIKeyInfo* ),
                                        void (*mouseCallback)( arGUIMouseInfo* ),
                                        void (*windowInitGLCallback)( arGUIWindowInfo* ),
                                        bool threaded ) :
  _keyboardCallback( keyboardCallback ),
  _mouseCallback( mouseCallback ),
  _windowCallback( windowCallback ),
  _windowInitGLCallback( windowInitGLCallback ),
  _maxWindowID( 0 ),
  _threaded( threaded )
{
  #if defined( AR_USE_WIN_32 )

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  // seems to be necessary on OS X, not necessarily under linux, but probably
  // doesn't hurt to just always enable it though
  if( XInitThreads() == 0 ) {
    std::cerr << "Could not initialize Xlib multi-threading support!" << std::endl;
  }

  #endif

  ar_mutex_init( &_windowsMutex );
}

arGUIWindowManager::~arGUIWindowManager( void )
{
  WindowIterator witr;

  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
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
    // print warning that previous callback is being overwritten?
  }

  _windowCallback = windowCallback;
}

void arGUIWindowManager::registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) )
{
  if( _keyboardCallback ) {
    // print warning that previous callback is being overwritten?
  }

  _keyboardCallback = keyboardCallback;
}

void arGUIWindowManager::registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) )
{
  if( _mouseCallback ) {
    // print warning that previous callback is being overwritten?
  }

  _mouseCallback = mouseCallback;
}

void arGUIWindowManager::registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) )
{
  if( _windowInitGLCallback ) {
    // print warning that previous callback is being overwritten?
  }

  _windowInitGLCallback = windowInitGLCallback;
}

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

int arGUIWindowManager::addWindow( const arGUIWindowConfig& windowConfig )
{
  arGUIWindow* window = new arGUIWindow( _maxWindowID, windowConfig,
                                         _windowInitGLCallback, _userData );

  ar_mutex_lock( &_windowsMutex );

  _windows[ _maxWindowID ] = window;

  if( _threaded ) {
    // this should only return once the window is actually up and running
    if( window->beginEventThread() < 0 ) {
      delete window;
      _windows.erase( _maxWindowID );
      ar_mutex_unlock( &_windowsMutex );
      return -1;
    }
  }
  else {
    if( window->_performWindowCreation() < 0 ) {
      std::cerr << "addWindow: _performWindowCreation error" << std::endl;
      delete window;
      _windows.erase( _maxWindowID );
      ar_mutex_unlock( &_windowsMutex );
      return -1;
    }
  }

  ar_mutex_unlock( &_windowsMutex );

  return _maxWindowID++;
}

int arGUIWindowManager::registerDrawCallback( const int windowID, arGUIRenderCallback* drawCallback )
{
  ar_mutex_lock( &_windowsMutex );

  if( _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return -1;
  }

  _windows[ windowID ]->registerDrawCallback( drawCallback );

  ar_mutex_unlock( &_windowsMutex );

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
  // to push any pending gui events onto their stack and process any lingering
  // WM events since they are not doing this in their own thread
  if( !_threaded && ( consumeAllWindowEvents() < 0 ) ) {
    std::cerr << "processWindowEvents: consumeAllWindowEvents Error" << std::endl;
  }

  ar_mutex_lock( &_windowsMutex );
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
          std::cerr << "processWindowEvents: Unknown Event Type" << std::endl;
        break;
      }

      delete GUIInfo;
    }
  }
  ar_mutex_unlock( &_windowsMutex );

  return 0;
}

arGUIInfo* arGUIWindowManager::getNextWindowEvent( const int windowID )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return NULL;
  }

  arGUIInfo* guiInfo = _windows[ windowID ]->getNextGUIEvent();

  ar_mutex_unlock( &_windowsMutex );

  return guiInfo;
}

arWMEvent* arGUIWindowManager::addWMEvent( const int windowID, arGUIWindowInfo event )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return NULL;
  }

  arWMEvent* eventHandle = NULL;

  // ensure that the windowID is set correctly, some functions depend on it
  // being present
  event.setWindowID( windowID );

  // in single threaded mode, just execute this command directly, no need to
  // do all the message passing
  if( !_threaded ) {
    switch( event.getState() ) {
      case AR_WINDOW_SWAP:
        _windows[ windowID ]->swap();
      break;

      case AR_WINDOW_DRAW:
        _windows[ windowID ]->_drawHandler();
      break;

      case AR_WINDOW_RESIZE:
        _windows[ windowID ]->resize( event.getSizeX(), event.getSizeY() );
      break;

      case AR_WINDOW_MOVE:
        _windows[ windowID ]->move( event.getPosX(), event.getPosY() );
      break;

      case AR_WINDOW_VIEWPORT:
        _windows[ windowID ]->setViewport( event.getPosX(), event.getPosY(),
                                           event.getSizeX(), event.getSizeY() );
      break;

      case AR_WINDOW_FULLSCREEN:
        _windows[ windowID ]->fullscreen();
      break;

      case AR_WINDOW_DECORATE:
        _windows[ windowID ]->decorate( event.getFlag() == 1 ? true : false );
      break;

      case AR_WINDOW_RAISE:
        _windows[ windowID ]->raise( arZOrder( event.getFlag() ) );
      break;

      case AR_WINDOW_CURSOR:
        _windows[ windowID ]->setCursor( arCursor( event.getFlag() ) );
      break;

      case AR_WINDOW_CLOSE:
        std::cout << "FOO 1" << std::endl;
        _windows[ windowID ]->_killWindow();
        std::cout << "FOO 2" << std::endl;
      break;

      default:
        // print error/warning?
      break;
    }
  }
  else {
    eventHandle = _windows[ windowID ]->addWMEvent( event );
  }

  ar_mutex_unlock( &_windowsMutex );

  // do not call wait on the eventHandle here, the caller is the one who
  // decides whether this was a blocking request or not
  return eventHandle;
}

int arGUIWindowManager::addAllWMEvent( arGUIWindowInfo wmEvent,
                                       bool blocking ){
  WindowIterator witr;

  EventVector eventHandles;
  EventIterator eitr;

  static bool warn = false;
  if( !warn && blocking && !_threaded ) {
    // std::cerr << "Called addAllWMEvent with blocking == true in single threaded "
    //           << "mode, are you sure that's what you meant to do?" << std::endl;
    warn = true;
  }

  ar_mutex_lock( &_windowsMutex );

  // first, pass the event to all windows so they can get started on it
  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    // ARGH, how can we make this locking cleaner?
    ar_mutex_unlock( &_windowsMutex );
    arWMEvent* eventHandle = addWMEvent( witr->second->getID(), wmEvent );
    ar_mutex_lock( &_windowsMutex );

    if( eventHandle ) {
      eventHandles.push_back( eventHandle );
    }
    else if( _threaded ) {
      // in single threaded mode, addWMEvent *will* return NULL's
      // print error/warning?
    }
  }

  ar_mutex_unlock( &_windowsMutex );

  // then, if necessary, wait for all the events to complete
  for( eitr = eventHandles.begin(); eitr != eventHandles.end(); eitr++ ) {
    (*eitr)->wait( blocking );
  }

  return 0;
}

int arGUIWindowManager::swapWindowBuffer( const int windowID, bool blocking )
{
  arWMEvent* eventHandle = addWMEvent( windowID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ) );

  if( eventHandle ) {
    eventHandle->wait( blocking );
  }

  return 0;
}

int arGUIWindowManager::swapAllWindowBuffers( bool blocking )
{
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ), blocking );
}

int arGUIWindowManager::drawWindow( const int windowID, bool blocking )
{
  arWMEvent* eventHandle = addWMEvent( windowID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DRAW ) );

  if( eventHandle ) {
    eventHandle->wait( blocking );
  }

  return 0;
}

int arGUIWindowManager::drawAllWindows( bool blocking )
{
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DRAW ), blocking );
}

int arGUIWindowManager::consumeWindowEvents( const int windowID, bool blocking )
{
  ar_mutex_lock( &_windowsMutex );
  if( _threaded || _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return -1;
  }

  int returnVal = _windows[ windowID ]->_consumeWindowEvents();

  ar_mutex_unlock( &_windowsMutex );

  return returnVal;
}


int arGUIWindowManager::consumeAllWindowEvents( bool blocking )
{
  bool allSuccess = 0;

  if( _threaded ) {
    return -1;
  }

  WindowIterator itr;

  ar_mutex_lock( &_windowsMutex );
  for( itr = _windows.begin(); itr != _windows.end(); itr++ ) {
    if( itr->second->_consumeWindowEvents() < 0 ) {
      allSuccess = -1;
    }
  }
  ar_mutex_unlock( &_windowsMutex );

  return allSuccess;
}

int arGUIWindowManager::resizeWindow( const int windowID, int width, int height )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_RESIZE );
  event.setSizeX( width );
  event.setSizeY( height );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  // call the user's window callback with a resize event?

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

int arGUIWindowManager::moveWindow( const int windowID, int x, int y )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_MOVE );
  event.setPosX( x );
  event.setPosY( y );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  // call the user's window callback with a move event?

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

int arGUIWindowManager::setWindowViewport( const int windowID, int x, int y, int width, int height )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_VIEWPORT );
  event.setPosX( x );
  event.setPosY( y );
  event.setSizeX( width );
  event.setSizeY( height );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

int arGUIWindowManager::fullscreenWindow( const int windowID )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_FULLSCREEN );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

int arGUIWindowManager::decorateWindow( const int windowID, bool decorate )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_DECORATE );
  event.setFlag( decorate ? 1 : 0 );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

int arGUIWindowManager::raiseWindow( const int windowID, arZOrder zorder )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_RAISE );
  event.setFlag( int( zorder ) );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

/// Sends an event to the window manager, requesting that a particular
/// window's cursor be set to AR_CURSOR_ARROW, AR_CURSOR_NONE, etc.
/// The setCursor(...) method of the window gets called as a consequence.
int arGUIWindowManager::setWindowCursor( const int windowID, arCursor cursor )
{
  arGUIWindowInfo event( AR_WINDOW_EVENT, AR_WINDOW_CURSOR );
  event.setFlag( int( cursor ) );

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
}

bool arGUIWindowManager::windowExists( const int windowID )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return false;
  }

  ar_mutex_unlock( &_windowsMutex );

  return true;
}

int arGUIWindowManager::getBpp( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return 0;
  }

  return _windows[ windowID ]->getBpp();
}

std::string arGUIWindowManager::getTitle( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return std::string( "" );
  }

  return _windows[ windowID ]->getTitle();
}

std::string arGUIWindowManager::getXDisplay( const int windowID ){
  if( _windows.find( windowID ) == _windows.end() ) {
    return std::string( "" );
  }

  return _windows[ windowID ]->getXDisplay();
}

void arGUIWindowManager::setTitle( const int windowID, const std::string& title )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return;
  }

  _windows[ windowID ]->setTitle( title );
}

arVector3 arGUIWindowManager::getWindowSize( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return arVector3( -1.0f, -1.0f, -1.0f );
  }

  return arVector3( float( _windows[ windowID ]->getWidth() ), float( _windows[ windowID ]->getHeight() ), 0.0f );
}

arVector3 arGUIWindowManager::getWindowPos( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return arVector3( -1.0f, -1.0f, -1.0f );
  }

  return arVector3( float( _windows[ windowID ]->getPosX() ), float( _windows[ windowID ]->getPosY() ), 0.0f );
}

bool arGUIWindowManager::isStereo( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return false;
  }

  return _windows[ windowID ]->isStereo();
}
bool arGUIWindowManager::isFullscreen( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return false;
  }

  return _windows[ windowID ]->isFullscreen();
}

bool arGUIWindowManager::isDecorated( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return true;
  }

  return _windows[ windowID ]->isDecorated();
}

arZOrder arGUIWindowManager::getZOrder( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return false;
  }

  return _windows[ windowID ]->getZOrder();
}

void* arGUIWindowManager::getUserData( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return NULL;
  }

  return _windows[ windowID ]->getUserData();
}

void arGUIWindowManager::setUserData( const int windowID, void* userData )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return;
  }

  _windows[ windowID ]->setUserData( userData );
}

arGraphicsWindow* arGUIWindowManager::getGraphicsWindow( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return NULL;
  }

  return _windows[ windowID ]->getGraphicsWindow();
}

void arGUIWindowManager::returnGraphicsWindow( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return;
  }

  _windows[ windowID ]->returnGraphicsWindow();
}

void arGUIWindowManager::setGraphicsWindow( const int windowID, arGraphicsWindow* graphicsWindow )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return;
  }

  _windows[ windowID ]->setGraphicsWindow( graphicsWindow );
}

arCursor arGUIWindowManager::getWindowCursor( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return AR_CURSOR_NONE;
  }

  return _windows[ windowID ]->getCursor();
}

arVector3 arGUIWindowManager::getMousePos( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return arVector3( -1.0f, -1.0f, -1.0f );
  }

  arGUIMouseInfo mouseState = _windows[ windowID ]->getGUIEventManager()->getMouseState();

  return arVector3( float( mouseState.getPosX() ), float( mouseState.getPosY() ), 0.0f );
}

arGUIKeyInfo arGUIWindowManager::getKeyState( const arGUIKey key )
{
  #if defined( AR_USE_WIN_32 )

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #endif

  return arGUIKeyInfo();
}

void arGUIWindowManager::setThreaded( bool threaded ) {
  ar_mutex_lock( &_windowsMutex );
  if( _windows.size() ) {
    // can't change threading mode once windows have been created
    ar_mutex_unlock( &_windowsMutex );
    return;
  }

  _threaded = threaded;

  ar_mutex_unlock( &_windowsMutex );
}

void arGUIWindowManager::_sendDeleteEvent( const int windowID )
{
  arWMEvent* eventHandle = addWMEvent( windowID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_CLOSE ) );

  if( eventHandle ) {
    // in multi-threading mode, wait until the window has processed the message
    // before returning
    eventHandle->wait( true );
  }
}

int arGUIWindowManager::deleteWindow( const int windowID )
{
  _sendDeleteEvent( windowID );

  ar_mutex_lock( &_windowsMutex );

  if( _windows.find( windowID ) == _windows.end() ) {
    ar_mutex_unlock( &_windowsMutex );
    return -1;
  }

  delete _windows[ windowID ];

  _windows.erase( windowID );

  ar_mutex_unlock( &_windowsMutex );

  // call the user's window callback with a close event? (probably shouldn't,
  // the callback is most likely how we got here in the first place)

  return 0;
}

int arGUIWindowManager::deleteAllWindows( void )
{
  WindowIterator witr = _windows.begin();

  ar_mutex_lock( &_windowsMutex );

  while( witr != _windows.end() ) {
    _sendDeleteEvent( witr->first );

    delete witr->second;

    _windows.erase( witr++ );
  }

  ar_mutex_unlock( &_windowsMutex );

  return 0;
}

int arGUIWindowManager::createWindows( const arGUIWindowingConstruct* windowingConstruct )
{
  if( !windowingConstruct ) {
    return -1;
  }
  std::cout << "creating windows" << std::endl;

  const std::vector< arGUIXMLWindowConstruct* >* windowConstructs = windowingConstruct->getWindowConstructs();

  // If there are multiple windows, default to threaded mode.
  // (this can be forced to a different value from the xml below)
  // note that if this call is the result of a reload message neither
  // setThreaded calls are going to have any affect
  if( windowConstructs->size() > 1 ) {
    setThreaded( true );
  }

  if( windowingConstruct->getThreaded() != -1 ) {
    setThreaded( bool( windowingConstruct->getThreaded() ) );
  }

  if( windowingConstruct->getUseFramelock() != -1 ) {
    useFramelock( bool( windowingConstruct->getUseFramelock() ) );
  }

  // several function calls below also try to lock this mutex, so we'll
  // deadlock if we try to lock it here, instead this entire function should
  // be atomic wrt the window manager's event loop
  // ar_mutex_lock( &_windowsMutex );

  std::vector< arGUIXMLWindowConstruct* >::const_iterator cItr;
  WindowIterator wItr;

  for( cItr = windowConstructs->begin(), wItr = _windows.begin();
       (cItr != windowConstructs->end()) && (wItr != _windows.end());
       cItr++, wItr++ ) {
    // get some easier to handle names
    int windowID = wItr->first;
    const arGUIWindowConfig* config = (*cItr)->getWindowConfig();

    // there are certain attributes that cannot be tweaked during runtime and
    // if they differ from the new attributes the window must be re-created
    if( isStereo( windowID ) != config->getStereo() ||
        getBpp( windowID ) != config->getBpp() ||
        getXDisplay( windowID ) != config->getXDisplay() ) {
      // delete the current window
      deleteWindow( windowID );

      // create the new window
      addWindow( *config );
    }
    else {
      // we can just tweak all the window's settings
      // NOTE: the order of these tweaks *does* matter, if the current window
      // is fullscreen, resize needs to come first or the move and decorate
      // will get ignored.  There may be other order interactions as well

      resizeWindow( windowID, config->getWidth(), config->getHeight() );

      moveWindow( windowID, config->getPosX(), config->getPosY() );

      decorateWindow( windowID, config->getDecorate() );

      setTitle( windowID, config->getTitle() );

      setWindowCursor( windowID, config->getCursor() );

      raiseWindow( windowID, config->getZOrder() );

      if( config->getFullscreen() ) {
        fullscreenWindow( windowID );
      }
    }
  }

  // if there are more created than parsed windows, they then need to be deleted
  if( wItr != _windows.end() ) {
    while( wItr != _windows.end() ) {
      if( deleteWindow( (wItr++)->first ) < 0 ) {
        std::cout << "could not delete window: " << wItr->first << std::endl;
      }
    }
  }

  // if there are more parsed than created windows, they then need to be created
  if( cItr != windowConstructs->end() ) {
    for( ; cItr != windowConstructs->end(); cItr++ ) {
      if( addWindow( *((*cItr)->getWindowConfig()) ) < 0 ) {
        std::cout << "could not create new gui window" << std::endl;
      }
    }
  }

  // set all the drawcallbacks and graphicsWindows (the collections should
  // now match in size and order)
  for( cItr = windowConstructs->begin(), wItr = _windows.begin();
       (cItr != windowConstructs->end()) && (wItr != _windows.end());
       cItr++, wItr++ ) {
    registerDrawCallback( wItr->first, (*cItr)->getGUIDrawCallback() );
    setGraphicsWindow( wItr->first, (*cItr)->getGraphicsWindow() );
  }

  std::cout << "done creating" << std::endl;

  // ar_mutex_unlock( &_windowsMutex );

  return 0;
}

void arGUIWindowManager::useFramelock( bool isOn )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->useWildcatFramelock( isOn );
  }
  ar_mutex_unlock( &_windowsMutex );
}

void arGUIWindowManager::findFramelock( void )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->findWildcatFramelock();
  }
  ar_mutex_unlock( &_windowsMutex );
}

void arGUIWindowManager::activateFramelock( void )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->activateWildcatFramelock();
  }
  ar_mutex_unlock( &_windowsMutex );
}

void arGUIWindowManager::deactivateFramelock( void )
{
  ar_mutex_lock( &_windowsMutex );
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->deactivateWildcatFramelock();
  }
  ar_mutex_unlock( &_windowsMutex );
}

