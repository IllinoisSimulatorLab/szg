//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_APP_FRAMEWORK_H
#define AR_SZG_APP_FRAMEWORK_H

#include "arInputSimulator.h"
#include "arFramerateGraph.h"
#include "arThread.h"
#include "arSZGClient.h"
#include "arInputNode.h"
#include "arInputState.h"
#include "arNetInputSource.h"
#include "arHead.h"
#include "arSoundServer.h"
#include "arAppLauncher.h"
#include "arNavManager.h"
#include "arNavigationUtilities.h"
#include "arFrameworkEventFilter.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include <set>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arFrameworkCalling.h"

class SZG_CALL arSZGAppFramework {
  public:
    arSZGAppFramework();
    virtual ~arSZGAppFramework();
    
    // Event loop management and related methods.
    virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    virtual bool createWindows(bool useWindowing) = 0;
    virtual void loopQuantum() = 0;
    virtual void exitFunction() = 0;
    
    // Misc. methods. common to all frameworks.
    virtual void setDataBundlePath(const string&, const string&){}
    virtual void loadNavMatrix(){}
    void speak( const std::string& message );
    bool setInputSimulator( arInputSimulator* sim );
    string getLabel(){ return _label; }
    bool getStandalone() const { return _standalone; }
    const string getDataPath(){ return _dataPath; }
        
    // Set-up the viewer (i.e. the user's head).
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    arHead* getHead(){ return &_head; }
    virtual void setFixedHeadMode(bool isOn) { _head.setFixedHeadMode(isOn); }
    virtual arMatrix4 getMidEyeMatrix() { return _head.getMidEyeMatrix(); }
    virtual arVector3 getMidEyePosition() { return _head.getMidEyePosition(); }
    virtual void setUnitConversion( float conv );
    virtual void setUnitSoundConversion( float conv );
    virtual float getUnitConversion();
    virtual float getUnitSoundConversion();
      
