/**
 * @file arGUIEventManager.cpp
 * Implementation of the arGUIEventManager class.
 */
#include "arPrecompiled.h"

#include <iostream>
#include <vector>

#include "arStructuredData.h"

#include "arGUIEventManager.h"
#include "arGUIWindow.h"

arGUIEventManager::arGUIEventManager( void ) :
  _active( true )
{
  ar_mutex_init( &_eventsMutex );

  _GUIInfoDictionary = new arTemplateDictionary();

  // build the common GUI info dictionary
  _GUIInfoTemplate = new arDataTemplate( "GUIInfo" );
  _GUIInfoTemplate->add( "eventType", AR_INT );
  _GUIInfoTemplate->add( "state",     AR_INT );
  _GUIInfoTemplate->add( "windowID",  AR_INT );
  _GUIInfoTemplate->add( "flag",      AR_INT );

  // now add the pieces exclusive to each info subtype
  _keyInfoTemplate = new arDataTemplate( *_GUIInfoTemplate );
  _keyInfoTemplate->setName( "keyInfo" );
  _keyInfoTemplate->add( "key",   AR_INT );
  _keyInfoTemplate->add( "alt",   AR_INT );
  _keyInfoTemplate->add( "ctrl",  AR_INT );

  _mouseInfoTemplate = new arDataTemplate( *_GUIInfoTemplate );
  _mouseInfoTemplate->setName( "mouseInfo" );
  _mouseInfoTemplate->add( "button",   AR_INT );
  _mouseInfoTemplate->add( "posX",     AR_INT );
  _mouseInfoTemplate->add( "posY",     AR_INT );
  _mouseInfoTemplate->add( "prevPosX", AR_INT );
  _mouseInfoTemplate->add( "prevPosY", AR_INT );

  _windowInfoTemplate = new arDataTemplate( *_GUIInfoTemplate );
  _windowInfoTemplate->setName( "windowInfo" );
  _windowInfoTemplate->add( "sizeX", AR_INT );
  _windowInfoTemplate->add( "sizeY", AR_INT );
  _windowInfoTemplate->add( "posX",  AR_INT );
  _windowInfoTemplate->add( "posY",  AR_INT );

  _eventIDs[ AR_GENERIC_EVENT ] = _GUIInfoDictionary->add( _GUIInfoTemplate );
  _eventIDs[ AR_KEY_EVENT ]     = _GUIInfoDictionary->add( _keyInfoTemplate );
  _eventIDs[ AR_MOUSE_EVENT ]   = _GUIInfoDictionary->add( _mouseInfoTemplate );
  _eventIDs[ AR_WINDOW_EVENT ]  = _GUIInfoDictionary->add( _windowInfoTemplate );
}

arGUIEventManager::~arGUIEventManager( void )
{
  delete _GUIInfoDictionary;

  delete _GUIInfoTemplate;
  delete _keyInfoTemplate;
  delete _mouseInfoTemplate;
  delete _windowInfoTemplate;
}

bool arGUIEventManager::eventsPending( void )
{
  ar_mutex_lock( &_eventsMutex );

  if( _events.empty() ) {
    ar_mutex_unlock( &_eventsMutex );
    return false;
  }
  else {
    ar_mutex_unlock( &_eventsMutex );
    return true;
  }
}

arGUIInfo* arGUIEventManager::getNextEvent( void )
{
  ar_mutex_lock( &_eventsMutex );

  if( _events.empty() ) {
    ar_mutex_unlock( &_eventsMutex );
    return NULL;
  }

  arStructuredData event = _events.front();
  _events.pop();

  ar_mutex_unlock( &_eventsMutex );

  arGUIEventType eventType = arGUIEventType( event.getDataInt( event.getDataFieldIndex( "eventType" ) ) );

  // NOTE: caller (normally arGUIWindowManager::_processEvents) is responsible for
  // deleting these
  if( eventType == AR_KEY_EVENT ) {
    return new arGUIKeyInfo( event );
  }
  else if( eventType == AR_MOUSE_EVENT ) {
    return new arGUIMouseInfo( event );
  }
  else if( eventType == AR_WINDOW_EVENT ) {
    return new arGUIWindowInfo( event );
  }
  else {
    // print a warning? (there probably aren't any cases where this is what the
    // user is actually expecting)
    return new arGUIInfo( event );
  }
}

