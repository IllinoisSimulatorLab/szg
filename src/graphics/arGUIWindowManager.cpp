//@+leo-ver=4-thin
//@+node:jimc.20100409112755.102:@thin graphics\arGUIWindowManager.cpp
//@@language c++
//@@tabwidth -2
//@+others
//@+node:jimc.20100409112755.103:ar_windowManagerDefaultKeyboardFunction
#include "arPrecompiled.h"

#include "arStructuredData.h"
#include "arMath.h"
#include "arDataUtilities.h"
#include "arLogStream.h"
#include "arGUIWindowManager.h"
#include "arGUIEventManager.h"
#include "arGUIWindow.h"
#include "arGUIXMLParser.h"
#include "arFramelockUtilities.h"
#include "arTexture.h"

// Default callbacks for window and keyboard events.

void ar_windowManagerDefaultKeyboardFunction( arGUIKeyInfo* ki) {
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
//@-node:jimc.20100409112755.103:ar_windowManagerDefaultKeyboardFunction
//@+node:jimc.20100409112755.104:ar_windowManagerDefaultWindowFunction

void ar_windowManagerDefaultWindowFunction( arGUIWindowInfo* wi ) {
  if (!wi)
    return;
  arGUIWindowManager* wm = wi->getWindowManager();
  if (!wm)
    return;

  switch( wi->getState() ) {
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
//@-node:jimc.20100409112755.104:ar_windowManagerDefaultWindowFunction
//@+node:jimc.20100409112755.105:arGUIWindowManager::arGUIWindowManager

arGUIWindowManager::arGUIWindowManager( void (*windowCB)( arGUIWindowInfo* ) ,
                                        void (*keyboardCB)( arGUIKeyInfo* ),
                                        void (*mouseCB)( arGUIMouseInfo* ),
                                        void (*windowInitGLCB)( arGUIWindowInfo* ),
                                        bool threaded ) :
  _keyboardCallback( keyboardCB ? keyboardCB : ar_windowManagerDefaultKeyboardFunction),
  _mouseCallback( mouseCB ),
  _windowCallback( windowCB ? windowCB : ar_windowManagerDefaultWindowFunction),
  _windowInitGLCallback( windowInitGLCB ),
  _windowingConstruct( NULL ),
  _maxID( 0 ),
  _threaded( threaded ),
  _fActivatedFramelock( false )
{
#if defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
  // seems to be necessary on OS X, not necessarily under linux, but probably
  // doesn't hurt to just always enable it though
  if ( XInitThreads() == 0 ) {
    ar_log_error() << "arGUIWindowManager failed to init Xlib multi-threading.\n";
  }
#endif
  arTexture_setThreaded(_threaded); // Wherever _threaded is set, propagate that to arTexture.
}
//@-node:jimc.20100409112755.105:arGUIWindowManager::arGUIWindowManager
//@+node:jimc.20100409112755.106:Destructor

arGUIWindowManager::~arGUIWindowManager( void )
{
  for( WindowIterator witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    delete witr->second;
  }
}
//@-node:jimc.20100409112755.106:Destructor
//@+node:jimc.20100609152149.1:arGUIWindowManager::hasStereoWindows
bool arGUIWindowManager::hasStereoWindows( void ) {
  for( WindowIterator it = _windows.begin(); it != _windows.end(); ++it ) {
    if (it->second->isStereo()) {
      return true;
    }
  }
  return false;
}
//@-node:jimc.20100609152149.1:arGUIWindowManager::hasStereoWindows
//@+node:jimc.20100409112755.107:arGUIWindowManager::_keyboardHandler

void arGUIWindowManager::_keyboardHandler( arGUIKeyInfo* keyInfo )
{
  if ( _keyboardCallback ) {
    _keyboardCallback( keyInfo );
  }
}
//@-node:jimc.20100409112755.107:arGUIWindowManager::_keyboardHandler
//@+node:jimc.20100409112755.108:arGUIWindowManager::_mouseHandler

void arGUIWindowManager::_mouseHandler( arGUIMouseInfo* mouseInfo )
{
  if ( _mouseCallback ) {
    _mouseCallback( mouseInfo );
  }
}
//@-node:jimc.20100409112755.108:arGUIWindowManager::_mouseHandler
//@+node:jimc.20100409112755.109:arGUIWindowManager::_windowHandler

void arGUIWindowManager::_windowHandler( arGUIWindowInfo* windowInfo )
{
  if ( _windowCallback ) {
    _windowCallback( windowInfo );
  }
}
//@-node:jimc.20100409112755.109:arGUIWindowManager::_windowHandler
//@+node:jimc.20100409112755.110:arGUIWindowManager::registerWindowCallback

void arGUIWindowManager::registerWindowCallback( void (*windowCallback) ( arGUIWindowInfo* ) )
{
  if ( _windowCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new window callback.\n";
  }
  _windowCallback = windowCallback;
}
//@-node:jimc.20100409112755.110:arGUIWindowManager::registerWindowCallback
//@+node:jimc.20100409112755.111:arGUIWindowManager::registerKeyboardCallback

void arGUIWindowManager::registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) )
{
  if ( _keyboardCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new keyboard callback.\n";
  }
  _keyboardCallback = keyboardCallback;
}
//@-node:jimc.20100409112755.111:arGUIWindowManager::registerKeyboardCallback
//@+node:jimc.20100409112755.112:arGUIWindowManager::registerMouseCallback

void arGUIWindowManager::registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) )
{
  if ( _mouseCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new mouse callback.\n";
  }
  _mouseCallback = mouseCallback;
}
//@-node:jimc.20100409112755.112:arGUIWindowManager::registerMouseCallback
//@+node:jimc.20100409112755.113:arGUIWindowManager::registerWindowInitGLCallback

