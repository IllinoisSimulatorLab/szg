//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

/**
 * @file arGUIWindow.h
 * Header file for the arGUIWindow class.
 */
#ifndef _AR_GUI_WINDOW_H_
#define _AR_GUI_WINDOW_H_

#if defined( AR_USE_WIN_32 )
  #ifndef CDS_FULLSCREEN     // CDS_FULLSCREEN is not defined by some compilers
    #define CDS_FULLSCREEN 4
 #endif
#endif

#include "arThread.h"
#include "arGraphicsWindow.h"
#include "arGUIInfo.h"

#include <string>
#include <queue>

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/**
 * \struct arGUIWindowHandle
 *
 * A struct of the different OS-specific pieces comprising a window.
 *
 * Useful as a function parameter without the functions needing to have
 * different signatures for each OS.
 */

#if defined( AR_USE_WIN_32 )
  #include <windows.h>
  #include <GL/gl.h>
  #include <GL/glu.h>

  typedef struct {
      HWND  _hWnd;
      HDC   _hDC;
      HGLRC _hRC;
      HINSTANCE _hInstance;
  } arGUIWindowHandle;
#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  // NOTE: the fullscreen-by-extensions code still exists in the .cpp file, but
  // we are no longer using it to facilitate switching back and forth between
  // fullscreen and non-fullscreen windows easier, plus it's just plain not
  // supported on many platforms

  #include <X11/Xlib.h>
  #include <X11/cursorfont.h>

  #if defined( AR_USE_LINUX ) || defined( AR_USE_SGI )

    #undef HAVE_X11_EXTENSIONS

    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glx.h>

    #ifdef HAVE_X11_EXTENSIONS
      #include <X11/extensions/xf86vmode.h>
    #endif
  #elif defined( AR_USE_DARWIN )
    // FIXME: really should have a robust method for checking for the existence
    // of the X11 extensions, however on OS X this seems impossible.  Apple's
    // X11 User SDK packages provide all the necessary headers and libraries
    // to compile and link using extensions, but on runtime the app will fail
    // citing a lack of the "XFree86-VidModeExtension" which a "xdpyinfo
    // -queryExtension" confirms is, in fact, missing.  So, any check would
    // have to be runtime based rather than compile- or link-time based, not
    // sure how to do this robustly.  XListExtensions is another way to check,
    // but could only be done at runtime, right here we need to know whether
    // we should even try to include the extension headers or not
    #undef HAVE_X11_EXTENSIONS

    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
    #include <GL/glx.h>

    #ifdef HAVE_X11_EXTENSIONS
      #include <X11/extensions/xf86vmode.h>
    #endif
  #endif

  typedef struct {
    unsigned int cursorShape;    ///< an XC_{cursor} value
    Cursor cachedCursor;         ///< None if the corresponding cursor has not been created yet
  } cursorCacheEntry;

  typedef struct {
    Display*   _dpy;
    int        _screen;
    Window     _win;
    Window     _root;
    GLXContext _ctx;
    XSetWindowAttributes _attr;
    XVisualInfo* _vi;
    #ifdef HAVE_X11_EXTENSIONS
      XF86VidModeModeInfo  _dMode;
    #endif
    Atom _wDelete, _wHints;
  } arGUIWindowHandle;
#endif

class arGUIEventManager;
class arGUIInfo;
class arGUIWindowInfo;
class arWMEvent;

//@{
/**
 * @name Convenience typedef's for use in arGUIWindow functions.
 *
 * @todo Move these to a common location with other typedefs.
 */
typedef std::queue<arWMEvent* >  EventQueue;
typedef std::vector<arWMEvent* > EventVector;

typedef EventVector::iterator EventIterator;
//@}

/**
 * An arGUWindow draw callback.
 *
 * Derives from arRenderCallback so that setting draw callbacks with
 * arWindowInfo* arguments is a little easier.
 *
 * @note If a programmer wishes to register a draw callback with an
 *       arGUIWindow that has a signature different than the default they
 *       should subclass this class and implement the () operater themselves.
 *
 * @see arGUIWindow::arGUIWindow
 * @see arGUIWindow::registerDrawCallback
 */
