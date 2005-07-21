//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

/**
 * @file arGUIWindowManager.h
 * Header file for the arGUIWindowManager class.
 */
#ifndef _AR_GUI_WINDOW_MANAGER_H_
#define _AR_GUI_WINDOW_MANAGER_H_

#include <map>
#include <vector>
#include <string>

#include "arThread.h"
#include "arGUIDefines.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class arWMEvent;
class arGUIRenderCallback;
class arGUIWindowingConstruct;
class arGraphicsWindow;
class arGUIInfo;
class arGUIKeyInfo;
class arGUIMouseInfo;
class arGUIWindowInfo;
class arGUIWindow;
class arGUIWindowConfig;
class arVector3;

//@{
/** @name Convenience typedef's for use in arGUIWindowManager functions.
 *
 * @todo Move these to a common location with other typedefs.
 */
typedef std::map<int, arGUIWindow* > WindowMap;
typedef WindowMap::iterator WindowIterator;

typedef std::vector<arWMEvent* > EventVector;
typedef EventVector::iterator EventIterator;
//@}

/**
 * The manager for all the windows on the system.
 *
 * arGUIWindowManager is the API presented to the programmer wishing to create
 * GUI windows.  It is responsible for the creation, management, and
 * destruction of any such windows.  It is also responsible for passing back
 * window, keyboard, and mouse events to the programmer either through
 * callbacks or subclassing.  It can operate in two distinct modes, single-
 * threaded or multi-threaded, with the various arGUIWindow manipulators and
 * accessors behaving differently yet appropriately in either mode.
 *
 * @see arGUIWindow
 */
class SZG_CALL arGUIWindowManager
{
  public:

    /**
     * The arGUIWindowManager constructor.
     *
     * @param windowCallback       The user-defined function that will be
     *                             passed window events.
     * @param keyboardCallback     The user-defined function that will be
     *                             passed keyboard events.
     * @param mouseCallback        The user-defined function that will be
     *                             passed  mouse events
     * @param windowInitGLCallback The user-defined function that will be
     *                             called after a window's OpenGL context is
     *                             created
     * @param threaded             A flag determining whether the window
     *                             manager will operate in single-threaded mode
     */
    arGUIWindowManager( void (*windowCallback)( arGUIWindowInfo* ) = NULL,
                        void (*keyboardCallback)( arGUIKeyInfo* ) = NULL,
                        void (*mouseCallback)( arGUIMouseInfo* ) = NULL,
                        void (*windowInitGLCallback)( arGUIWindowInfo* ) = NULL,
                        bool threaded = true );

    /**
     * The arGUIWindowManager destructor
     *
     * @note The destructor is virtual so that arGUIWindowManager can be
     *       sub-classed if desired.
     */
    virtual ~arGUIWindowManager( void );

    /**
     * Start the window manager running.
     *
     * The window manager will enter an infinite loop where it will issue draw
     * requests, then swap requests, then process OS event messages and then
     * call the event handlers for those messages for each window it controls.
     *
     * @note The draw requests will not be blocking and the swap requests will
     *       be blocking iff the window manager is in multi-threaded mode.
     *
     * @return Unused, this function will never return.
     *
     * @see drawAllWindows
     * @see swapAllWindowBuffers
     * @see processWindowEvents
     */
    int startWithSwap( void );

    /**
     * Start the window manager running.
     *
     * Very similar to \ref startWithSwap this function will also enter the same
     * infinite loop but will not issue swap requests.  This function most
     * closely mimics GLUT's \c glutMainLoop() with the user responsible for
     * issuing swap buffer commands.
     *
     * @note The draw requests will not be blocking.
     *
     * @return Unused, this function will never return.
     *
     * @see drawAllWindows
     * @see processWindowEvents
     */
    int startWithoutSwap( void );