void arGUIWindowManager::registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) )
{
  if ( _windowInitGLCallback ) {
    ar_log_debug() << "arGUIWindowManager installing new window init GL callback.\n";
  }
  _windowInitGLCallback = windowInitGLCallback;
}
//@-node:jimc.20100409112755.113:arGUIWindowManager::registerWindowInitGLCallback
//@+node:jimc.20100409112755.114:arGUIWindowManager::startWithSwap

// todo: merge these two.

int arGUIWindowManager::startWithSwap( void )
{
  while ( true ) {
    drawAllWindows( false );
    swapAllWindowBuffers( true );
    processWindowEvents();
  }
  return 0;
}
//@-node:jimc.20100409112755.114:arGUIWindowManager::startWithSwap
//@+node:jimc.20100409112755.115:arGUIWindowManager::startWithoutSwap

int arGUIWindowManager::startWithoutSwap( void )
{
  while ( true ) {
    drawAllWindows( false );
    processWindowEvents();
  }
  return 0;
}
//@-node:jimc.20100409112755.115:arGUIWindowManager::startWithoutSwap
//@+node:jimc.20100409112755.116:arGUIWindowManager::addWindow

int arGUIWindowManager::addWindow( const arGUIWindowConfig& windowConfig,
                                   bool useWindowing )
{
  arGUIWindow* window = new arGUIWindow( _maxID, windowConfig,
                                         _windowInitGLCallback, _userData );
  // Tell the window who owns it... so events will include this info.
  // Let event processing callbacks refer to the window manager.
  window->setWindowManager( this );

  _windows[ _maxID ] = window;

  // Make the OS window, if it was requested.
  if (useWindowing) {
    if ( _threaded ) {
      // This should only return once the window is actually up and running
      if ( window->beginEventThread() < 0 ) {
        // Already printed warning.
        delete window;
        _windows.erase( _maxID );
        return -1;
      }
    }
    else {
      if ( window->_performWindowCreation() < 0 ) {
        // Already printed warning.
        delete window;
        _windows.erase( _maxID );
        return -1;
      }
    }
  }

  return _maxID++;
}
//@-node:jimc.20100409112755.116:arGUIWindowManager::addWindow
//@+node:jimc.20100409112755.117:arGUIWindowManager::registerDrawCallback

int arGUIWindowManager::registerDrawCallback( const int ID, arGUIRenderCallback* drawCallback )
{
  if (!windowExists(ID))
    return -1;

  _windows[ ID ]->registerDrawCallback( drawCallback );
  return 0;
}
//@-node:jimc.20100409112755.117:arGUIWindowManager::registerDrawCallback
//@+node:jimc.20100409112755.118:arGUIWindowManager::processWindowEvents

