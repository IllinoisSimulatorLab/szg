//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GUI_COMPONENT_FRAMEWORK
#define AR_GUI_COMPONENT_FRAMEWORK

#include "arDataServer.h"
#include "arDataClient.h"
#include "arDataUtilities.h"
#include "arGraphicsAPI.h"
#include "arGraphicsWindow.h"
#include "arSoundClient.h"
#include "arVRCamera.h"
#include "arGUIInfo.h"
#include "arFrameworkCalling.h"

#include <vector>

class arGUIXMLParser;

// Framework for cluster applications using one master and several slaves.
class SZG_CALL arGUIComponentFramework : public arSZGAppFramework {
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_guiComponentFrameworkMessageTask( void* );
  friend void ar_guiComponentFrameworkWindowEventFunction( arGUIWindowInfo* );
  friend void ar_guiComponentFrameworkWindowInitGLFunction( arGUIWindowInfo* );
  friend void ar_guiComponentFrameworkKeyboardFunction( arGUIKeyInfo* );
  friend void ar_guiComponentFrameworkMouseFunction( arGUIMouseInfo* );

  friend class arGUIComponentWindowInitCallback;
  friend class arGUIComponentRenderCallback;
 public:
  arGUIComponentFramework( void );
  virtual ~arGUIComponentFramework( void );

  // Initializes the various objects but does not start the event loop.
  bool init( int&, char** );

  // Start services, maybe windowing, and maybe an internal event loop.
  // Returns only if useEventLoop is false, or on error.
  // If useEventLoop is false, caller should run the event loop either
  // coarsely via loopQuantum() or finely via preDraw(), sync(), etc.
  // Two functions, because default args don't jive with virtual arSZGAppFramework::start(void).
  bool start();
  bool start(bool useWindowing, bool useEventLoop);

  // Shutdown for many arGUIComponentFramework services.
  // Optionally block until the display thread exits.
  void stop( bool blockUntilDisplayExit );

  void setGraphicsMode( const string& graphicsMode ) {
    _SZGClient.setMode( "graphics", graphicsMode );
  }
  bool createWindows(bool useWindowing);
  void loopQuantum();
  void exitFunction();
  virtual void preDraw( void );
  // Different than onDraw (and the corresponding draw callbacks).
  // Essentially causes the window manager to draw all the windows.
  void draw( int windowID = -1 );
  virtual void sync( void );
  void swap( int windowID = -1 );

  // Another layer of indirection to promote object-orientedness.
  // We MUST have callbacks to subclass this object in Python.
  // At each point the callbacks were formerly invoked, the code instead calls
  // these virtual (hence overrideable) methods, which in turn invoke the callbacks.
  // This allows subclassing instead of callbacks.
  // (If you override these, the corresponding callbacks are of course ignored).

  // Convenient options for arSZGClient.
  virtual bool onStart( arSZGClient& SZGClient );
  virtual void onWindowStartGL( arGUIWindowInfo* );
  virtual void onWindowInit( void );
  virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
  virtual void onWindowEvent( arGUIWindowInfo* );
  virtual void onCleanup( void );
  virtual void onUserMessage( const int messageID, const string& messageBody );
  virtual void onOverlay( void );
  virtual void onKey( unsigned char key, int x, int y );
  virtual void onKey( arGUIKeyInfo* );
  virtual void onMouse( arGUIMouseInfo* );

  // Set the callbacks
  void setStartCallback( bool (*startCallback)( arGUIComponentFramework& fw, arSZGClient& ) );
  void setWindowStartGLCallback( void (*windowStartGL)( arGUIComponentFramework&, arGUIWindowInfo* ) );
  void setWindowCallback( void (*windowCallback)( arGUIComponentFramework& ) );
  void setDrawCallback( void (*draw)( arGUIComponentFramework&, arGraphicsWindow&, arViewport& ) );
  void setWindowEventCallback( void (*windowEvent)( arGUIComponentFramework&, arGUIWindowInfo* ) );
  void setExitCallback( void (*cleanup)( arGUIComponentFramework& ) );
  void setUserMessageCallback( void (*userMessageCallback)( arGUIComponentFramework&,
        const int messageID, const std::string& messageBody ) );
  void setOverlayCallback( void (*overlay)( arGUIComponentFramework& ) );
  // New-style keyboard callback.
  void setKeyboardCallback( void (*keyboard)( arGUIComponentFramework&, arGUIKeyInfo* ) );
  void setMouseCallback( void (*mouse)( arGUIComponentFramework&, arGUIMouseInfo* ) );
  // Tiny functions that only appear in the .h

  // msec since the first I/O poll (not quite start of the program).
  double getTime( void ) const { return _time; }

  // msec taken to compute/draw the last frame.
  double getLastFrameTime( void ) const { return _lastFrameTime; }

 protected:
  // Objects that provide services.
  // Must be pointers, so languages can initialize.  Really?

  // Misc. variables follow.
  // Holds the head position for the spatialized sound API.
  std::string _networks;
  // Used by (for instance) by the arBarrierServer to coordinate with the
  // arDataServer.
  arSignalObject _swapSignal;
  // Information about the various threads that are unique to the
  // arGUIComponentFramework (some info about the status of thread types
  // shared with other frameworks is in arSZGAppFramework.h)
  bool _useWindowing;

  // Callbacks.
  bool (*_startCallback)( arGUIComponentFramework&, arSZGClient& );
  void (*_windowInitCallback)( arGUIComponentFramework& );
  void (*_drawCallback)( arGUIComponentFramework& fw, arGraphicsWindow& win, arViewport& vp );
  void (*_windowEventCallback)( arGUIComponentFramework&, arGUIWindowInfo* );
  void (*_windowStartGLCallback)( arGUIComponentFramework&, arGUIWindowInfo* );
  void (*_cleanup)( arGUIComponentFramework& );
  void (*_userMessageCallback)( arGUIComponentFramework&,
      const int messageID, const std::string& messageBody );
  void (*_overlay)( arGUIComponentFramework& );
  void (*_keyboardCallback)( arGUIComponentFramework&, unsigned char key, int x, int y );
  void (*_arGUIKeyboardCallback)( arGUIComponentFramework&, arGUIKeyInfo* );
  void (*_mouseCallback)( arGUIComponentFramework&, arGUIMouseInfo* );

  // Time variables
  ar_timeval _startTime;
  double     _time;
  double     _lastFrameTime; // msec
  double     _lastComputeTime; // usec
  double     _lastSyncTime; // usec
  bool       _firstTimePoll;

  // Variable related to the "delay" message. Artificial slowdown.
  // the framerate.
  bool  _framerateThrottle;

  // It turns out that reloading parameters must occur in the main event
  // thread. This is because we allow the window manager to be single
  // threaded... and, in this case, all window manager calls should occur
  // in one thread only. (in response to the reload message)
  bool _requestReload;

  // Small utility functions.
  void _stop( void );
  bool _sync( void );

  // Graphics utility functions.
  bool _createWindowing( bool useWindowing );

  // Functions pertaining to initing/starting services.
  bool _start( bool useWindowing, bool useEventLoop );
  bool _startrespond( const std::string& s );

  // stop() when a callback has an error.
  void _stop(const char*, const arCallbackException&);

  // Systems level functions.
  bool _loadParameters( void );
  void _messageTask( void );

  // User-message functions
  void _processUserMessages();

  // Draw-related utility functions.
  void _drawWindow( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow );
};

#endif
