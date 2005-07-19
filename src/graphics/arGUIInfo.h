//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

/**
 * @file arGUIInfo.h
 * Header file for the arGUIInfo, arGUIInfo, arGUIMouseInfo and arGUIWindowInfo classes.
 */
#ifndef AR_GUI_INFO_H
#define AR_GUI_INFO_H

#include "arStructuredData.h"
#include "arGUIDefines.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/**
 * Information about an OS event.
 *
 * 'Abstract' base class returned by arGUIEventManager to arWindow and
 * arWindowManager on requests for the next OS event.
 *
 * @see arWindowManager::getNextWindowEvent
 * @see arWindow::getNextGUIEvent
 */
class SZG_CALL arGUIInfo
{
  public:

    /**
     * The arGUIInfo default constructor.
     *
     * @param eventType Which type of event this is.
     * @param state     What state the event is in.
     * @param windowID  Which window this event took place in.
     * @param flag      Generic flag for function specific information.
     * @param userData  User-defined data pointer.
     */
    arGUIInfo( arGUIEventType eventType = AR_GENERIC_EVENT,
               arGUIState state = AR_GENERIC_STATE,
               int windowID = -1, int flag = 0, void* userData = NULL );

    /**
     * An arGUIInfo constructor.
     *
     * @param data An arStructuredData record from which to create an arGUIInfo
     *             object.
     */
    arGUIInfo( arStructuredData& data );

    /**
     * The arGUIInfo destructor.
     */
    ~arGUIInfo( void );

    //@{
    /** @name arGUIInfo state accessors.
     *
     * Retrieve or set an arGUIInfo's state.
     */
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
    //@}

  private:
    /// variables common to all events

    arGUIEventType _eventType;    ///< The type of event.
    arGUIState _state;            ///< The state of the event.

    int _windowID;                ///< Identifier for the window this event took place in.

    int _flag;                    ///< A generic flag, use is defined on a per-function basis.

    void* _userData;              ///< A user-defined data pointer, defined on a per window basis.
};

/**
 * Information about a keyboard event.
 *
 * @see arGUIInfo
 * @see arGUIWindowManager::getKeyState
 * @see arGUIWindowManager::_keyboardHandler
 */
class SZG_CALL arGUIKeyInfo : public arGUIInfo
{
  public:

    /**
     * The arGUIKeyInfo default constructor.
     *
     * @param eventType Which type of event this is.
     * @param state     What state the event is in
     * @param windowID  Which window this event took place in.
     * @param flag      Generic flag for function specific information.
     * @param key       Which key was involved in this event.
     * @param ctrl      The state of the ctrl modifier.
     * @param alt       The state of the alt modifier.
     */
    arGUIKeyInfo( arGUIEventType eventType = AR_KEY_EVENT,
                  arGUIState state = AR_GENERIC_STATE,
                  int windowID = -1, int flag = 0,
                  arGUIKey key = AR_VK_GARBAGE,
                  int ctrl = 0, int alt = 0 );

    /**
     * An arGUIKeyInfo constructor.
     *
     * @param data An arStructuredData record from which to create an
     *             arGUIKeyInfo object.
     */
    arGUIKeyInfo( arStructuredData& data );

    /**
     * The arGUIKeyInfo destructor.
     */
    ~arGUIKeyInfo( void );

    //@{
    /** @name arGUIKeyInfo state accessors.
     *
     * Retrieve or set an arGUIKeyInfo's state.
     */
    void setKey( arGUIKey key ) { _key = key; }
    arGUIKey getKey( void ) const { return _key; }

    void setCtrl( int ctrl ) { _ctrl = ctrl; }
    int getCtrl( void ) const { return _ctrl; }

    void setAlt( int alt ) { _alt = alt; }
    int getAlt( void ) const { return _alt; }
    //@}

  private:

    arGUIKey _key;      ///< The key involved in this event.

    int _ctrl;          ///< The ctrl key modifier.
    int _alt;           ///< The alt key modifier.

    /// By design shift is not considered a modifier, all ascii
    /// keypresses are translated to their shift'ed variants.
};

