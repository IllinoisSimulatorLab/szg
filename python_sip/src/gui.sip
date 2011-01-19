// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include math.sip
%Include szgclient.sip

// global extra code
%ModuleHeaderCode
#include "arGUIDefines.h"
#include <string>
using namespace std;
%End

enum arGUIEventType
{
  AR_GENERIC_EVENT,         // Placeholder event (for default constructors).
  AR_KEY_EVENT,             // A keyboard event.
  AR_MOUSE_EVENT,           // A mouse event.
  AR_WINDOW_EVENT,          // A window event.
  AR_NUM_GUI_EVENTS         // The number of different OS events.
};

/**
 * States that an OS event can be in.
 *
 * Also used as the 'to-do' message detail in passing requests from
 * arGUIWindowManager to arGUIWindow (i.e., in this case AR_WINDOW_MOVE means
 * "move the window" rather than its normal "the window has moved").
 */
enum arGUIState
{
  AR_GENERIC_STATE,         // Placeholder state (for default constructors).
  AR_KEY_DOWN,              // A key has been pressed.
  AR_KEY_UP,                // A key has been released.
  AR_KEY_REPEAT,            // A key was pressed and is continuing to be pressed.
  AR_MOUSE_DOWN,            // A mouse button was pressed.
  AR_MOUSE_UP,              // A mouse button was released.
  AR_MOUSE_MOVE,            // The mouse has moved.
  AR_MOUSE_DRAG,            // A mouse button is held down and the mouse is being moved.
  AR_WINDOW_MOVE,           // The window has been moved.
  AR_WINDOW_RESIZE,         // The window has been resized.
  AR_WINDOW_CLOSE,          // The window has been closed.
  AR_WINDOW_FULLSCREEN,     // Change the window to fullscreen.
  AR_WINDOW_DECORATE,       // Change the window's decoration state.
  AR_WINDOW_RAISE,          // Change the window's z order.
  AR_WINDOW_CURSOR,         // Change the window's cursor.
  AR_WINDOW_DRAW_LEFT,           // Draw the window (GL_BACK_LEFT)
  AR_WINDOW_DRAW_RIGHT,           // Draw the window (GL_BACK_RIGHT).
  AR_WINDOW_SWAP,           // Swap the window's buffers.
  AR_WINDOW_VIEWPORT,       // Set the window's viewport.
  AR_WINDOW_INITGL,         // Initialize the window's opengl context.
  AR_NUM_GUI_STATES         // The number of different event states.
};


class arGUIInfo {
%TypeHeaderCode
#include "arGUIInfo.h"
%End
  public:

    // eventType Which type of event this is.
    // state     What state the event is in.
    // windowID  Which window this event took place in.
    // flag      Generic flag for function specific information.
    // userData  User-defined data pointer.
    arGUIInfo( arGUIEventType eventType = AR_GENERIC_EVENT,
               arGUIState state = AR_GENERIC_STATE,
               int windowID = -1, int flag = 0 );
    //arGUIInfo( arStructuredData& data );

    void setEventType( arGUIEventType eventType );
    arGUIEventType getEventType() const;

    void setState( arGUIState state );
    arGUIState getState() const;

    void setWindowID( int windowID );
    int getWindowID() const;

    void setFlag( int flag );
    int getFlag() const;

    //void setUserData( void* userData );
    //void* getUserData( void ) const;

    //void setWindowManager( arGUIWindowManager* wm);
    //arGUIWindowManager* getWindowManager( void ) const;
};


class arGUIKeyInfo : arGUIInfo {
%TypeHeaderCode
#include "arGUIInfo.h"
%End
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
                  unsigned int key = AR_VK_GARBAGE,
                  int ctrl = 0, int alt = 0 );
//    arGUIKeyInfo( arStructuredData& data );

    // Accessors.
    void setKey( unsigned int key );
    unsigned int getKey() const;

    void setCtrl( int ctrl );
    int getCtrl() const;

    void setAlt( int alt );
    int getAlt() const;

    string __str__();
%MethodCode
      ostringstream s;
      s << "arGUIKeyInfo ";
      s << "ID=" << sipCpp->getWindowID();
      s << ", State=" << sipCpp->getState();
      s << ", Key=(" << sipCpp->getKey();
      s << "), CTRL=(" << sipCpp->getCtrl() << "), ALT=(" << sipCpp->getAlt() << ")";
      sipRes = new string( s.str() );
%End
};