class SZG_CALL arGUIRenderCallback : public arRenderCallback
{
  public:

    /**
     * The arGUIRenderCallback constructer.
     */
    arGUIRenderCallback( void )  { }

    /**
     * The arGUIRenderCallback destructor.
     */
    virtual ~arGUIRenderCallback( void )  { }

    /**
     * Leave it up to the subclass to decide to implement this
     */
    virtual void operator()( arGraphicsWindow&, arViewport& ) = 0;

    /**
     * Overload the () operator so this class can 'simulate' a function call.
     *
     * @warning As this function is pure virtual this class cannot be
     *          instantiated and must be subclassed before use.
     *
     * @param windowInfo default drawcallback argument
     */
    virtual void operator()( arGUIWindowInfo* windowInfo ) = 0;

  private:

};

/**
 * The default arGUIwindow draw callback.
 *
 * This draw callback can be used the majority of the time by any programmer
 * registering a draw callback with arGUIWindow.
 */
class SZG_CALL arDefaultGUIRenderCallback : public arGUIRenderCallback
{
  public:

    /**
     * The arDefaultGUIRenderCallback constructor.
     *
     * @param drawCallback The user-defined function to use as a draw callback
     */
    arDefaultGUIRenderCallback( void (*drawCallback)( arGUIWindowInfo* ) = NULL ) :
      _drawCallback( drawCallback ) { }

    /**
     * The arDefaultGUIRenderCallback destructor.
     */
    virtual ~arDefaultGUIRenderCallback( void )  { }

    /**
     * Implemented overloaded () operator.
     *
     * Makes the actual call to the user defined draw callback.
     */
    virtual void operator()( arGUIWindowInfo* windowInfo = NULL );

    virtual void operator()( arGraphicsWindow&, arViewport& );

  private:

    void (*_drawCallback)( arGUIWindowInfo* );        ///< The user-defined drawcallback

};

/**
 * A window configuration object.
 *
 * Meant to be used in the creation of a new arGUIWindow as a more convenient
 * means of specifying the window parameter than passing them in one by one.
 *
 * @note Should be immutable - the window should not change any values
 *
 *
 * @see arGUIWindowManager::addWindow
 * @see arGUIWindow::arGUIWindow
 */
class SZG_CALL arGUIWindowConfig
{
  public:

    /**
     * The arGUIWindowConfig constructor.
     *
     * @param x          The x location of the window.
     * @param y          The y location of the window.
     * @param width      The width of the window.
     * @param height     The height of the window.
     * @param bpp        The bit-depth of the window.
     * @param Hz         The refresh-rate of the window. 
     * @param decorate   Whether the window should have border decorations.
     * @param topmost    Whether the window should be on top or not.
     * @param fullscreen Whether the window should be fullscreen.
     * @param stereo     Whether the window should support active stereo.
     * @param title      The text in the title bar of the window.
     * @param XDisplay   The connection string to the X server.
     * @param initialCursor Request for initial cursor shape.
     *
     * @todo Test windows with XDisplay strings other than the default
     */
    arGUIWindowConfig( int x = 50, int y = 50, int width = 640, int height = 480,
                       int bpp = 16, int Hz = 0, bool decorate = true, bool topmost = false,
                       bool fullscreen = false, bool stereo = false,
                       const std::string& title = "SyzygyWindow",
                       const std::string& XDisplay = ":0.0",
                       int initialCursor = AR_CURSOR_ARROW );

    /**
     * The arGUIWindowConfig destructor.
     */
    ~arGUIWindowConfig( void );

    // int readFromFile( ... );
    // int readFromCommandLine( ... );
    // int readFromSZGDatabase( ... );

  public:

    int _x, _y, _width, _height, _bpp, _Hz;

    bool _decorate, _topmost, _fullscreen, _stereo;

    std::string _title, _XDisplay;

    int _initialCursor;

};