    /**
     * Create a new arGUIWindow.
     *
     * @note If the window manager is in single-threaded mode, the new
     *       arGUIWindow will exist in the same thread as the window manager.
     *       If the window manager is in multi-threaded mode then the new
     *       arGUIWindow will be created in a new thread.
     *
     * @note This function will only return (even in multi-threaded mode) only
     *       after the window has actually been created and is running.
     *
     * @param windowConfig The window configuration object specifying the
     *                     parameters of the new window.
     *
     * @return <ul>
     *           <li>The new window's ID (an integer > 0) on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see arGUIWindow::arGUIWindow
     * @see arGUIWindow::beginEventThread
     * @see arGUIWindow::_performWindowCreation
     */
    int addWindow( const arGUIWindowConfig& windowConfig );

    /**
     * Create a set of (possibly) new arGUIWindows.
     *
     * A utility function that uses a parsed XML description of arGUIWindow's
     * to create the windows.  If any windows currently exist this function
     * will attempt to do an intelligent 'diff' of the new windows against the
     * old windows, tweaking windows where possible and only creating or
     * deleting windows where necessary.
     *
     * @note This function may modify the window manager's threaded state.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see addWindow
     * @see deleteWindow
     * @see setGraphicsWindow
     * @see registerDrawCallback
     * @see arGUIWindowingConstruct
     * @see arGUIXMLWindowConstruct
     */
    int createWindows( const arGUIWindowingConstruct* windowingConstruct = NULL );