class arGUIMouseInfo : arGUIInfo {
%TypeHeaderCode
#include "arGUIInfo.h"
%End
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
                    unsigned int button = AR_BUTTON_GARBAGE,
                    int posX = -1, int posY = -1,
                    int prevPosX = -1, int prevPosY = -1 );
  //  arGUIMouseInfo( arStructuredData& data );

    // Accessors.
    void setButton( unsigned int button );
    unsigned int getButton() const;

    void setPosX( int posX );
    void setPosY( int posY );
    void setPos( int posX, int posY );

    void setPrevPosX( int prevPosX );
    void setPrevPosY( int prevPosY );
    void setPrevPos( int prevPosX, int prevPosY );

    int getPosX() const;
    int getPosY() const;

    int getPrevPosX() const;
    int getPrevPosY() const;
};


class arGUIWindowInfo : arGUIInfo {
%TypeHeaderCode
#include "arGUIInfo.h"
%End
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

    //arGUIWindowInfo( arStructuredData& data );

    // Accessors.
    void setPosX( int posX );
    void setPosY( int posY );
    void setPos( int posX, int posY );

    void setSizeX( int sizeX );
    void setSizeY( int sizeY );
    void setSize( int sizeX, int sizeY );

    int getPosX() const;
    int getPosY() const;

    int getSizeX() const;
    int getSizeY() const;

    arGUIWindowManager* getWindowManager() const;

    string __str__();
%MethodCode
      ostringstream s;
      s << "arGUIWindowInfo ";
      s << "ID=" << sipCpp->getWindowID();
      s << ", State=" << sipCpp->getState();
      s << ", Position=(" << sipCpp->getPosX() << "," << sipCpp->getPosY();
      s << "), Size=(" << sipCpp->getSizeX() << "," << sipCpp->getSizeY() << ")";
      sipRes = new string( s.str() );
%End
};


//const unsigned int AR_CURSOR_ARROW    = 0x0000;
//const unsigned int AR_CURSOR_HELP     = 0x0001;
//const unsigned int AR_CURSOR_WAIT     = 0x0002;
//const unsigned int AR_CURSOR_NONE     = 0x0003;
//const unsigned int AR_ZORDER_NORMAL   = 0x0000;
//const unsigned int AR_ZORDER_TOP      = 0x0001;
//const unsigned int AR_ZORDER_TOPMOST  = 0x0002;

class arGUIWindowConfig {
%TypeHeaderCode
#include "arGUIWindow.h"
%End
 public:
   arGUIWindowConfig( int x = 50, int y = 50, 
                       int width = 640, int height = 480,
                       int bpp = 16, int Hz = 0, bool decorate = true,
                       unsigned int zorder = AR_ZORDER_TOP,
                       bool fullscreen = false, bool stereo = false,
                       const string& title = "Syzygy",
                       const string& XDisplay = ":0.0",
                       unsigned int cursor = AR_CURSOR_ARROW );

   ~arGUIWindowConfig();

   void setPos( int x, int y );
   void setPosX( int x );
   void setPosY( int y );
   void setWidth( int width );
   void setHeight( int height );
   void setSize( int width, int height );
   void setBpp( int bpp );
   void setHz( int Hz );
   void setDecorate( bool decorate );
   void setFullscreen( bool fullscreen );
   void setZOrder( unsigned int zorder );
   void setTitle( const string& title );
   void setXDisplay( const string& XDisplay );
   void setCursor( unsigned int cursor );

   int getPosX() const;
   int getPosY() const;
   int getWidth() const;
   int getHeight() const;
   int getBpp() const;
   int getHz() const;
   bool getDecorate() const;
   bool getFullscreen() const;
   bool getZOrder() const;
   string getTitle() const;
   string getXDisplay() const;
   unsigned int getCursor() const;

  string __str__();
%MethodCode
    ostringstream s;
    s << "arGUIWindowconfig\n";
    s << "position: (" << sipCpp->getPosX() << " " << sipCpp->getPosY() << ")\n";
    s << "size:     (" << sipCpp->getWidth() << " " 
      << sipCpp->getHeight() << ")\n";
    s << "bit depth: " << sipCpp->getBpp() << "\n";
    s << "decoration: " << sipCpp->getDecorate() << "\n";
    s << "zorder: " << sipCpp->getZOrder() << "\n";
    s << "title: " << sipCpp->getTitle() << "\n";
    sipRes = new string( s.str() );
%End

  private:
    arGUIWindowConfig( const arGUIWindowConfig& );
};

%Include view.sip

class arGUIRenderCallback : arRenderCallback {
%TypeHeaderCode
#include "arGUIWindow.h"
%End
  public:
    arGUIRenderCallback();
    virtual ~arGUIRenderCallback();
    virtual void operator()( int guiWinID,
                             arGraphicsWindow* graphicsWindow,
                             const bool drawLeftBuffer ) = 0;
    virtual void operator()( arGUIWindowInfo* windowInfo ) = 0;
    virtual void operator()( arGraphicsWindow&, arViewport& ) = 0;
  private:
    arGUIRenderCallback( const arGUIRenderCallback& );
};

