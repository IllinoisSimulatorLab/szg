// Defines mapping from python to C++ strings.onButtonEvent( arInputEvent& event, 
%Include typemaps.sip

%ModuleHeaderCode
#include <string>
using namespace std;
%End

// The order button, axis, matrix is important.  Arrays index by this.
enum arInputEventType {
  AR_EVENT_GARBAGE=-1,
  AR_EVENT_BUTTON,
  AR_EVENT_AXIS,
  AR_EVENT_MATRIX
};

class arInputEvent {
%TypeHeaderCode
#include "arInputEvent.h"
%End
  public:
    arInputEvent();
    arInputEvent( const arInputEventType type, const unsigned index );
    virtual ~arInputEvent();
    arInputEvent( const arInputEvent& e );
    // operator bool()...
    int __nonzero__();
%MethodCode
      sipRes = (int)(bool)*sipCpp;
%End
    
    arInputEventType getType() const;
    unsigned getIndex() const;
    int getButton() const;
    float getAxis() const;
    arMatrix4 getMatrix() const /TransferBack/;
    
    void setIndex( const unsigned i );
    bool setButton( const unsigned b );
    bool setAxis( const float a );
    //bool setMatrix( const float* v );
    bool setMatrix( const arMatrix4& m );
    void trash();
    void zero();

    string __str__();
%MethodCode
      ostringstream s(ostringstream::out);
      switch (sipCpp->getType()) {
        case AR_EVENT_BUTTON:
          s << "BUTTON[" << sipCpp->getIndex() << "]: " << sipCpp->getButton();
          break;
        case AR_EVENT_AXIS:
          s << "AXIS[" << sipCpp->getIndex() << "]: " << sipCpp->getAxis();
            break;
        case AR_EVENT_MATRIX:
          s << "MATRIX[" << sipCpp->getIndex() << "]:\n" << sipCpp->getMatrix();
          break;
        case AR_EVENT_GARBAGE:
          s << "GARBAGE[" << sipCpp->getIndex() << "]";
          break;
        default:
          s << "EVENT_ERROR[" << sipCpp->getIndex() << "]";
      }
      sipRes = new string( s.str() );
%End
};

class arButtonEvent : arInputEvent {
%TypeHeaderCode
#include "arInputEvent.h"
%End
  public:
    arButtonEvent( const unsigned index, const int b );
};

class arAxisEvent : arInputEvent {
%TypeHeaderCode
#include "arInputEvent.h"
%End
  public:
    arAxisEvent( const unsigned index, const float a );
};

class arMatrixEvent : arInputEvent {
%TypeHeaderCode
#include "arInputEvent.h"
%End
  public:
    //arMatrixEvent( const unsigned index, const float* v );
    arMatrixEvent( const unsigned index,  const arMatrix4& m );
};

class arGarbageEvent : arInputEvent {
%TypeHeaderCode
#include "arInputEvent.h"
%End
  public:
    arGarbageEvent();
};



class arIntInputDeviceMap {
%TypeHeaderCode
#include "arInputState.h"
%End
  public:
    arIntInputDeviceMap();
    ~arIntInputDeviceMap();
    unsigned getNumberDevices() const;
    unsigned getNumberEvents() const;
    unsigned getNumberDeviceEvents( const unsigned iDevice ) const;
    bool getEventOffset( const unsigned iDevice, unsigned& offset ) const;
//    void addInputDevice( const unsigned numEvents,
//                         vector<eventDataType>& dataSpace );
//    bool remapInputEvents( const unsigned iDevice,
//                           const unsigned newNumEvents,
//                           vector<eventDataType>& dataSpace );
  private:
   arIntInputDeviceMap( const arIntInputDeviceMap& rhs );
};

class arFloatInputDeviceMap {
%TypeHeaderCode
#include "arInputState.h"
%End
  public:
    arFloatInputDeviceMap();
    ~arFloatInputDeviceMap();
    unsigned getNumberDevices() const;
    unsigned getNumberEvents() const;
    unsigned getNumberDeviceEvents( const unsigned iDevice ) const;
    bool getEventOffset( const unsigned iDevice, unsigned& offset ) const;
//    void addInputDevice( const unsigned numEvents,
//                         vector<eventDataType>& dataSpace );
//    bool remapInputEvents( const unsigned iDevice,
//                           const unsigned newNumEvents,
//                           vector<eventDataType>& dataSpace );
  private:
   arFloatInputDeviceMap( const arFloatInputDeviceMap& rhs );
};