/**
 * A handle to a window manager event.
 *
 * Used to keep track of a passed window manager event by both the window
 * and the window manager.  The window creates an arWMEvent in response to a
 * message by the window manager and the window manager uses the arWMEvent
 * to block on the message's completion.
 *
 * @see arGUIWindowManager::addWMEvent
 * @see arGUIWindowManager::addAllWMEvent
 * @see arGUIWindow::addWMEvent
 * @see arGUIWindow::_processWMEvents
 * @see arGUIWindowInfo
 */
class SZG_CALL arWMEvent
{
  public:

    /**
     * The arWMEvent constructor.
     *
     * @param event Information about the event.
     */
    arWMEvent( const arGUIWindowInfo& event );

    /**
     * The arWMEvent destructor.
     */
    ~arWMEvent( void );

    /**
     * Reset the event state so that it can be reused.
     *
     * @param event Information to reset the state to.
     */
    void reset( const arGUIWindowInfo& event );

    /**
     * Possibly wait on the completion of the event.
     *
     * @note Those functions returned a arWMEvent should <b>always</b> call
     *       wait on the event, with \a blocking equal to true if they don't
     *       want to wait for the event to complete so that the event's
     *       \c _done status is appropriately updated and the event can be
     *       reused.
     *
     * @param blocking Whether to block on the completion of the event or not.
     */
    void wait( const bool blocking = false );

    /**
     * Signal the completion of the event.
     *
     * @see arGUIWindow::_processWMEvents
     */
    void signal( void );

  // should change this to private...
  public:

    arGUIWindowInfo _event;             ///< The current message information.
    bool _conditionFlag;                ///< Condition flag for use in conjunction with _eventCond.

    int _done;                          ///< 0 if new/reset event, 1 if signaled, 2 if waited, allows 'done' events to be reused by arGUIWindow.

    arConditionVar _eventCond;          ///< Condition variable for use in blocking.
    arMutex _eventMutex, _doneMutex;    ///< Mutexes to ensure thread safety.

};

/**
 * Wrapper class for buffer swaps.
 *
 * @see arGUIWindow::swap
 *
 * @todo implement wildcat framelocking functionality inside this class
 */
class arGUIWindowBuffer
{
  public:

    /**
     * The arGUIWindowBuffer constructor.
     *
     * @param dblBuf If the window is double-buffered (unused).
     */
    arGUIWindowBuffer( bool dblBuf = true );

    /**
     * The arGUIWindowBuffer destructor.
     */
    ~arGUIWindowBuffer( void );

    /**
     * Perform the buffer swap using OS-specific API calls.
     *
     * @param windowHandle Handle to the window to perform the buffer swap on.
     * @param stereo       Under Win32 there are different buffer swap calls for stereo windows.
     *
     * @return 0
     */
    int swapBuffer( const arGUIWindowHandle& windowHandle, const bool stereo ) const;

  private:

    bool _dblBuf;     ///< if the window is double buffered enable (currently unused).

};

/**
 * A GUI Window in which graphics can be displayed.
 *
 * This class is responsible for creating an OS-level window to be managed by
 * an arGUIWindowManager.  It is also responsible for responding to any draw
 * requests by calling the appropriate handler.  It can operate in either a
 * threaded or non-threaded mode.

 * @note This class is never actually exposed to the programmer, she must
 * access arGUIWindow functionality through the arGUIWindowManager.
 *
 * @see arGUIWindowManager
 * @see arGUIWindowBuffer
 * @see arWMEvent
 *
 */
class SZG_CALL arGUIWindow
{
  /**
   * @note arGUIWindowManager needs to be a friend for access to the window
   *       creation/destruction functions if this window is running in non-
   *       threaded mode.
   */
  friend class arGUIWindowManager;

  public:

    /**
     * The arGUIWindow constructor.
     *
     * @note Because of flow control issues elsewhere the OS window is not
     *       actually  created in the constructor.
     *
     * @param ID           A unique identifier for this window.
     * @param windowConfig The window configuration object.
     * @param drawCallback The user-defined function to be called on draw
     *                     requests.
     */
    arGUIWindow( int ID, arGUIWindowConfig windowConfig,
                 void (*windowInitGLCallback)( arGUIWindowInfo* windowInfo ) = NULL,
                 void* userData = NULL );