int arGUIEventManager::consumeEvents( arGUIWindow* window, const bool blocking )
{
  if( !_active ) {
    return -1;
  }

  #if defined( AR_USE_WIN_32 )

  MSG msg;

  if( blocking ) {
    if( !WaitMessage() ) {
      std::cerr << "consumeEvents: WaitMessage error" << std::endl;
    }
  }

  while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )  {
    int getResult = GetMessage( &msg, NULL, 0, 0 );

    if( getResult == -1 ) {
      std::cerr << "consumeEvents: GetMessage error" << std::endl;
      return -1;
    }
    else {
      // TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
  }

  #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  XEvent event;

  // OS X seems to have an issue with X11 "unexpected async reply" errors
  // because of some multithreading issues, so we lock/unlock the display
  // to mitigate the problem
  XLockDisplay( window->getWindowHandle()._dpy );

  if( blocking ) {
    XPeekEvent( window->getWindowHandle()._dpy, &event );
  }

  while( XPending( window->getWindowHandle()._dpy ) ) {
    memset( &event, 0, sizeof( XEvent ) );
    XNextEvent( window->getWindowHandle()._dpy, &event );

    switch( event.type ) {
      case ClientMessage:
        if( (Atom) event.xclient.data.l[ 0 ] == window->getWindowHandle()._wDelete ) {
          int posX = _windowState._posX;
          int posY = _windowState._posY;

          int sizeX = _windowState._sizeX;
          int sizeY = _windowState._sizeY;

          if( addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_CLOSE, window->getID(), 0, posX, posY, sizeX, sizeY ) ) < 0 ) {
            // print an error
          }
        }
      break;

      case CreateNotify:
      case ConfigureNotify:
      {
        int posX = event.xconfigure.x;
        int posY = event.xconfigure.y;

        int sizeX = event.xconfigure.width;
        int sizeY = event.xconfigure.height;

        // ConfigureNotify's are passed on both window move's and resize's,
        // it's up to us to figure out which one it was, if x and y are both
        // 0 then this is actually a resize event, however it can be both a
        // resize *and* a move simultaneously if the upper left corner of the
        // window is dragged (drugged?)
        if( ( posX != _windowState._posX || posY != _windowState._posY ) &&
            ( posX != 0 || posY != 0 ) ) {
          if( addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_MOVE, window->getID(), 0, posX, posY, sizeX, sizeY ) ) < 0 ) {
            // print an error?
          }
        }

        if( ( sizeX != _windowState._sizeX || sizeY != _windowState._sizeY  ) &&
            ( posX == 0 && posY == 0 ) ) {
          posX = _windowState._posX;
          posY = _windowState._posY;

          if( addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, AR_WINDOW_RESIZE, window->getID(), 0, posX, posY, sizeX, sizeY ) ) < 0 ) {
            // print an error?
          }
        }
      }
      break;

      case DestroyNotify:
      break;

      case Expose:
      break;

      case MapNotify:
      case UnmapNotify:
      break;

      case MappingNotify:
        // update the client's knowledge of the keyboard
        XRefreshKeyboardMapping( (XMappingEvent *) &event );
      break;

      case VisibilityNotify:
        switch( event.xvisibility.state ) {
          case VisibilityUnobscured:
            window->setVisible( true );
          break;

          case VisibilityPartiallyObscured:
            window->setVisible( true );
          break;

          case VisibilityFullyObscured:
            window->setVisible( false );
          break;

          default:
          break;
        }
      break;

      case EnterNotify:
      case LeaveNotify:
        // call mouse callback?
      break;

      case MotionNotify:
      {
        int posX = event.xmotion.x;
        int posY = event.xmotion.y;

        int prevPosX = _mouseState._posX;
        int prevPosY = _mouseState._posY;

        arGUIButton button = _mouseState._button;
        arGUIState state = AR_GENERIC_STATE;

        if( button ) {
          state = AR_MOUSE_DRAG;
        }
        else {
          state = AR_MOUSE_MOVE;
        }

        if( addEvent( arGUIMouseInfo( AR_MOUSE_EVENT, state, window->getID(), 0, button, posX, posY, prevPosX, prevPosY ) ) < 0 ) {
          // print an error?
        }
      }
      break;

      case ButtonRelease:
      case ButtonPress:
      {
        int posX = event.xbutton.x;
        int posY = event.xbutton.y;

        int prevPosX = _mouseState._posX;
        int prevPosY = _mouseState._posY;

        arGUIButton button = AR_BUTTON_GARBAGE;
        arGUIState state = AR_GENERIC_STATE;

        if( event.type == ButtonRelease ) {
          state = AR_MOUSE_UP;
        }
        else {
          state = AR_MOUSE_DOWN;
        }

        switch( event.xbutton.button ) {
          case Button1:
            button = AR_LBUTTON;
          break;
          case Button2:
            button = AR_MBUTTON;
          break;
          case Button3:
            button = AR_RBUTTON;
          break;
          default:
          break;
        }

        if( addEvent( arGUIMouseInfo( AR_MOUSE_EVENT, state, window->getID(), 0, button, posX, posY, prevPosX, prevPosY ) ) < 0 ) {
          // print an error?
        }
      }
      break;

      case KeyRelease:
      case KeyPress:
      {
        XComposeStatus composeStatus;
        char asciiCode[ 32 ] = { 0 }, keys[ 32 ] = { 0 };
        KeySym keySym;
        int len;

        // check the state of the modifiers
        // NOTE: the masks used below /may/ vary between X versions and/or WM's
        // freeglut makes the assumption they won't, but SDL has code for
        // determining the "right" mask to use
        int ctrl = 0, alt = 0;

        if( event.xkey.state & ControlMask ) {
          ctrl = 1;
        }

        // there's a weird issue on slackware 10 + XFree86 6.7 + KDE 3.2.3 with
        // the right alt key showing up as the correct keycode but getting
        // converted to the XK_ISO_Level3_Shift keysym, no idea why...
        if( event.xkey.state & Mod1Mask ) {
          alt = 1;
        }

        arGUIKey key = AR_VK_GARBAGE;

        // try to translate to ascii
        len = XLookupString( &event.xkey, asciiCode, sizeof( asciiCode ),
                             &keySym, &composeStatus );

        // ctrl'ed ascii keys are in fact translated, but we want to tease out
        // ctrl as a standalone modifier, also make sure only "true" ASCII keys
        // are being converted
        if( len > 0 && !ctrl && keySym > 31 && keySym < 127 ) {
          key = arGUIKey( asciiCode[ 0 ] );
        }
        else {
          key = arGUIKey( keySym );
        }

        arGUIState state = AR_GENERIC_STATE;

        KeyboardIterator kitr;

        if( event.type == KeyRelease ) {
          XQueryKeymap( window->getWindowHandle()._dpy, keys );

           if( event.xkey.keycode < 256 ) {
             if( keys[ event.xkey.keycode >> 3 ] & ( 1 << ( event.xkey.keycode % 8 ) ) ) {
               // ignore the KeyRelease of the KeyPress/KeyRelease pair
               // NOTE: some key press (but not key release) events that have
               // both ctrl and alt set seem to get eaten by the OS if they are
               // shortcuts, (e.g. ctrl+alt+a but not ctrl+alt+0), resulting in
               // the user only ever seeing one AR_KEY_UP (but not even a prior
               // AR_KEY_DOWN) for ctrl+alt+a when it is actually released, but
               // ctrl+alt+0 exhibits the expected, proper behavior
               continue;
             }
             else {
               state = AR_KEY_UP;
             }
           }
        }
        else if( ( kitr = _keyboardState.find( key ) ) != _keyboardState.end() &&
              (kitr->second)._state != AR_KEY_UP ) {
          // ARGH, _only_ place we need to use recorded state, Windows has a
          // flag set in the message for determining repeatedness, can't find
          // a similar struct in Linux, but that doesnt mean it doesnt exist
          state = AR_KEY_REPEAT;
        }
        else {
          state = AR_KEY_DOWN;
        }

        if( addEvent( arGUIKeyInfo( AR_KEY_EVENT, state, window->getID(), 0, key, ctrl, alt ) ) < 0 ) {
          // print an error?
        }
      }
      break;

      case ReparentNotify:
      break;

      default:
       // print warning?
      break;
    }
  }

  XUnlockDisplay( window->getWindowHandle()._dpy );

  #endif

  return 0;
}