int arGUIWindowManager::processWindowEvents( void )
{
  // no registered callbacks, user should be handling events with getNextEvent
  // (what about window close events?)
  if ( !_keyboardCallback && !_mouseCallback && !_windowCallback ) {
    // could be subclassed, in which case this is actually ok
    // return -1;
  }

  // if the WM is in single-threaded mode, first it needs to tell the windows
  // to push any pending gui events onto their stack since they are not doing
  // this in their own thread
  if ( !_threaded && ( consumeAllWindowEvents() < 0 ) ) {
    ar_log_error() << "arGUIWindowManager processWindowEvents consumeAllWindowEvents problem.\n";
  }

  for( WindowIterator it = _windows.begin(); it != _windows.end(); ++it ) {
    arGUIWindow* currentWindow = it->second;

    while ( currentWindow->eventsPending() ) {
      arGUIInfo* GUIInfo = currentWindow->getNextGUIEvent();

      if ( !GUIInfo ) {
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
          if ( _keyboardCallback ) {
            _keyboardHandler( (arGUIKeyInfo*) GUIInfo );
          }
        break;

        case AR_MOUSE_EVENT:
          if ( _mouseCallback ) {
            _mouseHandler( (arGUIMouseInfo*) GUIInfo );
          }
        break;

        case AR_WINDOW_EVENT:
          if ( _windowCallback ) {
            _windowHandler( (arGUIWindowInfo*) GUIInfo );
          }
          else {
           // take whatever measures are necessary for minimal functionality
           // (i.e. if the user never registered a callback, at least handle
           // the close event)
          }
        break;

        default:
          ar_log_error() << "arGUIWindowManager processWindowEvents: Unknown Event Type" << ar_endl;
        break;
      }

      delete GUIInfo;
    }
  }

  return 0;
}
//@-node:jimc.20100409112755.118:arGUIWindowManager::processWindowEvents
//@+node:jimc.20100409112755.119:arGUIWindowManager::getNextWindowEvent

arGUIInfo* arGUIWindowManager::getNextWindowEvent( const int ID )
{
  return windowExists(ID) ?  _windows[ ID ]->getNextGUIEvent() : NULL;
}
//@-node:jimc.20100409112755.119:arGUIWindowManager::getNextWindowEvent
//@+node:jimc.20100409112755.120:arGUIWindowManager::addWMEvent

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

      case AR_WINDOW_DRAW_LEFT:
        w->_drawHandler( true );
      break;

      case AR_WINDOW_DRAW_RIGHT:
        w->_drawHandler( false );
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
//@-node:jimc.20100409112755.120:arGUIWindowManager::addWMEvent
//@+node:jimc.20100409112755.121:arGUIWindowManager::addAllWMEvent

int arGUIWindowManager::addAllWMEvent( arGUIWindowInfo wmEvent, bool blocking ) {
  static bool warn = false;
  if ( !warn && blocking && !_threaded ) {
    // Bugs in a syzygy framework might cause a deadlock here.
    ar_log_debug() << "arGUIWindowManager: addAllWMEvent blocking while singlethreaded.\n";
    warn = true;
  }

  // Pass the event to all windows so they can get started on it
  WindowIterator witr;
  EventVector eventHandles;
  for( witr = _windows.begin(); witr != _windows.end(); witr++ ) {
    arWMEvent* eventHandle = addWMEvent( witr->second->getID(), wmEvent );
    if ( eventHandle ) {
      eventHandles.push_back( eventHandle );
    }
    else if ( _threaded ) {
      // If !_threaded, addWMEvent returns NULL.  Warn?
    }
  }

  // Wait for the events to complete.
  for( EventIterator eitr = eventHandles.begin(); eitr != eventHandles.end(); eitr++ )
    (*eitr)->wait( blocking );
  return 0;
}
//@-node:jimc.20100409112755.121:arGUIWindowManager::addAllWMEvent
//@+node:jimc.20100409112755.122:arGUIWindowManager::swapWindowBuffer

int arGUIWindowManager::swapWindowBuffer( const int ID, bool blocking )
{
  arWMEvent* eventHandle = addWMEvent(
    ID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ) );
  if ( eventHandle )
    eventHandle->wait( blocking );
  return 0;
}
//@-node:jimc.20100409112755.122:arGUIWindowManager::swapWindowBuffer
//@+node:jimc.20100409112755.123:arGUIWindowManager::drawWindow