class arMatrixInputDeviceMap {
%TypeHeaderCode
#include "arInputState.h"
%End
  public:
    arMatrixInputDeviceMap();
    ~arMatrixInputDeviceMap();
    unsigned getNumberDevices() const;
    unsigned getNumberEvents() const;
    unsigned getNumberDeviceEvents( const unsigned iDevice ) const;
    bool getEventOffset( const unsigned iDevice, unsigned& offset ) const;
//    void addInputDevice( const unsigned numEvents,
//                         vector<eventDataType>& dataSpace );
//    bool remapInputEvents( const unsigned iDevice,
//                           const unsigned newNumEvents,
//                           vector<eventDataType>& dataSpace );
  private:
   arMatrixInputDeviceMap( const arMatrixInputDeviceMap& rhs );
};



class arInputState {
%TypeHeaderCode
#include "arInputState.h"
%End
  public:
    arInputState();
    arInputState( const arInputState& x );
    ~arInputState();

    // The number of buttons equals the button signature.
    unsigned getNumberButtons() const;
    unsigned getNumberAxes() const;
    unsigned getNumberMatrices() const;

    int getButton(       const unsigned ) const;
    float getAxis(       const unsigned ) const;
    arMatrix4 getMatrix( const unsigned ) const /TransferBack/;

    bool getOnButton(  const unsigned ) const;
    bool getOffButton( const unsigned ) const;

    bool setButton( const unsigned iButton, const int value );
    bool setAxis(   const unsigned iAxis, const float value );
    bool setMatrix( const unsigned iMatrix, const arMatrix4& value );

    bool update( const arInputEvent& event );

    void setSignature( const unsigned maxButtons,
                       const unsigned maxAxes,
                       const unsigned maxMatrices,
                       bool printWarnings=false );

    void addInputDevice( const unsigned numButtons,
                         const unsigned numAxes,
                         const unsigned numMatrices );

    void remapInputDevice( const unsigned iDevice,
                           const unsigned numButtons,
                           const unsigned numAxes,
                           const unsigned numMatrices );

    bool getButtonOffset( unsigned iDevice, unsigned& offset );
    bool getAxisOffset(   unsigned iDevice, unsigned& offset );
    bool getMatrixOffset( unsigned iDevice, unsigned& offset );

//    bool setFromBuffers( const int* const buttonBuf,
//                         const unsigned numButtons,
//                         const float* const axisBuf,
//                         const unsigned numAxes,
//                         const float* const matrixBuf,
//                         const unsigned numMatrices );
//    bool saveToBuffers( int* const buttonBuf,
//                        float* const axisBuf,
//                        float* const matrixBuf ) const;
                        
    void updateLastButtons();
    void updateLastButton( const unsigned index );

    arIntInputDeviceMap getButtonDeviceMap() const /TransferBack/;
// Necessary to get around "error: Taking address of a temporary" with g++ 4.7.0.
%MethodCode
  sipRes = new arIntInputDeviceMap( sipCpp->getButtonDeviceMap() );
%End

    arFloatInputDeviceMap getAxisDeviceMap() const /TransferBack/;
// Necessary to get around "error: Taking address of a temporary" with g++ 4.7.0.
%MethodCode
  sipRes = new arFloatInputDeviceMap( sipCpp->getAxisDeviceMap() );
%End

    arMatrixInputDeviceMap getMatrixDeviceMap() const /TransferBack/;
// Necessary to get around "error: Taking address of a temporary" with g++ 4.7.0.
%MethodCode
  sipRes = new arMatrixInputDeviceMap( sipCpp->getMatrixDeviceMap() );
%End


};  


class arInputEventQueue {
%TypeHeaderCode
#include "arInputEventQueue.h"
%End
  public:
    arInputEventQueue();
    ~arInputEventQueue();
    arInputEventQueue( const arInputEventQueue& q );
    void appendEvent( const arInputEvent& event );
    void appendQueue( const arInputEventQueue& queue );
    bool empty() const;
    unsigned size() const;
    arInputEvent popNextEvent();
    
