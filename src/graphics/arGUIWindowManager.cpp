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

arGUIWindowManager::arGUIWindowManager( void (*windowCallback)( arGUIWindowInfo* ) ,
                                        void (*keyboardCallback)( arGUIKeyInfo* ),
                                        void (*mouseCallback)( arGUIMouseInfo* ),
                                        bool singleThreaded ) :
  _keyboardCallback( keyboardCallback ),
  _mouseCallback( mouseCallback ),
  _windowCallback( windowCallback ),
  _maxWindowID( 0 ),
  _singleThreaded( singleThreaded )
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

int arGUIWindowManager::startWithSwap( void )
{
  while( true ) {
    drawAllWindows( false );

    swapAllWindowBuffers( !_singleThreaded );

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

int arGUIWindowManager::addWindow( const arGUIWindowConfig& windowConfig, void (*drawCallback)( arGUIWindowInfo* ) )
{
  arGUIWindow* window = new arGUIWindow( _maxWindowID, windowConfig, drawCallback );

  _windows[ _maxWindowID ] = window;

  if( !_singleThreaded ) {
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
    }
  }

  return _maxWindowID++;
}

int arGUIWindowManager::registerDrawCallback( const int windowID, void (*drawCallback)( arGUIWindowInfo* ) )
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
  // to push any pending gui events onto their stack and process any lingering
  // WM events since they are not doing this in their own thread
  if( _singleThreaded && ( consumeAllWindowEvents() < 0 ) ) {
    std::cerr << "processWindowEvents: consumeAllWindowEvents Error" << std::endl;
  }

  WindowIterator it;

  for( it = _windows.begin(); it != _windows.end(); ++it ) {
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
      switch( GUIInfo->_eventType ) {
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

      // one of the windows got removed out from under us, most likely in a
      // deleteWindow call from the user callback
      if( _windows.find( currentWindow->_ID ) == _windows.end() ) {
        delete currentWindow;

        // it seems like the iterator in the outer for loop gets confused
        // because of the erase, safest just to just return now and deal
        // with the rest of the events next time through (but we should just
        // be able to break; {my lack of stl knowledge shines through})
        return -1;
      }
    }
  }

  return 0;
}

arGUIInfo* arGUIWindowManager::getNextWindowEvent( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return NULL;
  }

  return _windows[ windowID ]->getNextGUIEvent();
}

arWMEvent* arGUIWindowManager::addWMEvent( const int windowID, arGUIWindowInfo event )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return NULL;
  }

  arWMEvent* eventHandle = NULL;

  // ensure that the windowID is set correctly, some functions depend on it
  // being present
  event._windowID = windowID;

  // in single threaded mode, just execute this command directly, no need to
  // do all the message passing
  if( _singleThreaded ) {
    switch( event._state ) {
      case AR_WINDOW_SWAP:
        _windows[ windowID ]->swap();
      break;

      case AR_WINDOW_DRAW:
        _windows[ windowID ]->_drawHandler();
      break;

      case AR_WINDOW_RESIZE:
        _windows[ windowID ]->resize( event._sizeX, event._sizeY );
      break;

      case AR_WINDOW_MOVE:
        _windows[ windowID ]->move( event._posX, event._posY );
      break;

      case AR_WINDOW_VIEWPORT:
        _windows[ windowID ]->setViewport( event._posX, event._posY,
                                           event._sizeX, event._sizeY );
      break;

      case AR_WINDOW_FULLSCREEN:
        _windows[ windowID ]->fullscreen();
      break;

      case AR_WINDOW_DECORATE:
        _windows[ windowID ]->decorate( event._flag == 1 ? true : false );
      break;

      case AR_WINDOW_CLOSE:
        _windows[ windowID ]->_killWindow();

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

int arGUIWindowManager::addAllWMEvent( arGUIWindowInfo wmEvent, bool blocking )
{
  WindowIterator witr;

  EventVector eventHandles;
  EventIterator eitr;

  static bool warn = false;
  if( !warn && blocking && _singleThreaded ) {
    // std::cerr << "Called addAllWMEvent with blocking == true in singleThreaded "
    //           << "mode, are you sure that's what you meant to do?" << std::endl;
    warn = true;
  }

  // first, pass the event to all windows so they can get started on it
  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    arWMEvent* eventHandle = addWMEvent( witr->second->getID(), wmEvent );

    if( eventHandle ) {
      eventHandles.push_back( eventHandle );
    }
    else if( !_singleThreaded ) {
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
  if( _windows.find( windowID ) == _windows.end() ) {
    return -1;
  }

  if( !_singleThreaded ) {
    return -1;
  }

  return _windows[ windowID ]->_consumeWindowEvents();
}


int arGUIWindowManager::consumeAllWindowEvents( bool blocking )
{
  bool allSuccess = 0;

  if( !_singleThreaded ) {
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
  event._sizeX = width;
  event._sizeY = height;

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
  event._posX = x;
  event._posY = y;

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
  event._posX = x;
  event._posY = y;
  event._sizeX = width;
  event._sizeY = height;

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
  event._flag = decorate ? 1 : 0;

  arWMEvent* eventHandle = addWMEvent( windowID, event );

  if( eventHandle ) {
    eventHandle->wait( false );
  }

  return 0;
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

arVector3 arGUIWindowManager::getMousePos( const int windowID )
{
  if( _windows.find( windowID ) == _windows.end() ) {
    return arVector3( -1.0f, -1.0f, -1.0f );
  }

  arGUIMouseInfo mouseState = _windows[ windowID ]->getGUIEventManager()->getMouseState();

  return arVector3( float( mouseState._posX ), float( mouseState._posY ), 0.0f );
}

arGUIKeyInfo getKeyState( const arGUIKey key )
{
  #if defined( AR_USE_WIN_32 )

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #endif

  return arGUIKeyInfo();
}

int arGUIWindowManager::deleteWindow( const int windowID )
{
  arWMEvent* eventHandle = addWMEvent( windowID, arGUIWindowInfo ( AR_WINDOW_EVENT, AR_WINDOW_CLOSE ) );

  if( eventHandle ) {
    // in multi-threading mode, wait until the window has processed the message
    // before returning
    eventHandle->wait( true );
  }

  // do NOT call delete on the window here, processEvents may still be in the
  // middle of a loop on this window.  THIS NEEDS TO BE CHECKED OUT IN
  // GREATER DETAIL!!!!
  _windows.erase( windowID );

  // call the user's window callback with a close event? (probably shouldn't,
  // the callback is most likely how we got here in the first place)

  return 0;
}

int arGUIWindowManager::deleteAllWindows( void )
{
  WindowIterator witr;

  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    if( deleteWindow( witr->first ) < 0 ) {
      // print an error, continue with other deletes?
    }
  }

  return 0;
}

