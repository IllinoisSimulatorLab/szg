//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GUI_EVENT_MANAGER_H
#define AR_GUI_EVENT_MANAGER_H

#if defined( AR_USE_WIN_32 )
  #include "arPrecompiled.h"
#elif defined( AR_USE_LINUX )
#elif defined( AR_USE_DARWIN )
#elif defined( AR_USE_SGI )
#endif

#include "arGUIDefines.h"
#include "arGUIInfo.h"
#include "arGraphicsCalling.h"

#include <queue>

class arStructuredData;
class arTemplateDictionary;
class arDataTemplate;
class arGUIWindow;

//@{
/**
 * @name Convenience typedef's for arGUIEventManager functions.
 * @todo Move these to a common location with other typedefs.
 */
typedef std::map<arGUIKey, arGUIKeyInfo > KeyboardMap;

typedef KeyboardMap::iterator KeyboardIterator;
//@}

/**
 * Consumes pending OS events for a specific window and packages them into a
 * form usable by the event callbacks in arGUIWindowManager.  Can be controlled
 * externally by being fed manufactured OS events.
 *
 * @see arGUIWindow
 * @see arGUIWindow::_consumeWindowEvents
 * @see arGUIWindow::getNextGUIEvent
 */
class SZG_CALL arGUIEventManager
{
  public:

    /**
     * The arGUIEventManager constructor.
     *
     * @param userData User-defined data pointer set for all OS events.
     */
    arGUIEventManager( void* userData = NULL );

    /**
     * The arGUIEventManager destructor.
     */
    ~arGUIEventManager( void );

    /**
     * Consume any pending OS events for an arGUIWindow.
     *
     * @note Although this function takes an arGUIWindow pointer, it operates
     *       under the assumption it will always be passed the <b>same</b>
     *       window, if not then held state could easily be bogus.
     *
     * @param window   The window to process events on.
     * @param blocking If true and there are no pending events, wait in this
     *                 function until there are pending events, process them,
     *                 and only then return.
     *
     * @see arGUIWindow::_consumeWindowEvents
     */
    int consumeEvents( arGUIWindow* window, const bool blocking = true );

    /** @anchor addEventBase
     * Add an OS event to the queue of events the arGUIWindowManager has yet to
     * process from this window.
     *
     * @note To-be-added events <b>must</b> pass through this function before
     *       they are added to the queue as this function does some necessary
     *       pre-processing on them.
     *
     * @note The OS event added does not have to be a 'true' OS event, it can
     *       be manufactured by the programmer and still processed by the
     *       arGUIWindowManager if added through this function.
     *
     * @param event The OS event to add.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the event manager is not active, or on failure.
     *         </ul>
     */
    int addEvent( arStructuredData& event );

    //@{
    /**
     * @name Convenience wrappers
     *
     * These functions can be used in place of the cumbersome
     * \ref addEventBase "addEvent( arStructuredData& )" to more easily add OS
     * events.
     */
    int addEvent( const arGUIKeyInfo& keyInfo );
    int addEvent( const arGUIMouseInfo& mouseInfo );
    int addEvent( const arGUIWindowInfo& windowInfo );
    //@}

    /**
     * Retrieve the next OS event on the queue of to-be-processed events.
     *
     * @note The caller is responsible for deleting returned pointers but
     *       care should be taken as this function <b>can</b> return NULL.
     *
     * @return <ul>
     *           <li>A pointer to an OS event on success.
     *           <li>NULL if the event manager is not active or if there are
     *               no OS events waiting to be processed.
     *         </ul>
     */
    arGUIInfo* getNextEvent( void );

    //@{
    /**
     * @name Accessors
     *
     * Retrieve GUI state data.
     *
     * @warning If the focus of the app is lost then certain state may be
     *          wrong.  Furthermore, if the mouse is moved outside the window
     *          (without a button held down) then the state will only reflect
     *          its last movement <em>inside</em> the window.
     */
    arGUIKeyInfo getKeyState( const arGUIKey key );
    arGUIMouseInfo getMouseState( void ) const;
    arGUIWindowInfo getWindowState( void ) const;
    //@}

    //@{
    /**
     * @name Accessors
     *
     * Retrieve or set event manager state information.
     */
    bool eventsPending( void );

    bool isActive( void ) const { return _active; }
    void setActive( const bool active = true );

    void setUserData( void* userData ) { _userData = userData; }
    //@}

    #if defined( AR_USE_WIN_32 )

      /**
       * Wrapper function for \ref windowProcCB
       *
       * This function is necessary as the Win32 function RegisterClassEx only
       * takes a static function of this exact type.
       *
       * @warning This function should <b>never</b> be called directly by
       *          anyone else.
       *
       * @see windowProcCB
       * @see arGUIWindow::_setupWindowCreation
       * @see arGUIWindow::getGUIEventManager
       */
      static LRESULT CALLBACK windowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

      /**
       * Consume any pending OS events dispatched from \ref consumeEvents
       *
       * @param arGUIWindow A pointer to the same arGUIWindow passed to consumeEvents.
       *
       * @see consumeEvents
       */
      LRESULT CALLBACK windowProcCB( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, arGUIWindow* window );
    #endif

  private:

    bool _active;                             // Is the event manager active?

    // bug: this is a lot of data to be copying around, any way to do this without passing-by-value?
    std::queue<arStructuredData > _events;    // Queue of consumed os events waiting to be processed by the window manager.
    arLock _eventsMutex;                      // Mutex protecting the event queue.

    void* _userData;                          // Data pointer defined by associated window.

    //@{
    /**
     * @name Current GUI state.
     *
     * @warning Having focus stolen from the app either voluntarily or
     *          involuntarily while OS events are coming in can seriously
     *          mess with these state variables.  Only way to get around this
     *          is to be 'stateless' but implementing key repeat events or
     *          previous mouse positions seems unlikely without keeping at
     *          least some minimal state.
     */
    arGUIMouseInfo _mouseState;
    arGUIWindowInfo _windowState;
    KeyboardMap _keyboardState;
    //@}


    //@{
    /**
     * @name Data templates
     *
     * For use by arStructuredData records as templates for different OS
     * events.  Exists in a 'superclass/subclass' hierarchy to more easily
     * create the events.
     */
    arTemplateDictionary* _GUIInfoDictionary;
    arDataTemplate*       _GUIInfoTemplate;
    arDataTemplate*       _keyInfoTemplate;
    arDataTemplate*       _mouseInfoTemplate;
    arDataTemplate*       _windowInfoTemplate;
    //@}

    int                   _eventIDs[ AR_NUM_GUI_EVENTS ];   // deprecated.

};

#endif