    unsigned getNumberButtons()  const;
    unsigned getNumberAxes()     const;
    unsigned getNumberMatrices() const;

    void setSignature( unsigned numButtons,
                       unsigned numAxes,
                       unsigned numMatrices );
    unsigned getButtonSignature() const;
    unsigned getAxisSignature() const;
    unsigned getMatrixSignature() const;
                         
//    bool setFromBuffers( const int* const typeData,
//                         const int* const indexData,
//                         const int* const buttonData,
//                         const unsigned int numButtons,
//                         const float* const axisData,
//                         const unsigned int numAxes,
//                         const float* const matrixData,
//                         const unsigned int numMatrices );
//    bool saveToBuffers( int* const typeBuf,
//                        int* const indexBuf,
//                        int* const buttonBuf,
//                        float* const axisBuf,
//                        float* const matrixBuf ) const;
                        
    void clear();
};    


class arInputSource {
%TypeHeaderCode
#include "arInputSource.h"
%End
 public:
  arInputSource();
  virtual ~arInputSource();
  void setInputNode( arInputSink* );
  virtual bool init( arSZGClient& );
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
  int getNumberButtons() const;
  int getNumberAxes() const;
  int getNumberMatrices() const;

  // Send a single item.
  void sendButton(int index, int value) /ReleaseGIL/;
  void sendAxis(int index, float value) /ReleaseGIL/;
  void sendMatrix(int index, const arMatrix4& value) /ReleaseGIL/;
  void sendMatrix(const arMatrix4& value) /ReleaseGIL/;

  // Send several items as a single packet.
//  void sendButtonsAxesMatrices(
//    int numButtons, const int* rgiButtons, const int* rgvalueButtons,
//    int numAxes, const int* rgiAxes, const float* rgvalueAxes,
//    int numMatrices, const int* rgiMatrices, const arMatrix4* rgvalueMatrices);

  // Accumulate several values and then send them as a single packet.
  void queueButton(int index, int value);
  void queueAxis(int index, float value);
  void queueMatrix(int index, const arMatrix4& value);
  void queueMatrix(const arMatrix4& value);
  void sendQueue() /ReleaseGIL/;

  virtual void handleMessage( const string& messageType, const string& messageBody );
 private:
  arInputSource( const arInputSource& );
};


class arGenericDriver: arInputSource {
%TypeHeaderCode
#include "arGenericDriver.h"
%End
 public:
  arGenericDriver();
  ~arGenericDriver();
  void setSignature(unsigned, unsigned, unsigned);
};



class arSharedLibInputDriver {
%TypeHeaderCode
#include "arSharedLib.h"
#include "arGenericDriver.h"
#ifndef arSharedLibInputDriver_bugfix
#define arSharedLibInputDriver_bugfix
// sip 4.10 and 4.12 duplicate this block into sip_szgpart0.cpp.
class arSharedLibInputDriver : public arSharedLib {
 public:
  bool createFactory(const string& sharedLibName,
                     const string& path,
                     string& error) {
    return arSharedLib::createFactory( sharedLibName, path, "arInputSource", error );
  }
  arGenericDriver* _createObject() {
    return (arGenericDriver*) arSharedLib::createObject();
  }
};
#endif
%End
  public:
    bool createFactory(const string& sharedLibName,
                       const string& path,
                       string& error /Out/);
    arGenericDriver* _createObject() /PyName=createObject/;
  private:
    bool createFactory(const string& sharedLibName,
                       const string& path,
                       const string& type,
                       string& error);
};


class arNetInputSource: arInputSource {
%TypeHeaderCode
#include "arNetInputSource.h"
%End
 public:
  arNetInputSource();
  virtual ~arNetInputSource();

  // Only use ONE of these
  bool setSlot(unsigned);
  bool setServiceName( const string& name );

  bool connected() const;
  virtual bool init( arSZGClient& );
  virtual bool start();
};


class arInputSink {
%TypeHeaderCode
#include "arInputSink.h"
%End
 public:
  arInputSink();
  virtual ~arInputSink();

