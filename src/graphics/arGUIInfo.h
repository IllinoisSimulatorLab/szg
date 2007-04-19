//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Header file for the arGUIInfo, arGUIInfo, arGUIMouseInfo and arGUIWindowInfo classes.

#ifndef AR_GUI_INFO_H
#define AR_GUI_INFO_H

#include "arStructuredData.h"
#include "arGUIDefines.h"
#include "arGraphicsCalling.h"

// Forward declaration because, when an arGUIWindowManager owns a window,
// it puts its pointer into all arGUIInfo events.
class arGUIWindowManager;

// Info about an OS event.
//
// 'Abstract' base class returned by arGUIEventManager to arWindow and
// arWindowManager on requests for the next OS event.
//
// See arWindowManager::getNextWindowEvent, arWindow::getNextGUIEvent.

class SZG_CALL arGUIInfo
{
  public:

    // eventType Which type of event this is.
    // state     What state the event is in.
    // windowID  Which window this event took place in.
    // flag      Generic flag for function specific information.
    // userData  User-defined data pointer.
    arGUIInfo( arGUIEventType eventType = AR_GENERIC_EVENT,
               arGUIState state = AR_GENERIC_STATE,
               int windowID = -1, int flag = 0, void* userData = NULL );
    arGUIInfo( arStructuredData& data );

    // Accessors.
    void setEventType( arGUIEventType eventType ) { _eventType = eventType; }
    arGUIEventType getEventType( void ) const { return _eventType; }

    void setState( arGUIState state ) { _state = state; }
    arGUIState getState( void ) const { return _state; }

    void setWindowID( int windowID ) { _windowID = windowID; }
    int getWindowID( void ) const { return _windowID; }

    void setFlag( int flag ) { _flag = flag; }
    int getFlag( void ) const { return _flag; }

    void setUserData( void* userData ) { _userData = userData; }
    void* getUserData( void ) const { return _userData; }

    void setWindowManager( arGUIWindowManager* wm) { _wm = wm; }
    arGUIWindowManager* getWindowManager( void ) const { return _wm; }

  private:
    arGUIEventType _eventType;    // The type of event.
    arGUIState _state;            // The state of the event.
    int _windowID;                // Identifier for the window this event took place in.
    int _flag;                    // A generic flag, use is defined on a per-function basis.
    void* _userData;              // A user-defined data pointer, defined on a per window basis.
    arGUIWindowManager* _wm;      // Points back to the window manager that generated this event.
};

// Info about a keyboard event.
// See arGUIWindowManager::getKeyState, arGUIWindowManager::_keyboardHandler.

class SZG_CALL arGUIKeyInfo : public arGUIInfo
{
  public:

    // eventType Which type of event this is.
    // state     What state the event is in
    // windowID  Which window this event took place in.
    // flag      Generic flag for function specific information.
    // key       Which key was involved in this event.
    // ctrl      The state of the ctrl modifier.
    // alt       The state of the alt modifier.
    arGUIKeyInfo( arGUIEventType eventType = AR_KEY_EVENT,
                  arGUIState state = AR_GENERIC_STATE,
                  int windowID = -1, int flag = 0,
                  arGUIKey key = AR_VK_GARBAGE,
                  int ctrl = 0, int alt = 0 );
    arGUIKeyInfo( arStructuredData& data );

    // Accessors.
    void setKey( arGUIKey key ) { _key = key; }
    arGUIKey getKey( void ) const { return _key; }

    void setCtrl( int ctrl ) { _ctrl = ctrl; }
    int getCtrl( void ) const { return _ctrl; }

    void setAlt( int alt ) { _alt = alt; }
    int getAlt( void ) const { return _alt; }

  private:
    arGUIKey _key; // The key involved in this event.
    int _ctrl;     // The ctrl key modifier.
    int _alt;      // The alt key modifier.

    // Shift is not considered a modifier. All ascii
    // keypresses are translated to their shifted variants.
};

// Info about a mouse event.  See arGUIWindowManager::_mouseHandler.

class SZG_CALL arGUIMouseInfo : public arGUIInfo
{
  public:
    // eventType Which type of event this is.
    // state     What state the event is in
    // windowID  Which window this event took place in.
    // flag      Generic flag for function specific information.
    // button    Which mouse button was involved in this event.
    // posX      The current x position of the mouse.
    // posY      The current y position of the mouse.
    // prevPosX  The previous x position of the mouse.
    // prevPosY  The previous y position of the mouse.
    arGUIMouseInfo( arGUIEventType eventType = AR_MOUSE_EVENT,
                    arGUIState state = AR_GENERIC_STATE,
                    int windowID = -1, int flag = 0,
                    arGUIButton button = AR_BUTTON_GARBAGE,
                    int posX = -1, int posY = -1,
                    int prevPosX = -1, int prevPosY = -1 );
    arGUIMouseInfo( arStructuredData& data );

    // Accessors.
    void setButton( arGUIButton button ) { _button = button; }
    arGUIButton getButton( void ) const { return _button; }

    void setPosX( int posX ) { _posX = posX; }
    void setPosY( int posY ) { _posY = posY; }
    void setPos( int posX, int posY ) { _posX = posX; _posY = posY; }

    void setPrevPosX( int prevPosX ) { _prevPosX = prevPosX; }
    void setPrevPosY( int prevPosY ) { _prevPosY = prevPosY; }
    void setPrevPos( int prevPosX, int prevPosY ) { _prevPosX = prevPosX;
                                                    _prevPosY = prevPosY; }

    int getPosX( void ) const { return _posX; }
    int getPosY( void ) const { return _posY; }

    int getPrevPosX( void ) const { return _prevPosX; }
    int getPrevPosY( void ) const { return _prevPosY; }

  private:
    arGUIButton _button;    // Mouse button involved in this event.
    int _posX;              // Current x position of the mouse.
    int _posY;              // Current y position of the mouse.
    int _prevPosX;          // Previous x position of the mouse.
    int _prevPosY;          // Previous y position of the mouse.
};

// Info about a window event.  See arGUIWindowManager::_windowHandler.

class SZG_CALL arGUIWindowInfo : public arGUIInfo
{
  public:

    // eventType Which type of event this is.
    // state     What state the event is in
    // windowID  Which window this event took place in.
    // flag      Generic flag for function specific information.
    // posX      The x position of the window.
    // posY      The y position of the window.
    // sizeX     The width of the window.
    // sizeY     The height of the window.
    arGUIWindowInfo( arGUIEventType eventType = AR_WINDOW_EVENT,
                     arGUIState state = AR_GENERIC_STATE,
                     int windowID = -1, int flag = 0,
                     int posX = -1, int posY = -1,
                     int sizeX = -1, int sizeY = -1 );

    arGUIWindowInfo( arStructuredData& data );

    // Accessors.
    void setPosX( int posX ) { _posX = posX; }
    void setPosY( int posY ) { _posY = posY; }
    void setPos( int posX, int posY ) { _posX = posX; _posY = posY; }

    void setSizeX( int sizeX ) { _sizeX = sizeX; }
    void setSizeY( int sizeY ) { _sizeY = sizeY; }
    void setSize( int sizeX, int sizeY ) { _sizeX = sizeX; _sizeY = sizeY; }

    int getPosX( void ) const { return _posX; }
    int getPosY( void ) const { return _posY; }

    int getSizeX( void ) const { return _sizeX; }
    int getSizeY( void ) const { return _sizeY; }

  private:
    int _posX;      // Window's x position.
    int _posY;      // Window's y position.
    int _sizeX;     // Window's width.
    int _sizeY;     // Window's height.
};

#endif
