//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_APP_FRAMEWORK_H
#define AR_SZG_APP_FRAMEWORK_H

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
#include <set>

class SZG_CALL arSZGAppFramework {
  public:
    arSZGAppFramework();
    virtual ~arSZGAppFramework();
    
    virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    
    arSZGClient* getSZGClient() { return &_SZGClient; }
    string  getLabel(){ return _label; }
    bool getStandalone() const { return _standalone; }
    void setStandalone( bool onoff ) { _standalone = onoff; }

    virtual void setDataBundlePath(const string& bundlePathName,
                                   const string& bundleSubDirectory){}

    virtual void loadNavMatrix() = 0;
    
    void setEyeSpacing( float feet );
    void setClipPlanes( float near, float far );
    virtual void setFixedHeadMode(bool isOn) { _head.setFixedHeadMode(isOn); }
    virtual arMatrix4 getMidEyeMatrix() { return _head.getMidEyeMatrix(); }
    virtual arVector3 getMidEyePosition() { return _head.getMidEyePosition(); }
    virtual void setUnitConversion( float conv );
    virtual void setUnitSoundConversion( float conv );
    virtual float getUnitConversion();
    virtual float getUnitSoundConversion();
    const string getDataPath()
      { return _dataPath; }
    int getButton(       const unsigned int index ) const;
    float getAxis(       const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index, bool doUnitConversion=true ) const;
    bool getOnButton(    const unsigned int index ) const;
    bool getOffButton(   const unsigned int index ) const;
    unsigned int getNumberButtons()  const;
    unsigned int getNumberAxes()     const;
    unsigned int getNumberMatrices() const;
    bool setNavTransCondition( char axis,
                               arInputEventType type,
                               unsigned int index,
                               float threshold );
    bool setNavRotCondition( char axis,
                             arInputEventType type,
                             unsigned int index,
                             float threshold );
//    bool setWorldRotGrabCondition( arInputEventType type,
//                                   unsigned int index,
//                                   float threshold );
    void setNavTransSpeed( float speed );
    void setNavRotSpeed( float speed );
    void setNavEffector( const arEffector& effector );
    void ownNavParam( const std::string& paramName );
    void navUpdate();
    void navUpdate( arInputEvent& event );

    // should this return a copy instead? In some cases it points
    // inside the arInputNode
    arInputState* getInputState()
      { return (arInputState*)_inputState; }
      
    void setEventFilter( arFrameworkEventFilter* filter );
    void setEventCallback( arFrameworkEventCallback callback );

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
    
    // some applications need to be able to find out information
    // about the virtual computer
    arAppLauncher* getAppLauncher(){ return &_launcher; }

    // some applications want to do nonstandard things with the input node
    arInputNode* getInputNode(){ return _inputDevice; }
      
    // hide as soon as possible
    arSZGClient _SZGClient;

  protected:
    virtual bool _loadParameters() = 0;
    void _loadNavParameters();
    bool _parseNavParamString( const string& theString,
                               arInputEventType& type,
                               unsigned int& index,
                               float& threshold,
                               stringstream& initStream );
    bool _paramNotOwned( const std::string& theString );
    arInputNode* _inputDevice;
    arInputState* _inputState;
    string _label;
    arNetInputSource _netInputSource;
    arSoundServer _soundServer;
    arAppLauncher _launcher;
    bool _vircompExecution;
    arThread _messageThread;
    arThread _inputConnectionThread;
    
    // are we running in standalone mode?
    bool  _standalone;

    arCallbackEventFilter _callbackFilter;
    arFrameworkEventFilter* _eventFilter;
  
    string _dataPath;
    
    arHead _head;

    // the graphics unitConversion resides in the head now for convenience.
    float _unitSoundConversion;
    
    arNavManager _navManager;
    std::set< std::string > _ownedParams;

    // variables that have to do with deterministic shutdown

    // As a preliminary hack, the frameworks support a single, external
    // user thread, which can be deterministically shut down along with 
    // everything else.

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
};

#endif        //  #ifndefARSZGAPPFRAMEWORK_H

