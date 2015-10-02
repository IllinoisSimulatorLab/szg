//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"

#include "arDeviceClient.h"


arDeviceClient::arDeviceClient( arSZGClient* szgClient ) :
    arMessageHandler( szgClient ) {}


arDeviceClient::~arDeviceClient() {
    stop();
}


bool arDeviceClient::init( int slotNum, std::vector<arIOFilter*>* filterVecPtr ) {

    if (!_src.setSlot( slotNum )) {
        ar_log_error() << "DeviceClient: invalid slot " << slotNum << ".\n";
        if (!_szgClientPtr->sendInitResponse( false )) {
            cerr << "arDeviceClient error: maybe szgserver died.\n";
        }
        return false;
    }
    ar_log_remark() << "arDeviceClient listening on slot " << slotNum << ".\n";

    return _init( filterVecPtr );
}


bool arDeviceClient::init( std::string serviceName, std::vector<arIOFilter*>* filterVecPtr ) {

    if (!_src.setServiceName( serviceName )) {
        ar_log_error() << "DeviceClient: invalid service name " << serviceName << ".\n";
        if (!_szgClientPtr->sendInitResponse( false )) {
            cerr << "arDeviceClient error: maybe szgserver died.\n";
        }
        return false;
    }
    ar_log_remark() << "arDeviceClient listening to service" << serviceName << ".\n";

    return _init( filterVecPtr );
}


bool arDeviceClient::_init( std::vector<arIOFilter*>* filterVecPtr ) {

    _inputNode.addInputSource( &_src, false );

    if (filterVecPtr) {
        std::vector<arIOFilter*>::iterator iter;
        for (iter = filterVecPtr->begin(); iter != filterVecPtr->end(); ++iter ) {
            _inputNode.addFilter( *iter, false );
        }
    }

    if (!_inputNode.init( *_szgClientPtr )) {
        if (!_szgClientPtr->sendInitResponse( false )) {
            cerr << "arDeviceClient error: maybe szgserver died.\n";
        }
        return false;
    }

    if (!_szgClientPtr->sendInitResponse( true )) {
        cerr << "arDeviceClient error: maybe szgserver died.\n";
    }

    ar_usleep(40000); // avoid interleaving diagnostics from init and start

    return true;
}



bool arDeviceClient::start( void (*msgHandlerThreadFunc)( void* ) ) {

    if (!_inputNode.start()) {
        if (!_szgClientPtr->sendStartResponse( false )) {
            cerr << "arDeviceClient error: maybe szgserver died.\n";
        }
        return false;
    }

    // todo: warn only after 2 seconds have elapsed (separate thread).
    if (!_src.connected()) {
        ar_log_warning() << "arDeviceClient not yet connected.\n";
    }

    if (!_szgClientPtr->sendStartResponse( true )) {
        cerr << "DeviceClient error: maybe szgserver died.\n";
    }

    arThread dummy( msgHandlerThreadFunc, static_cast<void*>(this) );

    return true;
}


bool arDeviceClient::stop() {
    quitMessageTask();
    return _inputNode.stop();
}