int arGUIEventManager::addEvent( arStructuredData& event )
{
  if( !_active ) {
    return -1;
  }

  // carry out any pre-processing on the event before it is added
  // FIXME: should be checking the returns of getDataFieldIndex
  if( event.getDataInt( event.getDataFieldIndex( "eventType" ) ) == AR_KEY_EVENT ) {
    int key = event.getDataInt( event.getDataFieldIndex( "key" ) );

    if( key < 0 ) {
      std::cerr << "addEvent: getDataInt error" << std::endl;
      return -1;
    }

    // record the effect of this event on the state of the keyboard
    _keyboardState[ key ] = arGUIKeyInfo( event );

    // don't pass on shift, ctrl, or alt key presses as standalone events
    // AARGH, windows and linux don't agree on this, left vs. right isn't
    // differentiated in the keycode under windows, but it is in linux, but
    // since they aren't bit masks there doesn't seem to be a cross-platform
    // way of doing this
    #if defined( AR_USE_WIN_32 )
    if( key == AR_VK_SHIFT || key == AR_VK_CTRL || key == AR_VK_ALT ) {
      return 0;
    }
    #elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )
    if( key == AR_VK_LSHIFT || key == AR_VK_RSHIFT || key == AR_VK_LCTRL ||
        key == AR_VK_RCTRL || key == AR_VK_LALT || key == AR_VK_RALT ) {
      return 0;
    }
    #endif
  }
  else if( event.getDataInt( event.getDataFieldIndex( "eventType" ) ) == AR_MOUSE_EVENT ) {
    arGUIState state = arGUIState( event.getDataInt( event.getDataFieldIndex( "state" ) ) );
    arGUIButton button = arGUIButton( event.getDataInt( event.getDataFieldIndex( "button" ) ) );

    arGUIButton totalButtons = _mouseState._button;

    // "add" on to the mouse state which button is now down, or "subtract" off
    // which button is now up so that the mouse callback can do boolean comparisons
    if( state == AR_MOUSE_DOWN ) {
      totalButtons = button | _mouseState._button;
    }
    else if( state == AR_MOUSE_UP ) {
      totalButtons = ~button & _mouseState._button;
    }

    _mouseState = arGUIMouseInfo( event );

    _mouseState._button = totalButtons;
  }
  else if( event.getDataInt( event.getDataFieldIndex( "eventType" ) ) == AR_WINDOW_EVENT ) {
    _windowState = arGUIWindowInfo( event );
  }
  else {
    return -1;
  }

  ar_mutex_lock( &_eventsMutex );
  _events.push( event );
  ar_mutex_unlock( &_eventsMutex );

  return 0;
}