    //@{
    /** @name Register Callbacks
     *
     * If the arGUIWindowManager constructor was called with NULL for any of
     * the callbacks these functions can be used to later register appropriate
     * functions.
     *
     * @warning These functions will blindly overwrite any currently registered
     *          callbacks.
     */
    void registerWindowCallback( void (*windowCallback) ( arGUIWindowInfo* ) );
    void registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) );
    void registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) );
    void registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) );
    //@}

    /**
     * Register a draw callback with a window.
     *
     * @param windowID     The window to register the callback with.
     * @param drawCallback The user-defined draw callback.
     *
     * @return <ul>
     *           <li>0 on successful registration.
     *           <li>-1 if the window could not be found.
     *         </ul>
     */
    int registerDrawCallback( const int windowID, arGUIRenderCallback* drawCallback );

    /**
     * Process any pending window events.
     *
     * For each window that the window manager controls, retrieve every OS
     * event that occurred since the last call and pass them on to the
     * programmer using the registered callbacks.
     *
     * @note If the window manager is in single-threaded mode this function is
     *       also responsible for calling \ref consumeAllWindowEvents so that
     *       the OS events are properly consumed.
     *
     * @return <ul>
     *           <li>0 on success
     *           <li>-1 on failure
     *         </ul>
     *
     * @see consumeAllWindowEvents
     * @see arGUIWindow::getNextGUIEvent
     * @see _keyboardHandler
     * @see _mouseHandler
     * @see _windowHandler
     */
    int processWindowEvents( void );

    /**
     * Retrieve the next item on the window's stack of OS events.
     *
     * @note Meant as a helper function if the programmer wishes to control
     *       the operation of the window manager herself.
     *
     * @note The caller is responsible for deleting the returned pointer.
     *
     * @param windowID The window for which to retrieve the next event.
     *
     * @return <ul>
     *           <li>A pointer to the OS event on success.
     *           <li>NULL if the window isn't found, if the window isn't
     *               currently running, or if there are no current OS events.
     *         </ul>
     *
     * @see arGUIWindow::getNextGUIEvent
     */
    arGUIInfo* getNextWindowEvent( const int windowID );

    /**
     * Issue a draw request to a single window.
     *
     * @note If the window manager is in single-threaded mode the draw request
     *       takes the form of an immediate execution of the window's draw
     *       handler whereas if the window manager is in multi-threaded mode
     *       the draw request will be passed as a message to the window.
     *
     * @param windowID The window for which to request the draw.
     * @param blocking Whether the window manager should wait for the request
     *                 to finish before returning (meaningless in single-
     *                 threaded mode).
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see addWMEvent
     */
    int drawWindow( const int windowID, bool blocking = false );

    /**
     * Issue a draw request to every window.
     *
     * @note Operates similarly to \ref drawWindow with respect to single- vs
     *       multi-threading.
     *
     * @param blocking Whether the window manager should wait for the request
     *                 to finish before returning (meaningless in single-
     *                 threaded mode).
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see addAllWMEvent
     */
    int drawAllWindows( bool blocking = false );

    /**
     * Consume a window's pending OS events.
     *
     * The window will push any pending OS events onto its stack of such
     * events, to be processed by the window manager.
     *
     * @warning This function is only meant for use by \ref processWindowEvents
     *          in single-threaded mode.  Appropriate checks that it is not
     *          otherwise called are in place.
     *
     * @param windowID The window whose OS events should be consumed.
     * @param blocking Meaningless.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found, if the window is not
     *               currently running, or if there is otherwise an error.
     *         </ul>
     *
     * @see arGUIWindow::_consumeWindowEvents
     */
    int consumeWindowEvents( const int windowID, bool blocking = false );

    /**
     * Consume every window's pending OS events.
     *
     * @note Operates similarly to \ref consumeWindowEvents with respect to
     *       single- vs. multi-threading.
     *
     * @param blocking Meaningless.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see arGUIWindow::_consumeWindowEvents
     */
    int consumeAllWindowEvents( bool blocking = false );

    /// NOTE: if this wm is managing multiple windows with different _Hz's, then
    /// blocking must be false otherwise both windows will run at whatever the
    /// lowest rate is

    /**
     * Issue a swap request to a single window.
     *
     * @note If the window manager is in single-threaded mode the swap request
     *       takes the form of an immediate swap of the window's buffers
     *       whereas if the window manager is in multi-threaded mode the swap
     *       request will be passed as a message to the window.
     *
     * @param windowID The window for which to request the swap.
     * @param blocking Whether the window manager should wait for the request
     *                 to finish before returning (meaningless in single-
     *                 threaded mode).
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see addWMEvent
     *
     * @todo properly implement windows having different _Hz's.
     */
    int swapWindowBuffer( const int windowID, bool blocking = false );

    /**
     * Issue a swap request to every window.
     *
     * @note Operates similarly to \ref swapWindowBuffer with respect to
     *       single- vs multi-threading.
     *
     * @param blocking Whether the window manager should wait for the request
     *                 to finish before returning (meaningless in single-
     *                 threaded mode).
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see addAllWMEvent
     */
    int swapAllWindowBuffers( bool blocking = false );

    /**
     * Set a window's width and height.
     *
     * @note Sets the window's <b>client area</b> width and height.
     *
     * @param windowID The window to perform the operation on.
     * @param width    The new width.
     * @param height   The new height.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::resize
     */
    int resizeWindow( const int windowID, int width, int height );

    /**
     * Move a window to a new location.
     *
     * @note The upper-left corner of the window's screen (not client area)
     *       will be moved to the new location.
     *
     * @param windowID The window to perform the operation on.
     * @param x        The new x coordinate.
     * @param y        The new y coordinate.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::move
     */
    int moveWindow( const int windowID, int x, int y );

    /**
     * Sets the window's OpenGL viewport.
     *
     * @param windowID The window to perform the operation on.
     * @param x        The left corner x coordinate.
     * @param y        The bottom corner y coordinate.
     * @param width    The width of the viewport.
     * @param height   The height of the viewport.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::setViewport
     */
    int setWindowViewport( const int windowID, int x, int y, int width, int height );

    /**
     * Put the window in fullscreen mode.
     *
     * @note Does not resize the desktop to the window's resolution, instead
     *       the window's resolution is changed to match that of the desktop.
     *
     * @note Call \ref resizeWindow to restore the window to a non-fullscreen
     *       state.
     *
     * @param windowID The window to perform the operation on.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::fullscreen
     */
    int fullscreenWindow( const int windowID );

    /**
     * Set the state of the window's border decorations.
     *
     * @warning Under X11 this function is window manager specific, currently
     *          it should work under Motif, KDE and Gnome.
     *
     * @note Has no effect if the window is in fullscreen mode.
     *
     * @param windowID The window to perform the operation on.
     * @param decorate A flag determining whether border decorations should be
     *                 turned on or off.
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::decorate
     */
    int decorateWindow( const int windowID, bool decorate );

    /**
     * Set the state of the window's mouse cursor.
     *
     * @param windowID The window to perform the operation on.
     * @param cursor   The new cursor type of the window.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::setCursor
     */
    int setWindowCursor( const int windowID, arCursor cursor );

    /**
     * Set the state of the window's z order.
     *
     * @param windowID The window to perform the operation on.
     * @param zorder   The new zorder of the window.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window could not be found or there was otherwise
     *               an error.
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::raise
     */
    int raiseWindow( const int windowID, arZOrder zorder );

    //@{
    /** @name Window state accessors
     *
     * Retrieve or set a window's current state.
     */
    bool windowExists( const int windowID );

    arVector3 getWindowSize( const int windowID );
    arVector3 getWindowPos( const int windowID );
    arVector3 getMousePos( const int windowID );

    arCursor getWindowCursor( const int windowID );

    bool isStereo( const int windowID );
    bool isFullscreen( const int windowID );
    bool isDecorated( const int windowID );

    arZOrder getZOrder( const int windowID );

    int getBpp( const int windowID );
    std::string getTitle( const int windowID );
    std::string getXDisplay( const int windowID );

    void setTitle( const int windowID, const std::string& title );

    void* getUserData( const int windowID );
    void setUserData( const int windowID, void* userData );

    arGraphicsWindow* getGraphicsWindow( const int windowID );
    void returnGraphicsWindow( const int windowID );
    void setGraphicsWindow( const int windowID, arGraphicsWindow* graphicsWindow );
    //@}

    /**
     * Retrieve the current state of a keyboard key.
     *
     * @note this function is static as the keyboard is not tied to any
     *       particular window.
     *
     * @param key The key whose state to retrieve.
     *
     * @return arGUIKeyinfo The state information.
     *
     * @todo Actually implement this function using the appropriate OS calls.
     *       Note that under certain flavors of windows (namely ME) the
     *       necessary function need to implement this function (GetKeyState)
     *       is hard-coded to return nothing.
     */
    static arGUIKeyInfo getKeyState( const arGUIKey key );

    /// Getting the mouse state outside of the context of a window doesn't make
    /// sense.
    /// static arMouseInfo getMouseState( void );

    //@{
    /** @name Window manager state accessors
     *
     * Retrieve or set the window manager's current state.
     */
    int getNumWindows( void ) const { return _windows.size(); }
    bool hasActiveWindows( void ) const { return !_windows.empty(); }

    bool isFirstWindow( const int windowID ) const { return( _windows.find( windowID ) == _windows.begin() ); }
    int getFirstWindowID( void ) const { return _windows.begin()->first; }

    bool isThreaded( void ) const { return _threaded; }
    void setThreaded( bool threaded );

    void setUserData( void* userData ) { _userData = userData; }
    void* getUserData( void ) const { return _userData; }
    //@}

    /**
     * Issue a kill request to a single window.
     *
     * @note If the window manager is in single-threaded mode the kill request
     *       will be executed immediately whereas if the window manager is in
     *       multi-threaded mode the draw request will be passed as a blocking
     *       message to the window.
     *
     * @param windowID The window for which to request the kill.
     *
     * @return <ul>
     *           <li>0 on success (with a guarantee that on return the window
     *               has already been destroyed).
     *           <li>-1 on failure (the window may not actually be destroyed).
     *         </ul>
     *
     * @see addWMEvent
     * @see arGUIWindow::_killWindow
     */
    int deleteWindow( const int windowID );

    /**
     * Issue a kill request to every window.
     *
     * @note Operates similarly to \ref deleteWindow with respect to single- vs
     *       multi-threading.
     *
     * @return <ul>
     *           <li>0 on success (with a guarantee that on return the windows
     *               have already been destroyed).
     *           <li>-1 on failure (the windows may not actually be destroyed).
     *         </ul>
     *
     * @see deleteWindow
     */
    int deleteAllWindows( void );

    //@{
    /** @name Window manager wildcat framelock accessors.
     *
     * Operate upon the wildcat framelock.
     */
    void useFramelock( bool isOn );
    void findFramelock( void );
    void activateFramelock( void );
    void deactivateFramelock( void );
    //@}

  private:

    /**
     * Add a window manager message to a window.
     *
     * @note Meant to be used as a helper function for those public functions
     *       that take a single window ID.
     *
     * @note In single-threaded mode any event passed to this function will be
     *       executed by the window immediately and the return value
     *       <b>will</b> be NULL.
     *
     * @warning The caller should <b>not</b> delete the returned pointer.
     *          It will be managed by the window itself.  The caller is also
     *          the one who should decide if this is a blocking call or not.
     *
     * @param windowID The window to pass the message to.
     * @param event    The message to pass to the window.
     *
     * @return A pointer to the event added by the window, necessary so that
     *         the caller can appropriately block on the event's completion.
     *
     * @see arGUIWindow::addWMEvent
     */
    arWMEvent* addWMEvent( const int windowID, arGUIWindowInfo event );

    /**
     * Add a window manager message to every window.
     *
     * @note Meant to be used as a helper function for those public functions
     *       that perform an operation on all windows.
     *
     * @param wmEvent  The message to pass to the windows.
     * @param blocking Whether this should call should block on the event's
     *                 completion.  If so, it will not return until all windows
     *                 have parsed the message. (Meaningless in single-threaded
     *                 mode)
     *
     * @return <ul>
     *           <li>0
     *         </ul>
     *
     * @see addWmEvent
     * @see WMEvent::wait
     */
    int addAllWMEvent( arGUIWindowInfo wmEvent, bool blocking );

    /**
     * A helper function for \ref deleteWindow and \ref deleteAllWindows.
     *
     * Organizes functionality common to both delete calls.
     *
     * @param windowID The window to send the message to.
     */
    void _sendDeleteEvent( const int windowID );

    //@{
    /** @name Wrappers for the keyboard, mouse, and window callbacks.
     *
     * @note If the programmer wants something different to happen on OS events
     *       other than simply the user-defined callbacks being called then she
     *       should subclass arGUIWindowManager and overload these functions.
     */
    virtual void _keyboardHandler( arGUIKeyInfo* keyInfo );
    virtual void _mouseHandler( arGUIMouseInfo* mouseInfo );
    virtual void _windowHandler( arGUIWindowInfo* windowInfo );
    //@}

    void (*_keyboardCallback)( arGUIKeyInfo* keyInfo );              ///< The keyboard event callback
    void (*_mouseCallback)( arGUIMouseInfo* mouseInfo );             ///< The mouse event callback
    void (*_windowCallback)( arGUIWindowInfo* windowInfo );          ///< The window event callback
    void (*_windowInitGLCallback)( arGUIWindowInfo* windowInfo );    ///< The window opengl initialization callback

    WindowMap _windows;       ///< A map of all the managed windows and their id's.

    int _maxWindowID;         ///< The maximum window ID, used in creating new windows.

    bool _threaded;           ///< The mode of operation for the window manager.

    void* _userData;          ///< Default user defined data pointer passed to created windows.

};

#endif  // header guard

