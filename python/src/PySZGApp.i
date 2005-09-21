//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// **************** based on arSZGAppFramework.h *******************

%{
#include "arSZGAppFramework.h"
%}

class arSZGAppFramework {
  public:
    arSZGAppFramework();
    virtual ~arSZGAppFramework();
    
    virtual bool init(int& argc, char** argv ) = 0;
    virtual bool start() = 0;
    virtual void stop(bool blockUntilDisplayExit) = 0;
    
    string  getLabel(){ return _label; }
    bool getStandalone() const;
    arSZGClient* getSZGClient();

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
    arMatrix4 getMatrix( const unsigned int index ) const;
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

    arInputState* getInputState()
      { return (arInputState*)_inputState; }
      
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
};