class arGUIWindow {
%TypeHeaderCode
#include "arGUIWindow.h"
%End
 public:
  arGUIWindow( int ID, arGUIWindowConfig windowConfig);
  virtual ~arGUIWindow();
  void registerDrawCallback( arGUIRenderCallback* drawCallback /TransferBack/ );
  int beginEventThread();
  int swap();
  int resize( int newWidth, int newHeight );
  int move( int newX, int newY );
  int setViewport( int newX, int newY, int newWidth, int newHeight );
  int fullscreen();
  int makeCurrent( bool release = false );
  void minimize();
  void restore();
  void decorate( const bool decorate );
  unsigned int setCursor( unsigned int cursor );
  void setVisible( const bool visible );
  bool getVisible() const;
  string getTitle() const;
  void setTitle( const string& title );
  int getID() const;
  int getWidth() const;
  int getHeight() const;
  int getPosX() const;
  int getPosY() const;
  bool isStereo()      const;
  bool isFullscreen()  const;
  bool isDecorated()   const;
  unsigned int getZOrder() const;
  bool running() const;
  bool eventsPending() const;
  unsigned int getCursor() const;
  int getBpp() const;
  const arGUIWindowConfig& getWindowConfig() const;
  arGraphicsWindow* getGraphicsWindow();
  void returnGraphicsWindow();
  // arGraphicsWindow seems to expect to own the passed window
  void setGraphicsWindow( arGraphicsWindow* graphicsWindow /TransferThis/ );
 private:
  arGUIWindow( const arGUIWindow& );
};


class arGUIWindowManager {
%TypeHeaderCode
#include "arGUIWindowManager.h"
%End
 public:
  //arGUIWindowManager( void (*windowCallback)( arGUIWindowInfo* ) = NULL,
  //                    void (*keyboardCallback)( arGUIKeyInfo* ) = NULL,
  //                    void (*mouseCallback)( arGUIMouseInfo* ) = NULL,
  //                    void (*windowInitGLCallback)( arGUIWindowInfo* ) = NULL,
  //                    bool threaded = true );
  arGUIWindowManager();
  virtual ~arGUIWindowManager();
  int startWithSwap();
  int startWithoutSwap();
  int addWindow( const arGUIWindowConfig& windowConfig,
                 bool useWindowing = true );
  //int createWindows(const arGUIWindowingConstruct* windowingConstruct=NULL,  
  //                  bool useWindowing = true );
  //void registerWindowCallback( void (*windowCallback) ( arGUIWindowInfo* ) );
  //void registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) );
  //void registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) );
  //void registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) );
  //int registerDrawCallback( const int windowID, 
  //                          arGUIRenderCallback* drawCallback );
  int processWindowEvents();
  arGUIInfo* getNextWindowEvent( const int windowID );
  int drawWindow( const int windowID, bool blocking = false );
  int drawAllWindows( bool blocking = false );
  int consumeWindowEvents( const int windowID, bool blocking = false );
  int consumeAllWindowEvents( bool blocking = false );
  int swapWindowBuffer( const int windowID, bool blocking = false );
  int swapAllWindowBuffers( bool blocking = false );
  int resizeWindow( const int windowID, int width, int height );
  int moveWindow( const int windowID, int x, int y );
  int setWindowViewport( const int windowID, int x, int y, int width, int height );
  int fullscreenWindow( const int windowID );
  int decorateWindow( const int windowID, bool decorate );
  int setWindowCursor( const int windowID, unsigned int cursor );
  int raiseWindow( const int windowID, unsigned int zorder );
  bool windowExists( const int windowID );
  arVector3 getWindowSize( const int windowID ) /TransferBack/;
  arVector3 getWindowPos( const int windowID ) /TransferBack/;
  arVector3 getMousePos( const int windowID ) /TransferBack/;
  unsigned int getWindowCursor( const int windowID );
  bool isStereo( const int windowID );
  bool isFullscreen( const int windowID );
  bool isDecorated( const int windowID );
  unsigned int getZOrder( const int windowID );
  int getBpp( const int windowID );
  string getTitle( const int windowID );
  string getXDisplay( const int windowID );
  void setTitle( const int windowID, const string& title );
  void setAllTitles( const string& baseTitle, bool overwrite=true );
  arGraphicsWindow* getGraphicsWindow( const int windowID );
  void returnGraphicsWindow( const int windowID );
  void setGraphicsWindow( const int windowID, arGraphicsWindow* graphicsWindow /TransferThis/ );
  int getNumWindows() const;
  bool hasActiveWindows() const;
  bool isFirstWindow( const int windowID ) const;
  int getFirstWindowID() const;
  bool isThreaded() const;
  void setThreaded( bool threaded );
  int deleteWindow( const int windowID );
  int deleteAllWindows();
 private:
  arGUIWindowManager( const arGUIWindowManager& );
};


