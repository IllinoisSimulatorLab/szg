//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DEVICE_SERVER_FRAMEWORK_H
#define AR_DEVICE_SERVER_FRAMEWORK_H

#include "arSZGClient.h"
#include "arInputSource.h"
#include "arInputSink.h"
#include "arInputNode.h"
#include "arIOFilter.h"
#include "arNetInputSource.h"
#include "arNetInputSink.h"
#include "arFileSink.h"
#include "arDriversCalling.h"

#include <string>


class SZG_CALL arDeviceServerFramework {
  public:
    arDeviceServerFramework();
    virtual ~arDeviceServerFramework();
    virtual bool init( int& argc, char** argv, string forcedName=string("NULL") );
    void setDeviceName( const string& name ) { _deviceName = name; }
    bool checkCmdArg( int& argc, char** argv, const char* const sz );
    bool extractCmdArg( int& argc, char** argv, int i, string& val );
    virtual int messageLoop();
    arSZGClient* getSZGClient() { return &_szgClient; }
    arInputNode* getInputNode() { return &_inputNode; }
  protected:
    virtual bool _handleArgs( int& /*argc*/, char** /*argv*/ ) { return true; }
    virtual void _printUsage() {}
    virtual bool _configureInputNode();
    virtual bool _handleMessage( const string& /*messageType*/, const string& /*messageBody*/ )
                       { return true; }
    bool _addNetInput();
    bool _getLock();
    string _name;
    string _deviceName;
    unsigned _netOutputSlot;
    unsigned _nextNetInputSlot;
    bool _fNetInput;
    arSZGClient _szgClient;
    arInputNode _inputNode;
    arNetInputSink _netSink;
    arFileSink _fileSink;
};

#endif