   /**
     * The arGUIWindow destructor
     *
     * @note The destructor is virtual so that arGUIWindowManager can be
     *       sub-classed if desired.
     *
     * @see _killWindow
     */
    virtual ~arGUIWindow( void );

    /**
     * Register a draw callback.
     *
     * If the arGUIWindow constructor was called with NULL for the draw
     * callback this function can be used to later register an appropriate
     * callback.
     *
     * @note Will blindly overwrite any previously registered callback.
     *
     * @param drawCallback The user-defined function to be called on draw
     *                     requests.
     */
    void registerDrawCallback( arGUIRenderCallback* drawCallback );

    /**
     * Create a new window inside a new thread and start event processing.
     *
     * @note If the window manager responsible for this window is in multi-
     *       threaded mode any new windows that are added will be created using
     *       this function.
     *
     * @warning If window creation takes longer than 5 seconds (an arbitrarily
     *          picked value) this function will return in failure.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on window creation failure.
     *         </ul>
     *
     * @see arGUIWindowManager::addWindow
     * @see mainLoop
     */
    int beginEventThread( void );

    /**
     * Return the next item on the OS event stack.
     *
     * Used as a helper function by arGUIWindowManager to process OS events
     * and pass them to the appropriate handlers.
     *
     * @return <ul>
     *           <li>A pointer to an arGUIinfo object (or one of it's
     *               subclasses) on success.
     *           <li>NULL if the window is not running or if there are no
     *               pending OS events.
     *         </ul>
     *
     * @see arGUIWindowManager::processWindowEvents
     */
    arGUIInfo* getNextGUIEvent( void );

    /**
     * Add an event message from the window manager to the stack.
     *
     * @note The caller should <b>not</b> clean up the returned pointer, the
     *       window manages these itself.
     *
     * @param event The event message to add.
     *
     * @return <ul>
     *          <li>A pointer to the added message, the caller may use this to
     *              block on the call until it has completed.
     *          <li>NULL if the window is not running.
     *         </ul>
     *
     * @see arGUIWindowManager::addWMEvent
     */
    arWMEvent* addWMEvent( arGUIWindowInfo& event );


    /**
     * Perform a buffer swap.
     *
     * @note in non-threaded mode the window's context is made current by this
     *       function.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running or on failure.
     *         </ul>
     *
     * @see arGUIWindowBuffer::swapBuffer
     * @see makeCurrent
     */
    int swap( void );

    /**
     * Set the window's width and height.
     *
     * @note This function should be used to come out of fullscreen mode.
     *
     * @param newWidth The window's new width.
     * @param newHeight The window's new height.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running.
     *         </ul>
     *
     * @see fullscreen
     */
    int resize( int newWidth, int newHeight );

    /**
     * Move the window to a new location.
     *
     * @note The upper-left corner of the window's screen (not client area)
     *       will be moved to the new location.
     *
     * @param newX The window's new x coordinate.
     * @param newY The window's new y coordinate.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running or is in fullscreen mode.
     *         </ul>
     */
    int move( int newX, int newY );

    /**
     * Set the window's OpenGL viewport.
     *
     * @note in non-threaded mode the window's context is made current by this
     *       function.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running.
     *         </ul>
     *
     * @see makeCurrent
     */
    int setViewport( int newX, int newY, int newWidth, int newHeight );

    /**
     * Put the window into fullscreen mode.
     *
     * @note Does not resize the desktop to the window's resolution, instead
     *       the window's resolution is changed to match that of the desktop.
     *
     * @note Call \ref resize to restore the window to a non-fullscreen state.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running or already fullscreen.
     *         </ul>
     */
    int fullscreen( void );

    /**
     * Sets the state of the window's context.
     *
     * @param release If true, release the context from this thread otherwise
     *                make the context current to this thread.
     *
     * @return <ul>
     *           <li>0 on success or if the context is already current.
     *           <li>-1 if the window is not running or on failure.
     *         </ul>
     */
    int makeCurrent( bool release = false );