  virtual bool init( arSZGClient& );
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
  virtual bool sourceReconfig(int);
 private:
  arInputSink( const arInputSink& );
};


class arNetInputSink: arInputSink {
%TypeHeaderCode
#include "arNetInputSink.h"
%End
 public:
  arNetInputSink();
  virtual ~arNetInputSink();

  bool setSlot(unsigned slot);
  bool init(arSZGClient&);
  bool start();
  void setInfo(const string& info);
 private:
  arNetInputSink( const arNetInputSink& );
};


class arFileSink : arInputSink {
%TypeHeaderCode
#include "arFileSink.h"
%End
 public:
  arFileSink();
  ~arFileSink();
 
  bool init(arSZGClient&);
  bool start();
  bool stop();
};


class arInputNode: arInputSink {
%TypeHeaderCode
#include "arInputNode.h"
%End
  // Needs assignment operator and copy constructor, for pointer members.
  public:
    arInputNode( bool bufferEvents = false );
    // If anyone ever derives from this class, make the following virtual:
    // destructor init start stop restart receiveData sourceReconfig.
    ~arInputNode();
  
    bool init( arSZGClient& );
    bool start();
    bool stop();
    bool restart();
    //void receiveData(int,arStructuredData*);
    bool sourceReconfig(int);
    // iOwnIt iff the input node owns it & should delete it.
    //void addInputSource( arInputSource*, bool iOwnIt );
    void addInputSourceMine( arInputSource* a0 /TransferBack/ );
%MethodCode
      sipCpp->addInputSource( a0, false );
%End
    void addInputSourceYours( arInputSource* a0 /TransferThis/ );
%MethodCode
      sipCpp->addInputSource( a0, true );
%End

    //int addFilter( arIOFilter*, bool iOwnIt );
    void addFilterMine( arIOFilter* a0 /TransferBack/ );
%MethodCode
      sipCpp->addFilter( a0, false );
%End
    void addFilterYours( arIOFilter* a0 /TransferThis/ );
%MethodCode
      sipCpp->addFilter( a0, true );
%End

    //void addInputSink( arInputSink*, bool iOwnIt );
    void addInputSinkMine( arInputSink* a0 /TransferBack/ );
%MethodCode
      sipCpp->addInputSink( a0, false );
%End
    void addInputSinkYours( arInputSink* a0 /TransferThis/ );
%MethodCode
      sipCpp->addInputSink( a0, true );
%End
    
    //bool replaceFilter( int ID, arIOFilter* newFilter, bool iOwnIt );
    void replaceFilterMine( int a0, arIOFilter* a1 /TransferBack/ );
%MethodCode
      sipCpp->replaceFilter( a0, a1, false );
%End
    void replaceFilterYours( int a0, arIOFilter* a1 /TransferThis/ );
%MethodCode
      sipCpp->replaceFilter( a0, a1, true );
%End


    bool removeFilter( int ID );
    //void setEventCallback(void (*callback)(arInputEvent&))
  
    // getXXX() aren't const, because they use _dataSerializationLock.
    int getButton(int);
    float getAxis(int);
    arMatrix4 getMatrix(int) /TransferBack/;
  
    int getNumberButtons() const;
    int getNumberAxes() const;
    int getNumberMatrices() const;
    
    void processBufferedEvents();
    
    arInputState _inputState;

  private:
   arInputNode( const arInputNode& );
};


class arIOFilter {
%TypeHeaderCode
#include "arIOFilter.h"
%End
  public:
    arIOFilter();
    virtual ~arIOFilter();
  
    virtual bool configure( arSZGClient* );
    bool filter( arInputEventQueue* qin, arInputState* s );
    int getButton( const unsigned int index ) const;
    bool getOnButton(  const unsigned int buttonNumber );
    bool getOffButton( const unsigned int buttonNumber );
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const /TransferBack/;
    arInputState* getInputState();
    void insertNewEvent( const arInputEvent& newEvent );
    void setID( int id );
    int getID() const;
    
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent ) /PyName=onInputEvent/;
    
  private:
    arIOFilter( const arIOFilter& );
};


