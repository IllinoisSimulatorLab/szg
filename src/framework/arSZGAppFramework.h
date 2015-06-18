//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_APP_FRAMEWORK_H
#define AR_SZG_APP_FRAMEWORK_H

#include "arInputSimulator.h"
#include "arFramerateGraph.h"
#include "arSZGClient.h"
#include "arInputNode.h"
#include "arInputState.h"
#include "arNetInputSource.h"
#include "arInputFactory.h"
#include "arHead.h"
#include "arSoundServer.h"
#include "arAppLauncher.h"
#include "arNavManager.h"
#include "arNavigationUtilities.h"
#include "arFrameworkEventFilter.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arFrameworkCalling.h"

#include <set>
#include <deque>


//***********************************************************************
// Framework callback exception class (mainly for exception-handling
// in Python).
//***********************************************************************
class SZG_CALL arCallbackException {
  public:
    string message;
    arCallbackException( const string& msg ): message(msg) {}
};


class SZG_CALL arUserMessageInfo {
 public:
  arUserMessageInfo( int id, const string& body ):
    messageID( id ),
    messageBody( body )
    {}

  int messageID;
  string messageBody;
};

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

    // Methods common to all frameworks.

    virtual void setDataBundlePath( const string& /*bundlePath*/,
                                    const string& /*bundleSubDir*/ ) {}
    void autoDataBundlePath();
    virtual void loadNavMatrix() {}
    void speak( const std::string& message );
    bool setInputSimulator( arInputSimulator* sim );
    string getLabel() const { return _label; }
    bool getStandalone() const { return _standalone; }
    const string getDataPath() const { return _dataPath; }

    // Define the viewer, i.e., the user's eyes and ears.
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    arHead* getHead() { return &_head; }
    virtual void setFixedHeadMode(bool isOn) { _head.setFixedHeadMode(isOn); }
    virtual arMatrix4 getMidEyeMatrix() { return _head.getMidEyeMatrix(); }
    virtual arVector3 getMidEyePosition() { return _head.getMidEyePosition(); }
    virtual void setUnitConversion( float );
    virtual float getUnitConversion();

    // Access to the embedded input node.  Also see getInputNode().
    int getButton( const unsigned ) const;
    float getAxis( const unsigned ) const;
    arMatrix4 getMatrix( const unsigned, bool doUnitConversion=true ) const;
    bool getOnButton( const unsigned ) const;
    bool getOffButton( const unsigned ) const;
    // getNumberXXXs() are currently meaningful only on the master, not on slaves.
    // Todo: propagate signature from master to slaves, so e.g. utilities/vrtest.cpp
    // needn't do so manually through preExchange and postExchange.
    unsigned getNumberButtons()  const;
    unsigned getNumberAxes()     const;
    unsigned getNumberMatrices() const;

    // Methods for built-in navigation.

    bool setNavTransCondition( char axis,
                               arInputEventType type,
                               unsigned index,
                               float threshold );
    bool setNavRotCondition( char axis,
                             arInputEventType type,
                             unsigned index,
                             float threshold );
    void setNavTransSpeed( float );
    void setNavRotSpeed( float );
    void setNavEffector( const arEffector& );
    void ownNavParam( const std::string& paramName );
    void navUpdate();
    void navUpdate( const arInputEvent& event );
    void navUpdate( const arMatrix4& navMatrix );
    void setUseNavInputMatrix( const bool onoff ) { _useNavInputMatrix = onoff; }
    void setNavInputMatrixIndex( const unsigned index ) { _navInputMatrixIndex = index; }
    void setUnitConvertNavInputMatrix( const bool onoff ) { _unitConvertNavInputMatrix = onoff; }

    // Methods for event filtering (and the callbacks that allow
    // event-based processing instead of polling-based processing).

    bool setEventFilter( arFrameworkEventFilter* filter );
    void setEventCallback( arFrameworkEventCallback callback );
    virtual void setEventQueueCallback( arFrameworkEventQueueCallback callback );
    void processEventQueue();
    virtual void onProcessEventQueue( arInputEventQueue& theQueue );
    virtual bool onInputEvent( arInputEvent& /*event*/, arFrameworkEventFilter& /*filter*/ ) {
      return true;
    }

    virtual void onUserMessage( const int /*messageID*/, const string& /*messageBody*/ ) {}

    // Should this return a copy instead? In some cases it points
    // inside the arInputNode.
    arInputState* getInputState() { return (arInputState*)_inputState; }

    // This used to be in arDistSceneGraphFramework. Moved it up here
    // for the arMasterSlaveFramework sound database.
    const string getNavNodeName() const { return "SZG_NAV_MATRIX"; }

    // Some apps need a thread running external to the library.
    // For deterministic shutdown, we need to register that
    // thread's existence, know when it is shutting down, etc.

    // If an external event loop wants to call exit(),
    // it should wait until stopped() returns true, i.e. stop() has completed.
    bool stopped() const { return _stopped; }

    // True iff shutdown has begun.
    bool stopping() const { return _exitProgram; }

    void useExternalThread() { _useExternalThread = true; }

    // Tell stop() that external thread has started.
    void externalThreadStarted() { _externalThreadRunning = true; }

    // Tell stop() that external thread stopped cleanly.
    void externalThreadStopped() { _externalThreadRunning = false; }

    // Accessors for various internal objects/services. Necessary for flexibility.

    // Info about the virtual computer.
    arAppLauncher* getAppLauncher() { return &_launcher; }
    // For nonstandard use of the input node.
    arInputNode* getInputNode() { return _inputNode; }
    arInputNode* getInputDevice() { 
      ar_log_warning() << "getInputDevice() deprecated, please use getInputNode().\n";
      return getInputNode();  // Deprecated version of getInputNode()
    }
    // Allowing the user access to the window manager increases the flexibility
    // of the framework. Lots of info about the GUI becomes available.
    arGUIWindowManager* getWindowManager( void ) const { return _wm; }
    // Some applications want to be able to work with the arSZGClient directly.
    arSZGClient* getSZGClient() { return &_SZGClient; }

    // Add entries to the data bundle path (used to locate texture maps by
    // szgrender for scene-graph apps in cluster mode and by SoundRender to
    // locate sounds for both types of apps in cluster mode).
    virtual void addDataBundlePathMap(const string& /*bundlePathName*/,
                            const string& /*bundlePath*/) {}

    bool _onInputEvent( arInputEvent& event, arFrameworkEventFilter& filter ) {
      _inOnInputEvent = true;
      bool stat = onInputEvent( event, filter );
      _inOnInputEvent = false;
      return stat;
    }

    void postInputEventQueue( arInputEventQueue& q );
    void postInputEvent( arInputEvent& event );


  protected:
    arSZGClient _SZGClient;
    arInputNode* _inputNode;
    arInputState* _inputState;
    string _label;
    arNetInputSource _netInputSource;
    arSoundServer _soundServer;
    arAppLauncher _launcher;
    bool _vircompExecution;
    arThread _messageThread;
    arThread _inputConnectionThread;

    // For standalone.
    bool              _standalone;
    std::string       _standaloneControlMode;
    arInputSimulator  _simulator;
    arInputSimulator* _simPtr;
    bool              _showSimulator;
    arFramerateGraph  _framerateGraph;
    bool              _showPerformance;