    /**
     * Raise the window in the z ordering.
     */
    void raise( void );

    /**
     * Lower the window in the z ordering.
     */
    void lower( void );

    /**
     * Minimize the window to the task bar.
     */
    void minimize( void );

    /**
     * Restore the window from the task bar.
     */
    void restore( void );

    /**
     * Set the state of the window's border decorations.
     *
     * @warning Under X11 this function is window manager specific, currently
     *          it should work under Motif, KDE and Gnome.
     *
     * @note has no effect if the window is in fullscreen mode.
     *
     * @param decorate A flag determining whether border decorations should be
     *                 turned on or off.
     */
    void decorate( const bool decorate );

    /**
     * Set the state of the window's cursor
     *
     * @param cursor The new cursor state.
     *
     * @todo implement more cursor states.
     */
    arCursor setCursor( arCursor cursor );

    //@{
    /**
     * @name Window state accessors.
     *
     * Retrieve or set window state data.
     *
     * @note getWidth, getHeight, getPosX, and getPosY follow the freeglut
     *       conventions concerning how border decorations and screen area
     *       vs client area play into these values.
     *
     * @todo Add setters for members such as the title.
     */
    void setVisible( const bool visible )  {  _visible = visible;  }
    bool getVisible( void ) const {  return _visible;  }

    std::string getTitle( void ) const {  return _windowConfig._title; }

    int getID( void ) const { return _ID; }

    int getWidth( void ) const;
    int getHeight( void ) const;

    int getPosX( void ) const;
    int getPosY( void ) const;

    bool isStereo( void )     const { return _windowConfig._stereo; }
    bool isFullscreen( void ) const { return _fullscreen; }
    bool isDecorated( void )  const { return _decorate; }
    bool isTopmost( void )    const { return _topmost; }

    bool running( void ) const { return _running; }
    bool eventsPending( void ) const;

    arCursor getCursor( void ) const { return _cursor; }

    const arGUIWindowHandle& getWindowHandle( void ) const { return _windowHandle; }

    const arGUIWindowConfig& getWindowConfig( void ) const { return _windowConfig; }

    void* getUserData( void ) const { return _userData; }
    void setUserData( void* userData ) { _userData = userData; }
    //@}

    /**
     * Retrieve the arGUIEventManager for this window.
     *
     * @return A pointer to the windows arGUIEventManager private member.
     *
     * @todo There must be a better way of handling this, as the only place
     *       this function is needed is actually in arGUIEventManager.
     *       Possibly some friend functions would be enough rather than
     *       exposing the pointer, as it cannot even be returned as const.
     */
    arGUIEventManager* getGUIEventManager( void ) const {  return _GUIEventManager; }

    void useWildcatFramelock( bool isOn );
    void findWildcatFramelock( void );
    void activateWildcatFramelock( void );
    void deactivateWildcatFramelock( void );

  private:

    /**
     * Wrapper function for \ref _mainLoop
     *
     * @warning Not meant for use in non-threaded mode.
     *
     * @see beginEventThread
     * @see _mainLoop
     */
    static void mainLoop( void* data );

    /**
     * The main processing loop of the window.
     *
     * Responsible for window creation and then event processing using
     * \ref _consumeWindowEvents.
     *
     * @warning Not meant for use in non-threaded mode.
     *
     * @see _performWindowCreation
     * @see _consumeWindowEvents
     */
    void _mainLoop( void );

    /**
     * Consume any pending OS events and process any pending window manager
     * events.
     *
     * @note In non-threaded mode this function must be called by the window
     *       manager (preferably at least once per iteration) for the window
     *       to "do" anything at all.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if the window is not running or on failure.
     *        </ul>
     *
     * @see arGUIEventManager::consumeEvents
     * @see _processWMEvents
     */
    int _consumeWindowEvents( void );

    /**
     * Process any pending event messages from the window manager, taking
     * the appropriate action on each message.
     *
     * @note Meant for use as a helper function for \ref _consumeWindowEvents.
     *
     * @return 0
     */
    int _processWMEvents( void );