int arGUIWindowManager::drawWindow( const int ID, const bool drawLeftBuffer, bool blocking )
{
  arGUIState winInfoState = (drawLeftBuffer)?(AR_WINDOW_DRAW_LEFT):(AR_WINDOW_DRAW_RIGHT);
  arWMEvent* eventHandle = addWMEvent(
    ID, arGUIWindowInfo( AR_WINDOW_EVENT, winInfoState ) );
  if ( eventHandle )
    eventHandle->wait( blocking );
  return 0;
}
//@-node:jimc.20100409112755.123:arGUIWindowManager::drawWindow
//@+node:jimc.20100409112755.124:arGUIWindowManager::swapAllWindowBuffers

int arGUIWindowManager::swapAllWindowBuffers( bool blocking )
{
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_SWAP ), blocking );
}
//@-node:jimc.20100409112755.124:arGUIWindowManager::swapAllWindowBuffers
//@+node:jimc.20100409112755.125:arGUIWindowManager::drawAllWindows

int arGUIWindowManager::drawAllWindows( const bool drawLeftBuffer, bool blocking )
{
  arGUIState winInfoState = (drawLeftBuffer)?(AR_WINDOW_DRAW_LEFT):(AR_WINDOW_DRAW_RIGHT);
  return addAllWMEvent( arGUIWindowInfo( AR_WINDOW_EVENT, winInfoState ), blocking );
}
//@-node:jimc.20100409112755.125:arGUIWindowManager::drawAllWindows
//@+node:jimc.20100409112755.126:arGUIWindowManager::consumeWindowEvents

int arGUIWindowManager::consumeWindowEvents( const int ID, bool /*blocking*/ )
{
  return (_threaded || !windowExists(ID)) ? -1 :
    _windows[ ID ]->_consumeWindowEvents();
}
//@-node:jimc.20100409112755.126:arGUIWindowManager::consumeWindowEvents
//@+node:jimc.20100409112755.127:arGUIWindowManager::consumeAllWindowEvents

int arGUIWindowManager::consumeAllWindowEvents( bool /*blocking*/ )
{
  if ( _threaded )
    return -1;

  bool ok = true;
  for( WindowIterator itr = _windows.begin(); itr != _windows.end(); itr++ ) {
    ok &= itr->second->_consumeWindowEvents() >= 0;
  }
  return ok ? 0 : -1;
}
//@-node:jimc.20100409112755.127:arGUIWindowManager::consumeAllWindowEvents
//@+node:jimc.20100409112755.128:arGUIWindowManager::_doEvent

int arGUIWindowManager::_doEvent(const int ID, const arGUIWindowInfo& event) {
  arWMEvent* eventHandle = addWMEvent( ID, event );
  if (eventHandle)
    eventHandle->wait(false);
  return 0;
}
//@-node:jimc.20100409112755.128:arGUIWindowManager::_doEvent
//@+node:jimc.20100409112755.129:arGUIWindowManager::resizeWindow

int arGUIWindowManager::resizeWindow( const int ID, int width, int height )
{
  // call the user's window callback with a resize event?
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RESIZE, -1, 0, -1, -1, width, height ));
}
//@-node:jimc.20100409112755.129:arGUIWindowManager::resizeWindow
//@+node:jimc.20100409112755.130:arGUIWindowManager::moveWindow

int arGUIWindowManager::moveWindow( const int ID, int x, int y )
{
  // call the user's window callback with a move event?
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_MOVE, -1, 0, x, y ));
}
//@-node:jimc.20100409112755.130:arGUIWindowManager::moveWindow
//@+node:jimc.20100409112755.131:arGUIWindowManager::setWindowViewport

int arGUIWindowManager::setWindowViewport( const int ID, int x, int y, int width, int height )
{
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_VIEWPORT, -1, 0, x, y, width, height));
}
//@-node:jimc.20100409112755.131:arGUIWindowManager::setWindowViewport
//@+node:jimc.20100409112755.132:arGUIWindowManager::fullscreenWindow

int arGUIWindowManager::fullscreenWindow( const int ID )
{
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_FULLSCREEN ));
}
//@-node:jimc.20100409112755.132:arGUIWindowManager::fullscreenWindow
//@+node:jimc.20100409112755.133:arGUIWindowManager::decorateWindow

