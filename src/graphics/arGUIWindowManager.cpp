/**
 * @file arGUIWindowManager.cpp
 * Implementation of the arGUIWindowManager class.
 */
#include "arPrecompiled.h"

#include <iostream>
#include <sstream>

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

int arGUIWindowManager::addWindow( const arGUIWindowConfig& windowConfig,
                                   bool useWindowing )
{
  arGUIWindow* window = new arGUIWindow( _maxWindowID, windowConfig,
                                         _windowInitGLCallback, _userData );

  _windows[ _maxWindowID ] = window;

  // Only make the OS window if it has been requested.
  if (useWindowing){
    if( _threaded ) {
      // this should only return once the window is actually up and running
      if( window->beginEventThread() < 0 ) {
        delete window;
        _windows.erase( _maxWindowID );
        return -1;
      }
    }
    else {
      if( window->_performWindowCreation() < 0 ) {
        std::cerr << "addWindow: _performWindowCreation error" << std::endl;
        delete window;
        _windows.erase( _maxWindowID );
        return -1;
      }
    }
  }

  return _maxWindowID++;
}

int arGUIWindowManager::registerDrawCallback( const int windowID, arGUIRenderCallback* drawCallback )
{

  if( _windows.find( windowID ) == _windows.end() ) {
    return -1;
  }

  _windows[ windowID ]->registerDrawCallback( drawCallback );

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
    std::cerr << "processWindowEvents: consumeAllWindowEvents Error" << std::endl;
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
          std::cerr << "processWindowEvents: Unknown Event Type" << std::endl;
        break;
      }

      delete GUIInfo;
    }
  }

  return 0;
}

arGUIInfo* arGUIWindowManager::getNextWindowEvent( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return NULL;
  }

  arGUIInfo* guiInfo = _windows[ windowID ]->getNextGUIEvent();

  return guiInfo;
}

arWMEvent* arGUIWindowManager::addWMEvent( const int windowID, arGUIWindowInfo event )
{
  if( _windows.find( windowID ) == _windows.end() ) {
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
        _windows[ windowID ]->decorate( (event.getFlag() == 1) );
      break;

      case AR_WINDOW_RAISE:
        _windows[ windowID ]->raise( arZOrder( event.getFlag() ) );
      break;

      case AR_WINDOW_CURSOR:
        _windows[ windowID ]->setCursor( arCursor( event.getFlag() ) );
      break;

      case AR_WINDOW_CLOSE:
        _windows[ windowID ]->_killWindow();
      break;

      default:
        // print error/warning?
      break;
    }
  }
  else {
    eventHandle = _windows[ windowID ]->addWMEvent( event );
  }

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

  // first, pass the event to all windows so they can get started on it
  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    arWMEvent* eventHandle = addWMEvent( witr->second->getID(), wmEvent );

    if( eventHandle ) {
      eventHandles.push_back( eventHandle );
    }
    else if( _threaded ) {
      // in single threaded mode, addWMEvent *will* return NULL's
      // print error/warning?
    }
  }

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
  if( _threaded || _windows.find( windowID ) == _windows.end() ) {
    return -1;
  }

  int returnVal = _windows[ windowID ]->_consumeWindowEvents();

  return returnVal;
}