int arGUIEventManager::addEvent( const arGUIKeyInfo& keyInfo )
{
  arStructuredData keyInfoSD( _keyInfoTemplate );

  if( !keyInfoSD.dataIn( "eventType", &keyInfo._eventType, AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "state",     &keyInfo._state,     AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "windowID",  &keyInfo._windowID,  AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "flag",      &keyInfo._flag,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "key",       &keyInfo._key,       AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "ctrl",      &keyInfo._ctrl,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !keyInfoSD.dataIn( "alt",       &keyInfo._alt,       AR_INT, 1 ) ) {
    return -1;
  }

  if( addEvent( keyInfoSD ) < 0 ) {
    return -1;
  }

  return 0;
}

int arGUIEventManager::addEvent( const arGUIMouseInfo& mouseInfo )
{
  arStructuredData mouseInfoSD( _mouseInfoTemplate );

  if( !mouseInfoSD.dataIn( "eventType", &mouseInfo._eventType, AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "state",     &mouseInfo._state,     AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "windowID",  &mouseInfo._windowID,  AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "flag",      &mouseInfo._flag,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "button",    &mouseInfo._button,    AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "posX",      &mouseInfo._posX,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "posY",      &mouseInfo._posY,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "prevPosX",  &mouseInfo._prevPosX,  AR_INT, 1 ) ) {
    return -1;
  }

  if( !mouseInfoSD.dataIn( "prevPosY",  &mouseInfo._prevPosY,  AR_INT, 1 ) ) {
    return -1;
  }

  if( addEvent( mouseInfoSD ) < 0 ) {
    return -1;
  }

  return 0;
}

int arGUIEventManager::addEvent( const arGUIWindowInfo& windowInfo )
{
  arStructuredData windowInfoSD( _windowInfoTemplate );

  if( !windowInfoSD.dataIn( "eventType", &windowInfo._eventType, AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "state",     &windowInfo._state,     AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "windowID",  &windowInfo._windowID,  AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "flag",      &windowInfo._flag,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "posX",      &windowInfo._posX,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "posY",      &windowInfo._posY,      AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "sizeX",     &windowInfo._sizeX,     AR_INT, 1 ) ) {
    return -1;
  }

  if( !windowInfoSD.dataIn( "sizeY",     &windowInfo._sizeY,     AR_INT, 1 ) ) {
    return -1;
  }

  if( addEvent( windowInfoSD ) < 0 ) {
    return -1;
  }

  return 0;
}

#if defined( AR_USE_WIN_32 )
LRESULT CALLBACK arGUIEventManager::windowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  if( uMsg == WM_NCCREATE ) {
    // store the pointer to the arGUIWindow
    // see note in MSDN about how this function's possible return values
    // interact with GetLastError()
    SetLastError( 0 );
    if( !SetWindowLong( hWnd, GWL_USERDATA, (LONG) ((LPCREATESTRUCT) lParam)->lpCreateParams ) ) {
      if( GetLastError() ) {
        std::cerr << "windowProc: SetWindowLong error" << std::endl;
        return -1;
      }
    }
  }
  else {
    arGUIWindow* currentWindow = (arGUIWindow*) GetWindowLong( hWnd, GWL_USERDATA );

    if( currentWindow && currentWindow->running() && currentWindow->getGUIEventManager()->isActive() ) {
      return currentWindow->getGUIEventManager()->windowProcCB( hWnd, uMsg, wParam, lParam, currentWindow );
    }
  }

  return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

LRESULT CALLBACK arGUIEventManager::windowProcCB( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, arGUIWindow* window )
{
  switch( uMsg )
  {
    case WM_SYSCOMMAND:
      switch( wParam )
      {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
          // prevent screensaver and power saver modes
          return 0;
        break;

        case SC_MINIMIZE:
          window->setVisible( false );
        break;
      }
    break;

    case WM_SHOWWINDOW:
      window->setVisible( true );
    break;

    case WM_QUIT:
      // reserved for closing the current window and reopening it with
      // different parameters
      return 0;
    break;

    case WM_SIZE:
    {
      switch( wParam )
      {
        case SIZE_MINIMIZED:
          window->setVisible( false );
        break;

        case SIZE_MAXIMIZED:
        case SIZE_RESTORED:
          window->setVisible( true );
        break;

        default:
          // better safe than sorry
          window->setVisible( true );
        break;
      }
    }
    case WM_CLOSE:
    case WM_MOVE:
    {
      int posX = window->getPosX();
      int posY = window->getPosY();
      int sizeX = window->getWidth();
      int sizeY = window->getHeight();

      arGUIState state = AR_GENERIC_STATE;

      if( uMsg == WM_MOVE ) {
        posX = (int)(short) LOWORD( lParam );
        posY = (int)(short) HIWORD( lParam );

        state = AR_WINDOW_MOVE;
      }
      else if( uMsg == WM_SIZE ) {
        sizeX = (int)(short) LOWORD( lParam );
        sizeY = (int)(short) HIWORD( lParam );

        state = AR_WINDOW_RESIZE;
      }
      else if( uMsg == WM_CLOSE ) {
        state = AR_WINDOW_CLOSE;
      }

      // dummy move and/or resize message from a minimization
      if( ( ( sizeX == 0 ) && ( sizeY == 0 ) ) ||
          ( ( posX < 0 ) && ( posY < 0 ) ) ) {
        return 0;
      }

      // could also check if the current x/y, width/height are the exact same
      // as the new ones, probably don't need to send the message in that case

      if( addEvent( arGUIWindowInfo( AR_WINDOW_EVENT, state, window->getID(), 0, posX, posY, sizeX, sizeY ) ) < 0 ) {
        // print an error?
      }

      return 0;
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      // check the state of the modifiers
      int ctrl = 0, alt = 0;

      if( GetKeyState( AR_VK_CTRL ) < 0 ) {
        ctrl = 1;
      }

      if( GetKeyState( AR_VK_ALT ) < 0 ) {
        alt = 1;
      }

      BYTE kstate[ 256 ] = { 0 };
      WORD code[ 2 ] = { 0 };
      int len;

      if( !GetKeyboardState( kstate ) ) {
        std::cerr << "WindowProcCB: GetKeyboardState error" << std::endl;
        return -1;
      }

      // ToAscii will take into account whether a ctrl key is down
      // when translating (i.e. turning them into control codes), but
      // we don't want that so strip out the ctrl
      if( ctrl ) {
        kstate[ AR_VK_CTRL ] &= 0x0F;
      }

      // try to translate to ascii
      len = ToAscii( wParam, 0, kstate, code, 0 );

      if( len > 0 && code[ 0 ] > 31 && code[ 0 ] < 127 ) {
        wParam = (char) code[ 0 ];
      }

      arGUIState state = AR_GENERIC_STATE;

      if( uMsg == WM_SYSKEYUP || uMsg == WM_KEYUP ) {
        state = AR_KEY_UP;
      }
      else if( HIWORD( lParam ) & KF_REPEAT ) {
        state = AR_KEY_REPEAT;
      }
      else {
        state = AR_KEY_DOWN;
      }

      if( addEvent( arGUIKeyInfo( AR_KEY_EVENT, state, window->getID(), 0, arGUIKey( wParam ), ctrl, alt ) ) < 0 ) {
        // print an error?
      }

      return 0;
    }
    break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
      int posX = LOWORD( lParam );
      int posY = HIWORD( lParam );

      int prevPosX = _mouseState._posX;
      int prevPosY = _mouseState._posY;


      // Match X11 behavior by restricting to [ -32768, 32767 ]
      if( posX > 32767 ) {
        posX -= 65536;
      }
      if( posY > 32767 ) {
        posY -= 65536;
      }

      // do a quick check before we do any more work (seems to only be a
      // problem on the windows side)
      if( uMsg == WM_MOUSEMOVE && posX == prevPosX && posY == prevPosY ) {
        return -1;
      }

      arGUIButton button = AR_BUTTON_GARBAGE;
      arGUIState state = AR_GENERIC_STATE;

      switch( uMsg )
      {
        case WM_MOUSEMOVE:
          button = _mouseState._button;

          if( button ) {
            state = AR_MOUSE_DRAG;
          }
          else {
            state = AR_MOUSE_MOVE;
          }
        break;

        case WM_LBUTTONDOWN:
          button = AR_LBUTTON;
          state = AR_MOUSE_DOWN;
        break;

        case WM_LBUTTONUP:
          button = AR_LBUTTON;
          state = AR_MOUSE_UP;
       break;

        case WM_MBUTTONDOWN:
          button = AR_MBUTTON;
          state = AR_MOUSE_DOWN;
        break;

        case WM_MBUTTONUP:
          button = AR_MBUTTON;
          state = AR_MOUSE_UP;
        break;

        case WM_RBUTTONDOWN:
          button = AR_RBUTTON;
          state = AR_MOUSE_DOWN;
        break;

       case WM_RBUTTONUP:
          button = AR_RBUTTON;
          state = AR_MOUSE_UP;
        break;

        default:
          button = AR_BUTTON_GARBAGE;
          state = AR_GENERIC_STATE;
        break;
      }

      // check for button swaps (left to right, right to left)

      // attempt to match X11 behavior by capturing the mouse if a button was
      // pressed and held inside the window, and releasing once /all/ the
      // buttons have been released.
      if( state == AR_MOUSE_DOWN ) {
        SetCapture( window->getWindowHandle()._hWnd );
      }
      else if( !_mouseState._button && ( state == AR_MOUSE_UP ) ) {
        ReleaseCapture();
      }

      if( addEvent( arGUIMouseInfo( AR_MOUSE_EVENT, state, window->getID(), 0, button, posX, posY, prevPosX, prevPosY ) ) < 0 ) {
        // print an error?
      }

      return 0;
    }
    break;
  }

  // pass unhandled messages to DefWindowProc
  return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
#endif

void arGUIEventManager::setActive( const bool active )
{
  // just to be on the safe side...
  ar_mutex_lock( &_eventsMutex );
  _active = active;
  ar_mutex_unlock( &_eventsMutex );
}

arGUIKeyInfo arGUIEventManager::getKeyState( const arGUIKey key )
{
  return _keyboardState[ key ];
}

arGUIMouseInfo arGUIEventManager::getMouseState( void ) const
{
  return _mouseState;
}

arGUIWindowInfo arGUIEventManager::getWindowState( void ) const
{
  return _windowState;
}