int arGUIWindowManager::decorateWindow( const int ID, bool decorate )
{
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_DECORATE, -1, decorate?1:0 ));
}
//@-node:jimc.20100409112755.133:arGUIWindowManager::decorateWindow
//@+node:jimc.20100409112755.134:arGUIWindowManager::raiseWindow

int arGUIWindowManager::raiseWindow( const int ID, arZOrder zorder )
{
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RAISE, -1, int(zorder) ));
}
//@-node:jimc.20100409112755.134:arGUIWindowManager::raiseWindow
//@+node:jimc.20100409112755.135:arGUIWindowManager::setWindowCursor

int arGUIWindowManager::setWindowCursor( const int ID, arCursor cursor )
{
  return _doEvent(ID,
    arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_CURSOR, -1, int(cursor) ));
}
//@-node:jimc.20100409112755.135:arGUIWindowManager::setWindowCursor
//@+node:jimc.20100409112755.136:arGUIWindowManager::getBpp

int arGUIWindowManager::getBpp( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getBpp() : 0;
}
//@-node:jimc.20100409112755.136:arGUIWindowManager::getBpp
//@+node:jimc.20100409112755.137:arGUIWindowManager::getTitle

std::string arGUIWindowManager::getTitle( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getTitle() : std::string( "" );
}
//@-node:jimc.20100409112755.137:arGUIWindowManager::getTitle
//@+node:jimc.20100409112755.138:arGUIWindowManager::getXDisplay

std::string arGUIWindowManager::getXDisplay( const int ID ) {
  return windowExists(ID) ? _windows[ ID ]->getXDisplay() : std::string( "" );
}
//@-node:jimc.20100409112755.138:arGUIWindowManager::getXDisplay
//@+node:jimc.20100409112755.139:arGUIWindowManager::setTitle

void arGUIWindowManager::setTitle( const int ID, const std::string& title )
{
  if (windowExists(ID))
    _windows[ ID ]->setTitle( title );
}
//@-node:jimc.20100409112755.139:arGUIWindowManager::setTitle
//@+node:jimc.20100409112755.140:arGUIWindowManager::setAllTitles

void arGUIWindowManager::setAllTitles( const std::string& baseTitle, bool overwrite )
{
#ifdef NEVER_TRUE
  const bool fManyWindows = _windows.size() != 1; // Bug. *always* true!
#endif
  for (WindowIterator iter = _windows.begin(); iter != _windows.end(); ++iter) {
    arGUIWindow* win = iter->second;
    if (!overwrite && !win->untitled())
      continue;

    std::string title(baseTitle);
#ifdef NEVER_TRUE
    if (fManyWindows) {
      title += " #" + ar_intToString(iter->first);
    }
#endif
    win->setTitle( title );
  }
}
//@-node:jimc.20100409112755.140:arGUIWindowManager::setAllTitles
//@+node:jimc.20100409112755.141:arGUIWindowManager::getWindowSize

arVector3 arGUIWindowManager::getWindowSize( const int ID )
{
  return windowExists(ID) ?
    arVector3( _windows[ ID ]->getWidth(), _windows[ ID ]->getHeight(), 0.0f ) :
    arVector3( -1.0f, -1.0f, -1.0f );
}
//@-node:jimc.20100409112755.141:arGUIWindowManager::getWindowSize
//@+node:jimc.20100409112755.142:arGUIWindowManager::getWindowPos

arVector3 arGUIWindowManager::getWindowPos( const int ID )
{
  return windowExists(ID) ?
    arVector3( _windows[ ID ]->getPosX(), _windows[ ID ]->getPosY(), 0.0f ) :
    arVector3( -1.0f, -1.0f, -1.0f );
}
//@-node:jimc.20100409112755.142:arGUIWindowManager::getWindowPos
//@+node:jimc.20100409112755.143:arGUIWindowManager::isStereo

bool arGUIWindowManager::isStereo( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isStereo();
}
//@-node:jimc.20100409112755.143:arGUIWindowManager::isStereo
//@+node:jimc.20100409112755.144:arGUIWindowManager::isFullscreen
bool arGUIWindowManager::isFullscreen( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isFullscreen();
}
//@-node:jimc.20100409112755.144:arGUIWindowManager::isFullscreen
//@+node:jimc.20100409112755.145:arGUIWindowManager::isDecorated