#if defined( AR_USE_MINGW ) || defined( AR_LINKING_DYNAMIC) || !defined( AR_USE_WIN_32 )
    arInputFactory    _inputFactory;
#endif

    arCallbackEventFilter _callbackFilter;
    arFrameworkEventQueueCallback _eventQueueCallback;
    arFrameworkEventFilter _defaultUserFilter;
    arFrameworkEventFilter* _userEventFilter;
    bool _inOnInputEvent;

    std::deque< arUserMessageInfo > _userMessageQueue;
    arLock _userMessageLock;

    string _dataPath;
    bool _dataBundlePathSet;
    arHead _head;
    // Graphics unitConversion is now in the head, for convenience.
    float _unitSoundConversion;
    int _speechNodeID;
    arNavManager _navManager;
    std::set< std::string > _ownedParams;
    bool _useNavInputMatrix;
    unsigned _navInputMatrixIndex;
    bool _unitConvertNavInputMatrix;

    // For standalone and for masterslave.
    arGUIWindowManager* _wm;

    // Only for masterslave, not scenegraph.
    arGUIXMLParser* _guiXMLParser;

    // Have init() and start() been called?
    bool _initCalled;
    bool _startCalled;
    // Have the parameters been loaded?
    bool _parametersLoaded;

    // One external app-defined thread can be
    // deterministically shut down along with everything else.

    // Is there an app-defined thread?
    bool _useExternalThread;
    // Is that thread running? (methods externalThreadRunning()
    // and externalThreadStopped() let the app tell this to the framework.
    bool _externalThreadRunning;
    // Should stop() block until the GLUT display thread completes?
    bool _blockUntilDisplayExit;
    // Has stop() commenced, i.e. is the app exiting?
    bool _exitProgram;
    // Is the display loop running?
    bool _displayThreadRunning;
    // Has stop() completed?  Might be needed if the app-defined thread calls exit(0).
    bool _stopped;

    void _handleStandaloneInput();
    bool _loadInputDrivers();

    // Install _defaultUserFilter and _callbackFilter
    // (which initially does nothing, as it has no callback).
    // Any user-installed filter replaces _defaultUserFilter (installing a NULL
    // filter restores it), and user-installed event callback gets put in _callbackFilter.
    // NOTE: as of 1.2, _defaultUserFilter now calls framework->onInputEvent by default
    // (which does nothing, override in subclasses). No more steenkin' callbacks!
    void _installFilters();
    virtual bool _loadParameters() = 0;
    void _loadNavParameters();
    void _appendUserMessage( int messageID, const std::string& messageBody );
    bool _parseNavParamString( const string& theString,
                               arInputEventType& type,
                               unsigned& index,
                               float& threshold );
    bool _paramNotOwned( const std::string& theString );

    bool _okToInit(const char*);
    bool _okToStart() const;

  private:
    bool _checkInput() const;
    void _setNavTransSpeed( float );
    void _setNavRotSpeed( float );
    void _setNavEffector( const arEffector& );
};

#endif        //  #ifndefAR_SZG_APP_FRAMEWORK_H