    // Basic access to the embedded input node. Note: It is also possible to
    // access the input node directly (see below).
    int getButton( const unsigned int index ) const;
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index, bool doUnitConversion=true ) const;
    bool getOnButton( const unsigned int index ) const;
    bool getOffButton( const unsigned int index ) const;
    unsigned int getNumberButtons()  const;
    unsigned int getNumberAxes()     const;
    unsigned int getNumberMatrices() const;
    
    // Methods pertaining to the built-in navigation.
    bool setNavTransCondition( char axis,
                               arInputEventType type,
                               unsigned int index,
                               float threshold );
    bool setNavRotCondition( char axis,
                             arInputEventType type,
                             unsigned int index,
                             float threshold );
    void setNavTransSpeed( float speed );
    void setNavRotSpeed( float speed );
    void setNavEffector( const arEffector& effector );
    void ownNavParam( const std::string& paramName );
    void navUpdate();
    void navUpdate( arInputEvent& event );
      
    // Methods pertaining to event filtering (and the callbacks that allow
    // event-based processing instead of polling-based processing).
    bool setEventFilter( arFrameworkEventFilter* filter );
    void setEventCallback( arFrameworkEventCallback callback );
    virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );
    void processEventQueue();
    virtual void onProcessEventQueue( arInputEventQueue& theQueue );
    // Should this return a copy instead? In some cases it points
    // inside the arInputNode.
    arInputState* getInputState(){ return (arInputState*)_inputState; }

    // Some applications need a thread running external to the library.
    // For deterministic shutdown, we need to be able to register that
    // thread's existence, know when it is shutting down, etc.

    /// In the case of a user-defined, external event loop, the external
    /// program may need to know when the "stop" signal has been received.
    /// Returns whether or not the shutdown process has begun.
    bool stopping(){ return _exitProgram; }
    /// in the case of a user-defined external event loop, we may want the 
    /// exit(0) call to be made by the user code. The following function lets 
    /// the user code know that stop(...) is done. 
    /// Returns whether or not stop(...) has completed.
    bool stopped(){ return _stopped; }
    // sometimes we want to use an external thread or other event loop that
    // must be shutdown cleanly
    void useExternalThread(){ _useExternalThread = true; }
    // tells stop() that our external thread has started
    void externalThreadStarted(){ _externalThreadRunning = true; }
    // tells stop() that our external thread has shut-down cleanly
    void externalThreadStopped(){ _externalThreadRunning = false; }

    // Accessors for various internal objects/services. Necessary for flexibility.
    
    // Some applications need to be able to find out information
    // about the virtual computer.
    arAppLauncher* getAppLauncher(){ return &_launcher; }
    // Some applications want to do nonstandard things with the input node.
    arInputNode* getInputNode(){ return _inputDevice; }
    // Needed for backwards compatibility, even though it does the same thing
    // as the above.
    arInputNode* getInputDevice(){ return _inputDevice; }
    // Allowing the user access to the window manager increases the flexibility
    // of the framework. Lots of info about the GUI becomes available.
    arGUIWindowManager* getWindowManager( void ) { return _wm; }
    // Some applications want to be able to work with the arSZGClient directly.
    arSZGClient* getSZGClient() { return &_SZGClient; }
      
  protected:
    arSZGClient _SZGClient;
    arInputNode* _inputDevice;
    arInputState* _inputState;
    string _label;
    arNetInputSource _netInputSource;
    arSoundServer _soundServer;
    arAppLauncher _launcher;
    bool _vircompExecution;
    arThread _messageThread;
    arThread _inputConnectionThread;
        
    // Used in standalone mode.
    bool              _standalone;
    std::string       _standaloneControlMode;
    arInputSimulator  _simulator;
    arInputSimulator* _simPtr;
    bool              _showSimulator;
    arFramerateGraph  _framerateGraph;
    bool              _showPerformance;

    arCallbackEventFilter _callbackFilter;
    arFrameworkEventQueueCallback _eventQueueCallback;
    arFrameworkEventFilter _defaultUserFilter;
    arFrameworkEventFilter* _userEventFilter;
  
    // Misc. member variables.
    string _dataPath;
    arHead _head;
    // The graphics unitConversion resides in the head now for convenience.
    float _unitSoundConversion;
    int _speechNodeID;
    arNavManager _navManager;
    std::set< std::string > _ownedParams;
    
    // Standalone mode requires a window manager. Master/slave framework requires
    // a window manager both in standalone and phleet modes.
    arGUIWindowManager* _wm;
    // The scene graph framework does not use this parser, but the m/s framework does.
    arGUIXMLParser* _guiXMLParser;
    
    // Various book-keeping flags.
    // Keep track of when the init and start methods have been successfully called.
    // This lets us tell the user if they are called in the incorrect order.
    bool _initCalled;
    bool _startCalled;
    // Have the parameters been loaded?
    bool _parametersLoaded;

    // Variables that have to do with deterministic shutdown.
    // The frameworks support a single, external user thread, which can be 
    // deterministically shut down along with everything else.

    // Is there an application-defined thread?
    bool _useExternalThread;
    // Is that thread running? (the methods externalThreadRunning()
    // and externalThreadStopped() allow the application to communicate
    // this to the framework.
    bool _externalThreadRunning;
    // should stop(...) block until the GLUT display thread is done?
    bool _blockUntilDisplayExit;
    // Set to true when we are intending to exit. i.e. when stop(...) has
    // started.
    bool _exitProgram;
    // Set to true when the GLUT display loop is going.
    bool _displayThreadRunning;
    // Set to true when stop(...) has completed. Might be needed if the
    // exit(0) is going to occur in an application-defined thread.
    bool _stopped;
    
    // Installs two filters: _defaultUserFilter (which does nothing, it's a placeholder)
    // and _callbackFilter (which initially does nothing, as it has no callback).
    // Any user-installed filter replaces _defaultUserFilter (installing a NULL
    // filter restores it), and user-installed event callback gets put in _callbackFilter.
    void _installFilters();
    virtual bool _loadParameters() = 0;
    void _loadNavParameters();
    bool _parseNavParamString( const string& theString,
                               arInputEventType& type,
                               unsigned int& index,
                               float& threshold );
    bool _paramNotOwned( const std::string& theString );
    
};

#endif        //  #ifndefAR_SZG_APP_FRAMEWORK_H