bool arGUIWindowManager::isDecorated( const int ID )
{
  return windowExists(ID) && _windows[ ID ]->isDecorated();
}
//@-node:jimc.20100409112755.145:arGUIWindowManager::isDecorated
//@+node:jimc.20100409112755.146:arGUIWindowManager::getZOrder

arZOrder arGUIWindowManager::getZOrder( const int ID )
{
  return windowExists(ID) ?
    _windows[ ID ]->getZOrder() : AR_ZORDER_TOP /* default - no nil value? */;
}
//@-node:jimc.20100409112755.146:arGUIWindowManager::getZOrder
//@+node:jimc.20100409112755.147:arGUIWindowManager::getUserData

void* arGUIWindowManager::getUserData( const int ID )
{
  return windowExists(ID) ?  _windows[ ID ]->getUserData() : NULL;
}
//@-node:jimc.20100409112755.147:arGUIWindowManager::getUserData
//@+node:jimc.20100409112755.148:arGUIWindowManager::setUserData

void arGUIWindowManager::setUserData( const int ID, void* userData )
{
  if (windowExists(ID))
    _windows[ ID ]->setUserData( userData );
}
//@-node:jimc.20100409112755.148:arGUIWindowManager::setUserData
//@+node:jimc.20100409112755.149:arGUIWindowManager::getGraphicsWindow

arGraphicsWindow* arGUIWindowManager::getGraphicsWindow( const int ID )
{
  return windowExists(ID) ? _windows[ ID ]->getGraphicsWindow() : NULL;
}
//@-node:jimc.20100409112755.149:arGUIWindowManager::getGraphicsWindow
//@+node:jimc.20100409112755.150:arGUIWindowManager::returnGraphicsWindow

void arGUIWindowManager::returnGraphicsWindow( const int ID )
{
  if (windowExists(ID))
    _windows[ ID ]->returnGraphicsWindow();
}
//@-node:jimc.20100409112755.150:arGUIWindowManager::returnGraphicsWindow
//@+node:jimc.20100409112755.151:arGUIWindowManager::setGraphicsWindow

void arGUIWindowManager::setGraphicsWindow( const int ID, arGraphicsWindow* graphicsWindow )
{
  if (windowExists(ID))
    _windows[ ID ]->setGraphicsWindow( graphicsWindow );
}
//@-node:jimc.20100409112755.151:arGUIWindowManager::setGraphicsWindow
//@+node:jimc.20100409112755.152:arGUIWindowManager::getWindowCursor

arCursor arGUIWindowManager::getWindowCursor( const int ID )
{
  return windowExists(ID) ?
    _windows[ ID ]->getCursor() : AR_CURSOR_NONE;
}
//@-node:jimc.20100409112755.152:arGUIWindowManager::getWindowCursor
//@+node:jimc.20100409112755.153:arGUIWindowManager::getMousePos

arVector3 arGUIWindowManager::getMousePos( const int ID )
{
  if (!windowExists(ID))
    return arVector3( -1.0f, -1.0f, -1.0f );

  const arGUIMouseInfo mouseState = _windows[ ID ]->getGUIEventManager()->getMouseState();
  return arVector3( float( mouseState.getPosX() ), float( mouseState.getPosY() ), 0.0f );
}
//@-node:jimc.20100409112755.153:arGUIWindowManager::getMousePos
//@+node:jimc.20100409112755.154:arGUIWindowManager::getKeyState

arGUIKeyInfo arGUIWindowManager::getKeyState( const arGUIKey /*key*/ )
{
#if defined( AR_USE_WIN_32 )
#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
#endif
  return arGUIKeyInfo();
}
//@-node:jimc.20100409112755.154:arGUIWindowManager::getKeyState
//@+node:jimc.20100409112755.155:arGUIWindowManager::setThreaded

void arGUIWindowManager::setThreaded( bool threaded ) {
  if (!hasActiveWindows()) {
    _threaded = threaded;
    arTexture_setThreaded(_threaded); // Wherever _threaded is set, propagate that to arTexture.
  }
}
//@-node:jimc.20100409112755.155:arGUIWindowManager::setThreaded
//@+node:jimc.20100409112755.156:arGUIWindowManager::_sendDeleteEvent