/**
 * Information about a mouse event.
 *
 * @see arGUIInfo
 * @see arGUIWindowManager::_mouseHandler
 */
class SZG_CALL arGUIMouseInfo : public arGUIInfo
{
  public:

    /**
     * The arGUIMouseInfo default constructor.
     *
     * @param eventType Which type of event this is.
     * @param state     What state the event is in
     * @param windowID  Which window this event took place in.
     * @param flag      Generic flag for function specific information.
     * @param button    Which mouse button was involved in this event.
     * @param posX      The current x position of the mouse.
     * @param posY      The current y position of the mouse.
     * @param prevPosX  The previous x position of the mouse.
     * @param prevPosY  The previous y position of the mouse.
     */
    arGUIMouseInfo( arGUIEventType eventType = AR_MOUSE_EVENT,
                    arGUIState state = AR_GENERIC_STATE,
                    int windowID = -1, int flag = 0,
                    arGUIButton button = AR_BUTTON_GARBAGE,
                    int posX = -1, int posY = -1,
                    int prevPosX = -1, int prevPosY = -1 );

    /**
     * An arGUIMouseInfo constructor.
     *
     * @param data An arStructuredData record from which to create an
     *             arGUIMouseInfo object.
     */
    arGUIMouseInfo( arStructuredData& data );

    /**
     * The arGUIMouseInfo destructor.
     */
    ~arGUIMouseInfo( void );

    //@{
    /** @name arGUIMouseInfo state accessors.
     *
     * Retrieve or set an arGUIMouseInfo's state.
     */
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
    //@}

  private:

    arGUIButton _button;    ///< The mouse button involved in this event.

    int _posX;              ///< The current x position of the mouse.
    int _posY;              ///< The current y position of the mouse.
    int _prevPosX;          ///< The previous x position of the mouse.
    int _prevPosY;          ///< The previous y position of the mouse.
};

/**
 * Information about a window event.
 *
 * @see arGUIInfo
 * @see arGUIWindowManager::_windowHandler
 */
class SZG_CALL arGUIWindowInfo : public arGUIInfo
{
  public:

    /**
     * The arGUIMouseInfo default constructor.
     *
     * @param eventType Which type of event this is.
     * @param state     What state the event is in
     * @param windowID  Which window this event took place in.
     * @param flag      Generic flag for function specific information.
     * @param posX      The x position of the window.
     * @param posY      The y position of the window.
     * @param sizeX     The width of the window.
     * @param sizeY     The height of the window.
     */
    arGUIWindowInfo( arGUIEventType eventType = AR_WINDOW_EVENT,
                     arGUIState state = AR_GENERIC_STATE,
                     int windowID = -1, int flag = 0,
                     int posX = -1, int posY = -1,
                     int sizeX = -1, int sizeY = -1 );

    /**
     * An arGUIWindowInfo constructor.
     *
     * @param data An arStructuredData record from which to create an
     *             arGUIWindowInfo object.
     */
    arGUIWindowInfo( arStructuredData& data );

    /**
     * The arGUIWindowInfo destructor.
     */
    ~arGUIWindowInfo( void );

    //@{
    /** @name arGUIWindowInfo state accessors.
     *
     * Retrieve or set an arGUIWindowInfo's state.
     */
    void setPosX( int posX ) { _posX = posX; }
    void setPosY( int posY ) { _posY = posY; }
    void setPos( int posX, int posY ) { _posX = posX; _posY = posY; }

    void setSizeX( int sizeX ) { _sizeX = sizeX; }
    void setSizeY( int sizeY ) { _sizeY = sizeY; }
    void setSize( int sizeX, int sizeY ) { _sizeX = sizeX;
                                           _sizeY = sizeY; }

    int getPosX( void ) const { return _posX; }
    int getPosY( void ) const { return _posY; }

    int getSizeX( void ) const { return _sizeX; }
    int getSizeY( void ) const { return _sizeY; }
    //@}

  private:
    int _posX;      ///< The x position of the window.
    int _posY;      ///< The y position of the window.
    int _sizeX;     ///< The width of the window.
    int _sizeY;     ///< The height of the window.
};


#endif

