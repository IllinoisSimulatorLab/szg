/**
 * @file arGUIInfo.h
 * Header file for the arGUIInfo, arGUIInfo, arGUIMouseInfo and arGUIWindowInfo classes.
 */
#ifndef _AR_GUI_INFO_H_
#define _AR_GUI_INFO_H_

#include "arStructuredData.h"

#include "arGUIDefines.h"

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
     */
    arGUIInfo( arGUIEventType eventType = AR_GENERIC_EVENT,
               arGUIState state = AR_GENERIC_STATE,
               int windowID = -1, int flag = 0 );

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

  public:

    // variables common to all events

    arGUIEventType _eventType;    ///< The type of event.
    arGUIState _state;            ///< The state of the event.

    int _windowID;                ///< Identifier for the window this event took place in.

    int _flag;                    ///< A generic flag, use is defined on a per-function basis.
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

  public:

    arGUIKey _key;      ///< The key involved in this event.

    int _ctrl;          ///< The ctrl key modifier.
    int _alt;           ///< The alt key modifier.

    // By design shift is not considered a modifier, all ascii
    // keypresses are translated to their shift'ed variants.
};

/**
 * Information about a keyboard event.
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

  public:

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

  public:

    int _posX;      ///< The x position of the window.
    int _posY;      ///< The y position of the window.
    int _sizeX;     ///< The width of the window.
    int _sizeY;     ///< The height of the window.
};


#endif