class arPyIOFilter : arIOFilter {
%TypeHeaderCode
class arPyIOFilter : public arIOFilter {
  public:
    virtual void onButtonEvent( arInputEvent& /*event*/, unsigned /*index*/ ) {}
    virtual void onAxisEvent( arInputEvent& /*event*/, unsigned /*index*/ ) {}
    virtual void onMatrixEvent( arInputEvent& /*event*/, unsigned /*index*/ ) {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent ) {
      arInputEventType typ( inputEvent.getType() );
      if (typ == AR_EVENT_BUTTON) {
        onButtonEvent( inputEvent, inputEvent.getIndex() );
        return true;
      }
      if (typ == AR_EVENT_AXIS) {
        onAxisEvent( inputEvent, inputEvent.getIndex() );
        return true;
      }
      if (typ == AR_EVENT_MATRIX) {
        onMatrixEvent( inputEvent, inputEvent.getIndex() );
        return true;
      }
      return true;  
    }
};
%End
  public:
    virtual void onButtonEvent( arInputEvent& event, unsigned index );
    virtual void onAxisEvent( arInputEvent& event, unsigned index );
    virtual void onMatrixEvent( arInputEvent& event, unsigned index );
};


class arInputNodeConfig {
%TypeHeaderCode
#include "arInputNodeConfig.h"
%End
  public:
    arInputNodeConfig();
    ~arInputNodeConfig();

    void addInputSource( const string& source );
    bool parseXMLRecord( const string& nodeConfig );
};


class arPyDeviceServerFramework {
%TypeHeaderCode
#include "arDeviceServerFramework.h"
class arPyDeviceServerFramework : public arDeviceServerFramework {
  public:
    arPyDeviceServerFramework() : arDeviceServerFramework() {}
    virtual ~arPyDeviceServerFramework() {}
    virtual bool init( vector<string>& argVec, const string& forcedName=string("NULL") ) {
      int argc;
      char** argv;
      ar_vectorToArgcArgv( argVec, argc, argv );
      bool stat = arDeviceServerFramework::init( argc, argv, forcedName );
      ar_cleanupArgcArgv( argc, argv );
      return stat;
    }
    bool checkCmdArg( vector<string>& argVec, const string& argVal ) {
      int argc;
      char** argv;
      ar_vectorToArgcArgv( argVec, argc, argv );
      bool stat = arDeviceServerFramework::checkCmdArg( argc, argv, (const char* const)argVal.c_str() );
      ar_cleanupArgcArgv( argc, argv );
      return stat;
    }
    bool extractCmdArg( vector<string>& argVec, int i, string& val ) {
      int argc;
      char** argv;
      ar_vectorToArgcArgv( argVec, argc, argv );
      bool stat = arDeviceServerFramework::extractCmdArg( argc, argv, i, val );
      ar_cleanupArgcArgv( argc, argv );
      return stat;
    }
  protected:
    bool _handleArgs( int& argc, char** argv ) {
      vector<string> argVec;
      ar_argcArgvToVector( argc, argv, argVec );
      return _handleArgs( argVec );
    }
    virtual bool _handleArgs( vector<string>& /*argVec*/ ) { return true; }
};
%End
  public:
    arPyDeviceServerFramework();
    virtual ~arPyDeviceServerFramework();
    virtual bool init( vector<string>& argVec, const string& forcedName=string("NULL") );
    void setDeviceName( const string& name );
    bool checkCmdArg( vector<string>& argVec, const string& a1 );
    bool extractCmdArg( vector<string>& argVec, int a1, string& a2 );
    virtual int messageLoop();
%MethodCode
    PyThreadState *_save;
    _save = PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();
    sipRes = sipCpp->messageLoop();
    PyEval_AcquireLock();
    PyThreadState_Swap(_save);
%End

    arSZGClient* getSZGClient();
    arInputNode* getInputNode();

  protected:
    virtual bool _handleArgs( vector<string>& argVec ) /PyName=handleArgs/;
    virtual void _printUsage() /PyName=printUsage/;
    virtual bool _configureInputNode() /PyName=configureInputNode/;
    virtual bool _handleMessage( const string& messageType, const string& messageBody ) /PyName=onMessage/;
    bool _addNetInput() /PyName=addNetInput/;
  private:
    arPyDeviceServerFramework( const arPyDeviceServerFramework& );
};