void arGUIWindowManager::_sendDeleteEvent( const int ID )
{
  arWMEvent* eventHandle = addWMEvent( ID, arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_CLOSE ) );
  if ( eventHandle ) {
    // if _threaded, return only after the window has processed the message.
    eventHandle->wait( true );
  }
}
//@-node:jimc.20100409112755.156:arGUIWindowManager::_sendDeleteEvent
//@+node:jimc.20100409112755.157:arGUIWindowManager::deleteWindow

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
//@-node:jimc.20100409112755.157:arGUIWindowManager::deleteWindow
//@+node:jimc.20100409112755.158:arGUIWindowManager::deleteAllWindows

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
//@-node:jimc.20100409112755.158:arGUIWindowManager::deleteAllWindows
//@+node:jimc.20100409112755.159:arGUIWindowManager::createWindows

int arGUIWindowManager::createWindows( const arGUIWindowingConstruct* windowingConstruct, bool useWindowing )
{
  if (!windowingConstruct)
    return -1;

  _windowingConstruct = const_cast<arGUIWindowingConstruct*>(windowingConstruct);

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
  std::vector< int >::iterator wItr; // don't confuse wItr with WItr

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
    if ( isStereo( ID ) != config->getStereo() ||
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
      if ( addWindow( *config, useWindowing ) < 0 ) {
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
    if ( deleteWindow( *wItr ) < 0 ) {
      ar_log_error() << "arGUIWindowManager failed to delete window: " << *wItr << ar_endl;
    }
  }

  // If there are more parsed than created windows, then create them.
  for( ; cItr != windowConstructs->end(); cItr++ ) {
    if ( addWindow( *((*cItr)->getWindowConfig()), useWindowing ) < 0 ) {
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
//@-node:jimc.20100409112755.159:arGUIWindowManager::createWindows
//@+node:jimc.20100409112755.160:arGUIWindowManager::useFramelock

// useFramelock and findFramelock should work from createWindows(),
// for the the checks in activateFramelock and deactivateFramelock.

void arGUIWindowManager::useFramelock( bool fUse )
{
  ar_useFramelock( fUse );
}
//@-node:jimc.20100409112755.160:arGUIWindowManager::useFramelock
//@+node:jimc.20100409112755.161:arGUIWindowManager::findFramelock

void arGUIWindowManager::findFramelock( void )
{
  ar_findFramelock();
}
//@-node:jimc.20100409112755.161:arGUIWindowManager::findFramelock
//@+node:jimc.20100409112755.162:arGUIWindowManager::activateFramelock

// Hack for Framelock graphics cards.
void arGUIWindowManager::activateFramelock( void )
{
  if (_fActivatedFramelock)
    return;
  _fActivatedFramelock = true;

  if ( _threaded ) {
    ar_log_error() << "arGUIWindowManager ignoring framelock for rendering threads.\n";
    return;
  }
  if ( _windows.size() > 1 ) {
    ar_log_error() << "arGUIWindowManager ignoring framelock for multiple windows.\n";
    return;
  }

  ar_activateFramelock();
  ar_log_debug() << "arGUIWindowManager activated framelock.\n";
}
//@-node:jimc.20100409112755.162:arGUIWindowManager::activateFramelock
//@+node:jimc.20100409112755.163:arGUIWindowManager::deactivateFramelock

void arGUIWindowManager::deactivateFramelock( void )
{
  if (!_fActivatedFramelock)
    return;
  _fActivatedFramelock = false;

  if ( _threaded ) {
    ar_log_error() << "arGUIWindowManager ignoring framelock for rendering threads.\n";
    return;
  }
  if ( _windows.size() > 1 ) {
    ar_log_error() << "arGUIWindowManager ignoring framelock for multiple windows.\n";
    return;
  }

  ar_deactivateFramelock();
  ar_log_debug() << "arGUIWindowManager deactivated framelock.\n";
}
//@-node:jimc.20100409112755.163:arGUIWindowManager::deactivateFramelock
//@-others
//@-node:jimc.20100409112755.102:@thin graphics\arGUIWindowManager.cpp
//@-leo