int arGUIWindowManager::consumeAllWindowEvents( bool blocking )
{
  bool allSuccess = 0;

  if( _threaded ) {
    return -1;
  }

  WindowIterator itr;

  for( itr = _windows.begin(); itr != _windows.end(); itr++ ) {
    if( itr->second->_consumeWindowEvents() < 0 ) {
      allSuccess = -1;
    }
  }

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
  if( _windows.find( windowID ) == _windows.end() ) {
    return false;
  }

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

void arGUIWindowManager::setAllTitles( const std::string& baseTitle, bool overwrite )
{
  ostringstream os;
  WindowIterator iter;
  for (iter = _windows.begin(); iter != _windows.end(); ++iter) {
    int ID = iter->first;
    os.str(""); // clear it.
    os << ID;
    std::string title = baseTitle + " #" + os.str();
    arGUIWindow* win = iter->second;
    if (overwrite || win->getTitle() == "SyzygyWindow") {
      win->setTitle( title );
    }
  }
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
  if( _windows.size() ) {
    // can't change threading mode once windows have been created
    return;
  }

  _threaded = threaded;

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

  if( _windows.find( windowID ) == _windows.end() ) {
    return -1;
  }

  // Hmmmm. Not quite sure (yet) why deleting this here seems like BAD news.
  // (i.e. causing crashes, etc.)
  //delete _windows[ windowID ];

  _windows.erase( windowID );

  // call the user's window callback with a close event? (probably shouldn't,
  // the callback is most likely how we got here in the first place)

  return 0;
}

int arGUIWindowManager::deleteAllWindows( void )
{
  WindowIterator witr = _windows.begin();

  while( witr != _windows.end() ) {
    _sendDeleteEvent( witr->first );

    delete witr->second;

    // make sure the iterator is advanced otherwise it will be invalidated
    // by the call to erase
    _windows.erase( witr++ );
  }

  return 0;
}

int arGUIWindowManager::createWindows( const arGUIWindowingConstruct* windowingConstruct, bool useWindowing )
{
  if( !windowingConstruct ) {
    return -1;
  }

  std::cout << "arGUIWindowManager remark:: creating windows." << std::endl;

  const std::vector< arGUIXMLWindowConstruct* >* windowConstructs
    = windowingConstruct->getWindowConstructs();

  // If there are multiple windows, default to threaded mode.
  // If there is just one window, default to non-threaded mode.
  // Note that if this call is the result of a reload message neither
  // setThreaded calls are going to have any affect, since this basic window
  // manager state can only be altered at the initial window creation.
  if( windowConstructs->size() > 1 ) {
    setThreaded( true );
  }
  else{
    setThreaded( false );
  }

  // If the XML has something explicit to say about threading, go ahead and
  // override the defaults listed above.
  if( windowingConstruct->getThreaded() != -1 ) {
    setThreaded( bool( windowingConstruct->getThreaded() ) );
  }

  // By default, we are NOT use framelocking.
  if( windowingConstruct->getUseFramelock() != -1 ) {
    useFramelock( bool( windowingConstruct->getUseFramelock() ) );
  }
  else{
    useFramelock( false );
  }

  // instead of trying to worry about invalidating iterators with the
  // {add|delete}Window calls in the loops below, we first make a temp
  // copy of all the window IDs and iterate over that instead.
  std::vector< int > windowIDs;
  WindowIterator WItr;
  for( WItr = _windows.begin(); WItr != _windows.end(); WItr++ ) {
    windowIDs.push_back( WItr->first );
  }

  std::vector< arGUIXMLWindowConstruct* >::const_iterator cItr;
  std::vector< int >::iterator wItr;

  for( cItr = windowConstructs->begin(), wItr = windowIDs.begin();
       (cItr != windowConstructs->end()) && (wItr != windowIDs.end());
       cItr++, wItr++ ) {
    // get some easier to handle names
    int windowID = *wItr;
    const arGUIWindowConfig* config = (*cItr)->getWindowConfig();

    // there are certain attributes that cannot be tweaked during runtime and
    // if they differ from the new attributes the window must be re-created.
    // NOTE: we are a little bit *over* generous here. The OS *should* be able
    // to add/remove decoration or enter/leave fullscreen... but some Linux
    // window managers (like the one in Xandros circa 2005) are not so
    // capable. In reality, it doesn't hurt to have the remap the windows, so
    // go ahead in these cases (even for platforms where it isn't strictly 
    // necessary).
    if( isStereo( windowID ) != config->getStereo() ||
        getBpp( windowID ) != config->getBpp() ||
        getXDisplay( windowID ) != config->getXDisplay() ||
        isDecorated( windowID ) != config->getDecorate() ||
        isFullscreen( windowID ) != config->getFullscreen() ) {
      // Before we delete the current window, it is important to
      // *deactivate* any framelocking. This is essentially a harmless
      // call if framelocking is not used. When a new window has been mapped
      // and the first draw occurs, the user application will be responsible
      // for calling the activateFramelock(). (actually the szg frameworks
      // do this).
      deactivateFramelock();
      // delete the current window.
      deleteWindow( windowID );

      // create the new window
      addWindow( *config, useWindowing );
    }
    else {
      // we can just tweak all the window's settings
      // NOTE: the order of these tweaks *does* matter, if the current window
      // is fullscreen, resize needs to come first or the move and decorate
      // will get ignored.  There may be other order interactions as well
      resizeWindow( windowID, config->getWidth(), config->getHeight() );

      // The check ensures that decorate <-> not decorate, there is a new
      // window.
      //decorateWindow( windowID, config->getDecorate() );

      // since the decoration can affect the size of the window, we have to
      // resize /again/ to get our real requested size
      resizeWindow( windowID, config->getWidth(), config->getHeight() );

      moveWindow( windowID, config->getPosX(), config->getPosY() );

      setTitle( windowID, config->getTitle() );

      setWindowCursor( windowID, config->getCursor() );

      raiseWindow( windowID, config->getZOrder() );

      // The check ensures that nonfullscreen <-> fullscreen, there is
      // a new window.
      //if( config->getFullscreen() ) {
      //  fullscreenWindow( windowID );
      //}
    }
  }

  // If there are more already existing than parsed windows, they then need 
  // to be deleted.
  if( wItr != windowIDs.end() ) {
    for( ; wItr != windowIDs.end(); wItr++ ) {
      if( deleteWindow( *wItr ) < 0 ) {
        std::cerr << "could not delete window: " << *wItr << std::endl;
      }
    }
  }

  // If there are more parsed than created windows, they then need to be 
  // created
  if( cItr != windowConstructs->end() ) {
    for( ; cItr != windowConstructs->end(); cItr++ ) {
      if( addWindow( *((*cItr)->getWindowConfig()), useWindowing ) < 0 ) {
        std::cerr << "could not create new gui window" << std::endl;
      }
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

  std::cout << "arGUIWindowManager remark:: done creating." << std::endl;

  return 0;
}

void arGUIWindowManager::useFramelock( bool isOn )
{
  // This call should be able to work BEFORE the windows have been
  // created (as in createWindows(...)).
  // If not (as in the checks in activateFramelock and deactivateFramelock)
  // framelocking will not work (useWildcatFramelock(true) would never get
  // issued).
  _windows.begin()->second->useWildcatFramelock( isOn );
}

void arGUIWindowManager::findFramelock( void )
{
  // This call should be able to work BEFORE the windows have been
  // created (as in createWindows(...)).
  // If not (as in the checks in activateFramelock and deactivateFramelock),
  // findWildcatFramelock() will not get issued.
  _windows.begin()->second->findWildcatFramelock();
}

void arGUIWindowManager::activateFramelock( void )
{
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->activateWildcatFramelock();
  }
}

void arGUIWindowManager::deactivateFramelock( void )
{
  if( _windows.size() == 1 && !_threaded ) {
    _windows.begin()->second->deactivateWildcatFramelock();
  }
}