    /**
     * Create the GUI window.
     *
     * This function takes places in three steps (implemented as
     * \ref _setupWindowCreation, \ref _windowCreation, and
     * \ref _tearDownWindowCreation).
     *
     * @note This function is the main reason arGUIWindowManager is a friend
     *       class as in non-threaded mode it does not call
     *       \ref beginEventThread and must create the window directly.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @see _setupWindowCreation
     * @see _windowCreation
     * @see _tearDownWindowCreation
     */
    int _performWindowCreation( void );

    /**
     * Prepare for window creation.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     */
    int _setupWindowCreation( void );

    /**
     * Perform the OS-specific window creation procedure.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     */
    int _windowCreation( void );

    /**
     * Cleanup after window creation.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     */
    int _tearDownWindowCreation( void );

    /**
     * Change the desktop's resolution.
     *
     * @deprecated This function should not be used, it is only fully implemented
     *             under Win32 as under Linux it would need to make use of X11
     *             extensions which are not supported on every X platform (e.g.
     *             OSX, and SGI)
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     */
    int _changeScreenResolution( void );

    /**
     * Destroy the window and stop the event manager.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 on failure.
     *         </ul>
     *
     * @todo In multi-threaded mode, ensure that this function only carries out
     *       window deletion once all other functions are 'done' so that other
     *       functions do not attempt to access destroyed window handles.
     */
    int _killWindow( void );

    /**
     * Wrapper for the draw callback.
     *
     * @note If the programmer wants something different to happen on draw
     *       requests other than simply the user-defined callback being called
     *       then she should subclass arGUIWindow and overload this function.
     *
     * @note in non-threaded mode the window's context is made current by this
     *       function.
     *
     * @see makeCurrent
     */
    virtual void _drawHandler( void );

    int _ID;                                    ///< A unique identifier for this window.
    string _className;                          ///< Registered class for this window (Win32 only)

    arGUIWindowConfig _windowConfig;            ///< The initial window configuration object, should never be changed.

    arGUIWindowHandle _windowHandle;            ///< The OS-specific window components

    arThread _windowEventThread;                ///< Thread in which the window exists in multi-threaded mode

    arGUIRenderCallback* _drawCallback;         ///< The user-defined draw callback.
    void (*_windowInitGLCallback)( arGUIWindowInfo* );      ///< The user-defined window opengl initialization callback.

    bool _visible;                              ///< Is the window currently visible?
    bool _running;                              ///< Is the window currently running?
    bool _threaded;                             ///< Is the window in threaded or non-threaded mode?
    bool _fullscreen;                           ///< Is the window currently in fullscreen mode?
    bool _topmost;                              ///< Is the window currently topmost?
    bool _decorate;                             ///< Is the window currently decorated?

    arCursor _cursor;                           ///< The current window cursor.

    ar_timeval _lastFrameTime;                  ///< For framerate throttling

    arGUIEventManager* _GUIEventManager;        ///< The window's arGUIEventManager.
    arGUIWindowBuffer* _windowBuffer;           ///< The window's arGUIWindowBuffer.

    arConditionVar _creationCond;               ///< Signaled when the window has been created.
    arConditionVar _destructionCond;            ///< Signaled when the window has been destroyed.
    arMutex _creationMutex, _destructionMutex;  ///< Mutexes for the condition variables.
    bool _creationFlag, _destructionFlag;       ///< Flags for the condition variables.

    EventQueue _WMEvents;                       ///< Queue of to-be-processed events received from the window manager.
    arMutex _WMEventsMutex;                     ///< Mutex protecting the window manager event queue.

    void* _userData;                            ///< User-set data pointer.


    /**
     * A queue of usable (and possible in-use) arWMEvents.
     *
     * This queue is necessary as both the window manager and the windows
     * themselves need a handle to added arWMEvents (the window manager to be
     * able to block on their completion if necessary and the window to be
     * able to signal such completion).  This queue allows several memory
     * management complications (i.e., who deletes such events and when) to be
     * neatly sidestepped.
     */
    EventVector _usableEvents;
    arMutex _usableEventsMutex;  ///< Mutex protecting the usable events queue.

};

#endif

